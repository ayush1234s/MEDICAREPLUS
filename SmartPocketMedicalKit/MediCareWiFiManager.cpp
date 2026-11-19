#include "MediCareWiFiManager.h"

// ============================================================================
// GLOBAL AND STATIC VARIABLES
// ============================================================================
volatile WiFiMgrState wifiMgrState = WIFI_MGR_DISCONNECTED;
volatile bool wifiConnectedFlag = false;
SemaphoreHandle_t wifiStateMutex = NULL;
char wifiConnectedSSID[33] = {0};
bool wifiJustConnectedEvent = false;

static Preferences _prefs;
static WiFiConfigStore _configStore;

static uint8_t _currentAttemptIndex = 0;
static unsigned long _lastAttemptTime = 0;
static bool _attemptInProgress = false;
static unsigned long _attemptStartTime = 0;
static const unsigned long ATTEMPT_TIMEOUT_MS = 6000UL;

static void appendJSONString(String &out, const String &value) {
  static const char hex[] = "0123456789abcdef";
  out += '"';
  for (size_t i = 0; i < value.length(); ++i) {
    const uint8_t ch = static_cast<uint8_t>(value[i]);
    if (ch == '"' || ch == '\\') {
      out += '\\';
      out += static_cast<char>(ch);
    } else if (ch < 0x20) {
      out += "\\u00";
      out += hex[ch >> 4];
      out += hex[ch & 0x0f];
    } else {
      out += static_cast<char>(ch);
    }
  }
  out += '"';
}

// ============================================================================
// INITIALIZATION
// ============================================================================
void wifiManagerInit(const char* initialSeedSSID, const char* initialSeedPass) {
  if (wifiStateMutex == NULL) {
    wifiStateMutex = xSemaphoreCreateMutex();
  }
  
  memset(&_configStore, 0, sizeof(_configStore));
  _configStore.version = NVS_WIFI_VERSION;
  _configStore.count = 0;

  bool loaded = wifiManagerLoadConfig(_configStore);

  // If NVM has no networks saved yet, seed initial network once so it appears in NVM & Web Table
  if ((!loaded || _configStore.count == 0) && initialSeedSSID && strlen(initialSeedSSID) > 0) {
    strncpy(_configStore.networks[0].ssid, initialSeedSSID, 32);
    _configStore.networks[0].ssid[32] = '\0';
    if (initialSeedPass) {
      strncpy(_configStore.networks[0].password, initialSeedPass, 64);
      _configStore.networks[0].password[64] = '\0';
    } else {
      _configStore.networks[0].password[0] = '\0';
    }
    _configStore.count = 1;
    wifiManagerSaveConfig(_configStore);
    Serial.printf("[WiFiManager] Initialized NVM with initial network: %s\n", initialSeedSSID);
  }
}

// ============================================================================
// NVS / PREFERENCES PERSISTENCE
// ============================================================================
bool wifiManagerLoadConfig(WiFiConfigStore &store) {
  if (!_prefs.begin(NVS_WIFI_NAMESPACE, true)) { // Read-only
    Serial.println("[WiFiManager] NVS namespace not found or empty.");
    return false;
  }

  uint8_t ver = _prefs.getUChar("ver", 0);
  if (ver != NVS_WIFI_VERSION) {
    Serial.printf("[WiFiManager] NVS version mismatch (%d vs %d). Defaulting.\n", ver, NVS_WIFI_VERSION);
    _prefs.end();
    return false;
  }

  uint8_t count = _prefs.getUChar("count", 0);
  if (count > WIFI_MAX_NETWORKS) count = WIFI_MAX_NETWORKS;
  store.version = ver;
  store.count = count;

  for (uint8_t i = 0; i < count; i++) {
    char keySsid[16], keyPass[16];
    snprintf(keySsid, sizeof(keySsid), "ssid_%d", i);
    snprintf(keyPass, sizeof(keyPass), "pass_%d", i);

    String s = _prefs.getString(keySsid, "");
    String p = _prefs.getString(keyPass, "");

    strncpy(store.networks[i].ssid, s.c_str(), sizeof(store.networks[i].ssid) - 1);
    store.networks[i].ssid[sizeof(store.networks[i].ssid) - 1] = '\0';

    strncpy(store.networks[i].password, p.c_str(), sizeof(store.networks[i].password) - 1);
    store.networks[i].password[sizeof(store.networks[i].password) - 1] = '\0';
  }

  _prefs.end();
  Serial.printf("[WiFiManager] Loaded %d configured networks from NVM.\n", store.count);
  return true;
}

bool wifiManagerSaveConfig(const WiFiConfigStore &newStore) {
  if (newStore.count > WIFI_MAX_NETWORKS) {
    Serial.println("[WiFiManager] Save failed: Network count exceeds limit.");
    return false;
  }

  // Validate that SSIDs are non-empty
  for (uint8_t i = 0; i < newStore.count; i++) {
    if (strlen(newStore.networks[i].ssid) == 0) {
      Serial.printf("[WiFiManager] Save rejected: Network %d has empty SSID.\n", i + 1);
      return false;
    }
  }

  if (!_prefs.begin(NVS_WIFI_NAMESPACE, false)) { // Read-Write
    Serial.println("[WiFiManager] Failed to open NVS in read-write mode.");
    return false;
  }

  _prefs.putUChar("ver", NVS_WIFI_VERSION);
  _prefs.putUChar("count", newStore.count);

  for (uint8_t i = 0; i < newStore.count; i++) {
    char keySsid[16], keyPass[16];
    snprintf(keySsid, sizeof(keySsid), "ssid_%d", i);
    snprintf(keyPass, sizeof(keyPass), "pass_%d", i);

    _prefs.putString(keySsid, newStore.networks[i].ssid);
    _prefs.putString(keyPass, newStore.networks[i].password);
  }

  // Clean up any remaining slots if count decreased
  for (uint8_t i = newStore.count; i < WIFI_MAX_NETWORKS; i++) {
    char keySsid[16], keyPass[16];
    snprintf(keySsid, sizeof(keySsid), "ssid_%d", i);
    snprintf(keyPass, sizeof(keyPass), "pass_%d", i);
    _prefs.remove(keySsid);
    _prefs.remove(keyPass);
  }

  _prefs.end();

  // Update in-memory store thread-safely
  if (wifiStateMutex && xSemaphoreTake(wifiStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    memcpy(&_configStore, &newStore, sizeof(WiFiConfigStore));
    xSemaphoreGive(wifiStateMutex);
  } else {
    memcpy(&_configStore, &newStore, sizeof(WiFiConfigStore));
  }

  Serial.printf("[WiFiManager] Saved %d networks safely into NVM.\n", newStore.count);
  return true;
}

bool wifiManagerDeleteNetwork(uint8_t index) {
  if (index >= _configStore.count) return false;

  WiFiConfigStore updated;
  memset(&updated, 0, sizeof(updated));
  updated.version = NVS_WIFI_VERSION;
  updated.count = 0;

  for (uint8_t i = 0; i < _configStore.count; i++) {
    if (i == index) continue;
    memcpy(&updated.networks[updated.count], &_configStore.networks[i], sizeof(WiFiCredential));
    updated.count++;
  }

  return wifiManagerSaveConfig(updated);
}

// ============================================================
// STA MODE OPERATION & SCAN-AND-CONNECT FAILOVER
// ============================================================
void wifiManagerStartSTA() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);
  
  _currentAttemptIndex = 0;
  _lastAttemptTime = millis() - WIFI_RETRY_INTERVAL_MS;
  _attemptInProgress = false;

  if (wifiStateMutex && xSemaphoreTake(wifiStateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    wifiMgrState = WIFI_MGR_CONNECTING;
    wifiConnectedFlag = false;
    wifiConnectedSSID[0] = '\0';
    xSemaphoreGive(wifiStateMutex);
  }
}

// Scans nearby networks and returns an active index from NVM if found
static int scanForBestSavedNetwork() {
  if (_configStore.count == 0) return -1;

  Serial.println("[WiFiManager] Scanning nearby Wi-Fi networks...");
  int n = WiFi.scanNetworks(false, true); // Fast passive scan

  if (n <= 0) {
    Serial.printf("[WiFiManager] Scan found %d networks nearby.\n", n);
    return -1;
  }

  int bestIdx = -1;
  int bestRssi = -1000;

  for (int i = 0; i < n; i++) {
    String foundSsid = WiFi.SSID(i);
    int foundRssi = WiFi.RSSI(i);

    for (uint8_t s = 0; s < _configStore.count; s++) {
      if (foundSsid.equals(_configStore.networks[s].ssid)) {
        if (foundRssi > bestRssi) {
          bestRssi = foundRssi;
          bestIdx = s;
        }
      }
    }
  }

  WiFi.scanDelete();

  if (bestIdx >= 0) {
    Serial.printf("[WiFiManager] Match found: %s (Signal: %d dBm)\n", 
                  _configStore.networks[bestIdx].ssid, bestRssi);
  } else {
    Serial.println("[WiFiManager] No saved NVM network detected in range.");
  }

  return bestIdx;
}

void wifiManagerUpdate() {
  wl_status_t status = WiFi.status();

  if (status == WL_CONNECTED) {
    if (!wifiConnectedFlag) {
      if (wifiStateMutex && xSemaphoreTake(wifiStateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        wifiConnectedFlag = true;
        wifiMgrState = WIFI_MGR_CONNECTED;
        String curSsid = WiFi.SSID();
        strncpy(wifiConnectedSSID, curSsid.c_str(), sizeof(wifiConnectedSSID) - 1);
        wifiConnectedSSID[sizeof(wifiConnectedSSID) - 1] = '\0';
        wifiJustConnectedEvent = true;
        xSemaphoreGive(wifiStateMutex);
      } else {
        wifiConnectedFlag = true;
        wifiMgrState = WIFI_MGR_CONNECTED;
        String curSsid = WiFi.SSID();
        strncpy(wifiConnectedSSID, curSsid.c_str(), sizeof(wifiConnectedSSID) - 1);
        wifiJustConnectedEvent = true;
      }

      _attemptInProgress = false;

      // Configure NTP for India timezone UTC +5:30 (19800s offset, 0s daylight)
      configTime(19800, 0, "pool.ntp.org", "time.nist.gov");

      Serial.println();
      Serial.printf("[WiFiManager] Connected to SSID: %s | IP: %s\n", 
                    wifiConnectedSSID, WiFi.localIP().toString().c_str());
    }
    return;
  }

  // If connection was lost
  if (wifiConnectedFlag) {
    if (wifiStateMutex && xSemaphoreTake(wifiStateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      wifiConnectedFlag = false;
      wifiMgrState = WIFI_MGR_DISCONNECTED;
      wifiConnectedSSID[0] = '\0';
      xSemaphoreGive(wifiStateMutex);
    } else {
      wifiConnectedFlag = false;
      wifiMgrState = WIFI_MGR_DISCONNECTED;
      wifiConnectedSSID[0] = '\0';
    }
    Serial.println("[WiFiManager] Connection lost! Entering background scan & failover.");
    _lastAttemptTime = millis();
    _attemptInProgress = false;
  }

  // Check attempt timeout
  if (_attemptInProgress && (millis() - _attemptStartTime >= ATTEMPT_TIMEOUT_MS)) {
    Serial.println("[WiFiManager] Connection attempt timed out.");
    WiFi.disconnect();
    _attemptInProgress = false;
    _lastAttemptTime = millis();
    _currentAttemptIndex++;
  }

  // Non-blocking retry scheduling (~5 seconds): SCAN AND CONNECT
  if (!_attemptInProgress && (millis() - _lastAttemptTime >= WIFI_RETRY_INTERVAL_MS)) {
    if (_configStore.count == 0) {
      _lastAttemptTime = millis();
      return;
    }

    uint8_t targetIdx = 0;

    // First attempt a scan to see which NVM network is actually present in the air
    int scanMatch = scanForBestSavedNetwork();
    if (scanMatch >= 0) {
      targetIdx = (uint8_t)scanMatch;
    } else {
      if (_currentAttemptIndex >= _configStore.count) {
        _currentAttemptIndex = 0;
      }
      targetIdx = _currentAttemptIndex;
    }

    _lastAttemptTime = millis();
    _attemptStartTime = millis();
    _attemptInProgress = true;

    if (wifiStateMutex && xSemaphoreTake(wifiStateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      wifiMgrState = WIFI_MGR_CONNECTING;
      xSemaphoreGive(wifiStateMutex);
    }

    char* targetSSID = _configStore.networks[targetIdx].ssid;
    char* targetPass = _configStore.networks[targetIdx].password;

    Serial.printf("[WiFiManager] Connecting to NVM network [%d/%d]: %s (%s)\n",
                  targetIdx + 1, _configStore.count, targetSSID,
                  (strlen(targetPass) == 0 ? "OPEN" : "PROTECTED"));

    WiFi.disconnect();
    if (strlen(targetPass) == 0) {
      WiFi.begin(targetSSID);
    } else {
      WiFi.begin(targetSSID, targetPass);
    }
  }
}

// ============================================================
// ACCESS POINT MODE
// ============================================================
bool wifiManagerStartAP() {
  WiFi.disconnect(true);
  delay(50);
  // Use WIFI_AP_STA so the web portal can scan nearby networks while hosting the AP
  WiFi.mode(WIFI_AP_STA);

  bool cfg = WiFi.softAPConfig(MEDICARE_AP_IP, MEDICARE_AP_GW, MEDICARE_AP_SUBNET);
  if (!cfg) {
    Serial.println("[WiFiManager] AP IP Config Failed!");
  }

  bool apStarted = WiFi.softAP(MEDICARE_AP_SSID, MEDICARE_AP_PASS, 1, 0, 4);
  if (apStarted) {
    if (wifiStateMutex && xSemaphoreTake(wifiStateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      wifiMgrState = WIFI_MGR_AP_MODE;
      wifiConnectedFlag = false;
      xSemaphoreGive(wifiStateMutex);
    }
    Serial.println();
    Serial.println("==================================================");
    Serial.println("[WiFiManager] MediCare+ Configuration AP Started!");
    Serial.printf("SSID: %s | PASS: %s | IP: 192.168.4.1\n", MEDICARE_AP_SSID, MEDICARE_AP_PASS);
    Serial.println("==================================================");
    return true;
  } else {
    Serial.println("[WiFiManager] ERROR: Could not start Access Point!");
    return false;
  }
}

void wifiManagerStopAP() {
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  if (wifiStateMutex && xSemaphoreTake(wifiStateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    wifiMgrState = WIFI_MGR_DISCONNECTED;
    xSemaphoreGive(wifiStateMutex);
  }
}

// ============================================================
// LIVE SCAN FOR WEB PORTAL TABLE
// ============================================================
String wifiManagerScanNetworksJSON() {
  int n = WiFi.scanNetworks(false, true);
  if (n <= 0) {
    return "[]";
  }

  String json = "[";
  int added = 0;
  for (int i = 0; i < n && added < 15; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.length() == 0) continue;

    // Check for duplicates
    bool duplicate = false;
    for (int j = 0; j < i; j++) {
      if (WiFi.SSID(j) == ssid) {
        duplicate = true;
        break;
      }
    }
    if (duplicate) continue;

    if (added > 0) json += ",";
    json += "{\"ssid\":";
    appendJSONString(json, ssid);
    json += ",\"rssi\":";
    json += String(WiFi.RSSI(i));
    json += ",\"secure\":";
    json += (WiFi.encryptionType(i) != WIFI_AUTH_OPEN) ? "true" : "false";
    json += "}";
    added++;
  }
  json += "]";
  WiFi.scanDelete();
  return json;
}

// ============================================================
// THREAD-SAFE STATE ACCESSORS
// ============================================================
bool wifiManagerIsConnected() {
  bool conn = false;
  if (wifiStateMutex && xSemaphoreTake(wifiStateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    conn = wifiConnectedFlag;
    xSemaphoreGive(wifiStateMutex);
  } else {
    conn = wifiConnectedFlag;
  }
  return conn;
}

WiFiMgrState wifiManagerGetState() {
  WiFiMgrState st = WIFI_MGR_DISCONNECTED;
  if (wifiStateMutex && xSemaphoreTake(wifiStateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    st = wifiMgrState;
    xSemaphoreGive(wifiStateMutex);
  } else {
    st = wifiMgrState;
  }
  return st;
}

uint8_t wifiManagerGetNetworkCount() {
  uint8_t count = 0;
  if (wifiStateMutex && xSemaphoreTake(wifiStateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    count = _configStore.count;
    xSemaphoreGive(wifiStateMutex);
  } else {
    count = _configStore.count;
  }
  return count;
}

bool wifiManagerHasSavedNetworks() {
  return (wifiManagerGetNetworkCount() > 0);
}

bool wifiManagerGetNetwork(uint8_t index, char* outSsid, char* outPass) {
  bool ok = false;
  if (wifiStateMutex && xSemaphoreTake(wifiStateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    if (index < _configStore.count) {
      if (outSsid) {
        strncpy(outSsid, _configStore.networks[index].ssid, 32);
        outSsid[32] = '\0';
      }
      if (outPass) {
        strncpy(outPass, _configStore.networks[index].password, 64);
        outPass[64] = '\0';
      }
      ok = true;
    }
    xSemaphoreGive(wifiStateMutex);
  }
  return ok;
}

String wifiManagerGetConnectedSSID() {
  String s = "";
  if (wifiStateMutex && xSemaphoreTake(wifiStateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    s = String(wifiConnectedSSID);
    xSemaphoreGive(wifiStateMutex);
  } else {
    s = String(wifiConnectedSSID);
  }
  return s;
}

void wifiManagerConsumeJustConnectedEvent(String &outSsid) {
  outSsid = "";
  if (wifiStateMutex && xSemaphoreTake(wifiStateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    if (wifiJustConnectedEvent) {
      wifiJustConnectedEvent = false;
      outSsid = String(wifiConnectedSSID);
    }
    xSemaphoreGive(wifiStateMutex);
  }
}

#include "MediCareWiFiPortal.h"
#include "MediCareWiFiManager.h"
#include <pgmspace.h>

// ============================================================================
// BASE64 QR CODE FOR AP SETUP (WIFI:T:WPA;S:MediCare+;P:medic@re;;)
// Converted from supplied QR code image to compact 1-bit monochrome BMP (178
// bytes)
// ============================================================================
const char MEDICARE_AP_QR_BASE64[] =
    "Qk2yAAAAAAAAAD4AAAAoAAAAHQAAAOP///"
    "8BAAEAAAAAAHQAAAATCwAAEwsAAAIAAAAAAAAAAAAAAP///wABKPQAfXCN8EVZ/"
    "RBFrGUQRXKlEH3xlfABVVQA/"
    "9TP+"
    "GAhi0CLlCtwJUxRmEb8Dngpqe1wvwaeOFCk5pCLgfGA1G73gAs2hjAE2C0QKpLzABRKoFD/"
    "UtdgARq1eH1DByhFImBARSUTUEWWwkB96nkQAUriGA==";

WebServer portalServer(80);

// External display functions provided by SmartPocketMedicalKit.ino
extern void fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                     uint16_t color);
extern void drawRect(int16_t x, int16_t y, int16_t w, int16_t h,
                     uint16_t color);
extern void drawText(const char *text, int x, int y, uint16_t color,
                     uint8_t scale);
extern void drawCentered(const char *text, int y, uint16_t color,
                         uint8_t scale);
extern void drawPixel(int16_t x, int16_t y, uint16_t color);

// Color definitions matching main sketch
#define PORTAL_BLACK 0x0000
#define PORTAL_WHITE 0xFFFF
#define PORTAL_CYAN 0x07FF
#define PORTAL_YELLOW 0xFFE0
#define PORTAL_GREEN 0x07E0
#define PORTAL_RED 0xF800
#define PORTAL_DARKBLUE 0x10A2
#define PORTAL_DARKGREY 0x4208
#define PORTAL_LIGHTGREY 0xC618
#define PORTAL_CARD_BG 0x0841

// ============================================================================
// RESPONSIVE TABLE-FORMAT HTML/CSS/JS (Stored in PROGMEM)
// ============================================================================
static const char PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0,maximum-scale=1.0,user-scalable=no">
<title>MediCare+ Wi-Fi Portal</title>
<style>
:root{--bg:#0b132b;--card:#1c2541;--border:#3a506b;--primary:#48cae4;--primary-dark:#0096c7;--text:#edf2f4;--text-dim:#8d99ae;--danger:#ef233c;--success:#06d6a0;--warn:#ffd166;}
*{box-sizing:border-box;margin:0;padding:0;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;}
body{background:var(--bg);color:var(--text);padding:14px;min-height:100vh;}
.container{max-width:540px;margin:0 auto;}
header{text-align:center;padding:10px 0 16px;}
.logo{font-size:24px;font-weight:800;letter-spacing:1px;color:var(--primary);}
.subtitle{font-size:13px;color:var(--text-dim);margin-top:3px;}
.card{background:var(--card);border:1px solid var(--border);border-radius:12px;padding:14px;margin-bottom:14px;box-shadow:0 4px 6px -1px rgba(0,0,0,0.3);}
.card-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:12px;}
.card-title{font-size:14px;font-weight:700;color:var(--primary);display:flex;align-items:center;gap:6px;}
.badge{background:var(--primary-dark);color:#fff;font-size:10px;padding:2px 7px;border-radius:10px;}
.table-responsive{width:100%;overflow-x:auto;-webkit-overflow-scrolling:touch;}
table{width:100%;border-collapse:collapse;margin-bottom:10px;font-size:13px;}
th{background:#0f172a;color:var(--text-dim);text-align:left;padding:8px 6px;font-size:11px;text-transform:uppercase;letter-spacing:0.5px;border-bottom:1px solid var(--border);}
td{padding:8px 6px;border-bottom:1px solid rgba(58,80,107,0.4);vertical-align:middle;}
input.table-input{width:100%;padding:8px 8px;background:#0b132b;border:1px solid var(--border);border-radius:6px;color:#fff;font-size:13px;outline:none;}
input.table-input:focus{border-color:var(--primary);}
.btn{padding:10px 14px;border:none;border-radius:8px;font-size:13px;font-weight:600;cursor:pointer;display:inline-flex;align-items:center;justify-content:center;gap:6px;transition:0.15s;}
.btn-primary{background:var(--primary);color:#0b132b;width:100%;padding:12px;margin-top:6px;}
.btn-primary:active{background:var(--primary-dark);}
.btn-outline{background:transparent;border:1px dashed var(--primary);color:var(--primary);width:100%;margin-bottom:8px;}
.btn-scan{background:#2b2d42;color:var(--primary);padding:6px 12px;font-size:12px;border:1px solid var(--border);}
.btn-add{background:var(--success);color:#0b132b;padding:4px 8px;font-size:11px;border-radius:4px;}
.btn-del{background:transparent;border:1px solid var(--danger);color:var(--danger);padding:4px 8px;font-size:11px;border-radius:4px;}
.btn-del:active{background:var(--danger);color:#fff;}
.btn-reboot{background:#3a506b;color:#edf2f4;width:100%;padding:12px;margin-top:10px;}
.signal-good{color:var(--success);font-weight:bold;}
.signal-mid{color:var(--warn);}
.signal-weak{color:var(--text-dim);}
.empty-msg{text-align:center;color:var(--text-dim);padding:14px;font-size:12px;}
.toast{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);padding:12px 20px;border-radius:8px;font-size:13px;font-weight:600;display:none;z-index:99;box-shadow:0 10px 15px -3px rgba(0,0,0,0.6);}
.toast-success{background:var(--success);color:#0b132b;}
.toast-error{background:var(--danger);color:#fff;}
</style>
</head>
<body>
<div class="container">
  <header>
    <div class="logo">MEDICARE+</div>
    <div class="subtitle">Your Smart Medical Kit</div>
  </header>

  <!-- SAVED NETWORKS TABLE -->
  <div class="card">
    <div class="card-header">
      <div class="card-title">Saved Networks in NVM <span id="netCountBadge" class="badge">0 / 10</span></div>
    </div>
    
    <div class="table-responsive">
      <table>
        <thead>
          <tr>
            <th style="width:8%">#</th>
            <th style="width:44%">Network (SSID)</th>
            <th style="width:34%">Password</th>
            <th style="width:14%;text-align:center">Action</th>
          </tr>
        </thead>
        <tbody id="savedTableBody"></tbody>
      </table>
    </div>

    <button type="button" class="btn btn-outline" onclick="addBlankRow()">Add Network Row</button>
    <button type="button" class="btn btn-primary" onclick="saveAllToNVM()">SAVE</button>
    <button type="button" class="btn btn-reboot" onclick="rebootDevice()">REBOOT MEDICARE+</button>
  </div>

  <!-- NEARBY DETECTED WI-FI SCAN TABLE -->
  <div class="card">
    <div class="card-header">
      <div class="card-title">Nearby Available Wi-Fi</div>
      <button type="button" class="btn btn-scan" id="btnScan" onclick="scanNearby()">Scan</button>
    </div>
    
    <div class="table-responsive">
      <table>
        <thead>
          <tr>
            <th style="width:15%">Signal</th>
            <th style="width:55%">Nearby SSID</th>
            <th style="width:15%">Auth</th>
            <th style="width:15%;text-align:center">Action</th>
          </tr>
        </thead>
        <tbody id="scannedTableBody">
          <tr><td colspan="4" class="empty-msg">Click "Scan" to detect nearby Wi-Fi</td></tr>
        </tbody>
      </table>
    </div>
  </div>
</div>

<div id="toast" class="toast"></div>

<script>
let savedNetworks = [];

function escapeHtml(value) {
  return String(value).replace(/[&<>"']/g, ch => ({
    '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;'
  })[ch]);
}

function showToast(msg, isError) {
  const t = document.getElementById('toast');
  t.innerText = msg;
  t.className = 'toast ' + (isError ? 'toast-error' : 'toast-success');
  t.style.display = 'block';
  setTimeout(() => { t.style.display = 'none'; }, 3500);
}

function renderSavedTable() {
  const tbody = document.getElementById('savedTableBody');
  tbody.innerHTML = '';
  document.getElementById('netCountBadge').innerText = `${savedNetworks.length} / 10`;

  if (savedNetworks.length === 0) {
    tbody.innerHTML = '<tr><td colspan="4" class="empty-msg">No networks saved in NVM.<br>Click "Add Network" or select from scanned list below.</td></tr>';
    return;
  }

  savedNetworks.forEach((net, idx) => {
    const tr = document.createElement('tr');
    tr.innerHTML = `
      <td style="color:var(--text-dim);font-weight:bold;">${idx + 1}</td>
      <td>
        <input type="text" class="table-input" maxlength="32" placeholder="SSID" value="${escapeHtml(net.ssid || '')}" oninput="savedNetworks[${idx}].ssid=this.value.trim()">
      </td>
      <td>
        <input type="password" class="table-input" maxlength="64" placeholder="${net.hasPass ? '(Unchanged)' : 'Password'}" value="${escapeHtml(net.password || '')}" oninput="savedNetworks[${idx}].password=this.value">
      </td>
      <td style="text-align:center;">
        <button type="button" class="btn btn-del" onclick="removeRow(${idx})">Delete</button>
      </td>
    `;
    tbody.appendChild(tr);
  });
}

function addBlankRow() {
  if (savedNetworks.length >= 10) {
    showToast('Maximum 10 networks allowed in NVM', true);
    return;
  }
  savedNetworks.push({ ssid: '', password: '', hasPass: false });
  renderSavedTable();
}

function removeRow(idx) {
  savedNetworks.splice(idx, 1);
  renderSavedTable();
}

function addFromScan(ssid) {
  // Check if already in savedNetworks
  for (let item of savedNetworks) {
    if (item.ssid === ssid) {
      showToast(`'${ssid}' is already in the table!`, true);
      return;
    }
  }
  if (savedNetworks.length >= 10) {
    showToast('Maximum 10 networks allowed in NVM', true);
    return;
  }
  savedNetworks.push({ ssid: ssid, password: '', hasPass: false });
  renderSavedTable();
  showToast(`Added '${ssid}'. Enter password and click Save.`, false);
}

async function loadSavedFromNVM() {
  try {
    const res = await fetch('/api/networks');
    if (res.ok) {
      const data = await res.json();
      if (Array.isArray(data)) {
        savedNetworks = data.map(item => ({ ssid: item.ssid, password: '', hasPass: item.hasPass }));
      }
    }
  } catch (e) {
    savedNetworks = [];
  }
  renderSavedTable();
}

async function scanNearby() {
  const btn = document.getElementById('btnScan');
  const tbody = document.getElementById('scannedTableBody');
  btn.innerText = 'Scanning...';
  tbody.innerHTML = '<tr><td colspan="4" class="empty-msg">Scanning 2.4GHz channels... please wait.</td></tr>';

  try {
    const res = await fetch('/api/scan');
    if (res.ok) {
      const list = await res.json();
      tbody.innerHTML = '';
      if (!Array.isArray(list) || list.length === 0) {
        tbody.innerHTML = '<tr><td colspan="4" class="empty-msg">No networks detected nearby.</td></tr>';
      } else {
        list.forEach(item => {
          let sigClass = item.rssi >= -60 ? 'signal-good' : (item.rssi >= -75 ? 'signal-mid' : 'signal-weak');
          const tr = document.createElement('tr');
          tr.innerHTML = `
            <td class="${sigClass}">${item.rssi} dBm</td>
            <td style="font-weight:600;">${escapeHtml(item.ssid)}</td>
            <td style="color:var(--text-dim);font-size:11px;">${item.secure ? 'WPA/WPA2' : 'Open'}</td>
            <td style="text-align:center;">
              <button type="button" class="btn btn-add">Add</button>
            </td>
          `;
          tr.querySelector('.btn-add').addEventListener('click', () => addFromScan(item.ssid));
          tbody.appendChild(tr);
        });
      }
    } else {
      tbody.innerHTML = '<tr><td colspan="4" class="empty-msg">Scan request failed.</td></tr>';
    }
  } catch (e) {
    tbody.innerHTML = '<tr><td colspan="4" class="empty-msg">Scan error or timeout.</td></tr>';
  }
  btn.innerText = 'Scan';
}

async function saveAllToNVM() {
  // Validation
  for (let i = 0; i < savedNetworks.length; i++) {
    if (!savedNetworks[i].ssid || savedNetworks[i].ssid.trim().length === 0) {
      showToast(`Row #${i + 1} has empty SSID!`, true);
      return;
    }
  }

  try {
    const res = await fetch('/api/save', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(savedNetworks)
    });
    const result = await res.json();
    if (result && result.status === 'ok') {
      showToast('All Networks Saved Permanently to NVM!', false);
      loadSavedFromNVM();
    } else {
      showToast('Error saving: ' + (result.message || 'Unknown'), true);
    }
  } catch (e) {
    showToast('Network error while saving to NVM', true);
  }
}

async function rebootDevice() {
  if (!confirm('Reboot MediCare+ now to apply NVM Wi-Fi settings?')) return;
  try {
    await fetch('/api/reboot');
    showToast('Rebooting MediCare+... Connecting to saved NVM network.', false);
  } catch (e) {
    showToast('Reboot initiated.', false);
  }
}

window.onload = () => {
  loadSavedFromNVM();
  scanNearby();
};
</script>
</body>
</html>
)rawliteral";

// ============================================================================
// BASE64 DECODER (LIGHTWEIGHT STANDALONE)
// ============================================================================
static int base64Decode(const char *input, uint8_t *output, int maxOutLen) {
  static const int8_t b64Table[128] = {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
      52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
      -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
      15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
      -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
      41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1};

  int inLen = strlen(input);
  int outIdx = 0;
  uint32_t buf = 0;
  int bits = 0;

  for (int i = 0; i < inLen; i++) {
    char c = input[i];
    if (c == '=')
      break;
    if (c < 0 || c >= 128)
      continue;
    int8_t val = b64Table[(uint8_t)c];
    if (val < 0)
      continue;

    buf = (buf << 6) | val;
    bits += 6;

    if (bits >= 8) {
      bits -= 8;
      if (outIdx < maxOutLen) {
        output[outIdx++] = (buf >> bits) & 0xFF;
      }
    }
  }
  return outIdx;
}

// ============================================================================
// SIMPLE JSON PARSER FOR SAVE
// ============================================================================
static int jsonHexDigit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

static bool readJSONString(const String &json, int quotePos, String &out,
                           int &afterPos) {
  if (quotePos < 0 || quotePos >= json.length() || json[quotePos] != '"')
    return false;
  out = "";
  for (int i = quotePos + 1; i < json.length(); ++i) {
    uint8_t ch = static_cast<uint8_t>(json[i]);
    if (ch == '"') {
      afterPos = i + 1;
      return true;
    }
    if (ch != '\\') {
      out += static_cast<char>(ch);
      continue;
    }
    if (++i >= json.length()) return false;
    char escaped = json[i];
    switch (escaped) {
      case '"': out += '"'; break;
      case '\\': out += '\\'; break;
      case '/': out += '/'; break;
      case 'b': out += '\b'; break;
      case 'f': out += '\f'; break;
      case 'n': out += '\n'; break;
      case 'r': out += '\r'; break;
      case 't': out += '\t'; break;
      case 'u': {
        if (i + 4 >= json.length()) return false;
        uint16_t code = 0;
        for (int digit = 1; digit <= 4; ++digit) {
          int value = jsonHexDigit(json[i + digit]);
          if (value < 0) return false;
          code = (code << 4) | value;
        }
        i += 4;
        if (code < 0x80) {
          out += static_cast<char>(code);
        } else if (code < 0x800) {
          out += static_cast<char>(0xC0 | (code >> 6));
          out += static_cast<char>(0x80 | (code & 0x3F));
        } else {
          out += static_cast<char>(0xE0 | (code >> 12));
          out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
          out += static_cast<char>(0x80 | (code & 0x3F));
        }
        break;
      }
      default: return false;
    }
  }
  return false;
}

static bool parseNetworksJson(const String &body, WiFiConfigStore &store) {
  store.count = 0;
  store.version = NVS_WIFI_VERSION;

  String normalized = body;
  normalized.trim();
  if (normalized == "[]") return true;

  int idx = 0;
  while (store.count < WIFI_MAX_NETWORKS) {
    int ssidPos = body.indexOf("\"ssid\":", idx);
    if (ssidPos == -1)
      break;

    int q1 = body.indexOf("\"", ssidPos + 7);
    String ssid;
    int ssidEnd = 0;
    if (!readJSONString(body, q1, ssid, ssidEnd)) break;

    String pass = "";
    int passPos = body.indexOf("\"password\":", ssidEnd);
    int nextRecordPos = ssidEnd;
    if (passPos != -1 && passPos < (ssidEnd + 100)) {
      int pq1 = body.indexOf("\"", passPos + 11);
      int passEnd = 0;
      if (readJSONString(body, pq1, pass, passEnd)) {
        nextRecordPos = passEnd;
      }
    }
    idx = nextRecordPos;

    if (ssid.length() > 0) {
      strncpy(store.networks[store.count].ssid, ssid.c_str(), 32);
      store.networks[store.count].ssid[32] = '\0';

      // If user left password blank, check if existing network had a password
      if (pass.length() == 0) {
        char existingPass[65] = {0};
        char existingSsid[33] = {0};
        for (uint8_t s = 0; s < wifiManagerGetNetworkCount(); s++) {
          if (wifiManagerGetNetwork(s, existingSsid, existingPass)) {
            if (ssid.equals(existingSsid)) {
              strncpy(store.networks[store.count].password, existingPass, 64);
              break;
            }
          }
        }
      } else {
        strncpy(store.networks[store.count].password, pass.c_str(), 64);
        store.networks[store.count].password[64] = '\0';
      }

      store.count++;
    }
  }
  return (store.count > 0);
}

// ============================================================================
// HTTP ROUTE HANDLERS
// ============================================================================
static void handleRoot() { portalServer.send(200, "text/html", PORTAL_HTML); }

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

static void handleGetNetworks() {
  WiFiConfigStore store;
  memset(&store, 0, sizeof(store));
  wifiManagerLoadConfig(store);

  String json = "[";
  for (uint8_t i = 0; i < store.count; i++) {
    if (i > 0)
      json += ",";
    json += "{\"id\":";
    json += String(i);
    json += ",\"ssid\":";
    appendJSONString(json, String(store.networks[i].ssid));
    json += ",\"hasPass\":";
    json += (strlen(store.networks[i].password) > 0) ? "true" : "false";
    json += "}";
  }
  json += "]";
  portalServer.send(200, "application/json", json);
}

static void handleScan() {
  String json = wifiManagerScanNetworksJSON();
  portalServer.send(200, "application/json", json);
}

static void handleSave() {
  if (!portalServer.hasArg("plain")) {
    portalServer.send(400, "application/json",
                      "{\"status\":\"error\",\"message\":\"Missing payload\"}");
    return;
  }

  String body = portalServer.arg("plain");
  WiFiConfigStore newStore;
  memset(&newStore, 0, sizeof(newStore));

  if (!parseNetworksJson(body, newStore)) {
    portalServer.send(400, "application/json",
                      "{\"status\":\"error\",\"message\":\"Invalid networks "
                      "list or empty SSIDs\"}");
    return;
  }

  if (wifiManagerSaveConfig(newStore)) {
    portalServer.send(200, "application/json",
                      "{\"status\":\"ok\",\"count\":" + String(newStore.count) +
                          "}");
  } else {
    portalServer.send(
        500, "application/json",
        "{\"status\":\"error\",\"message\":\"NVM flash save failed\"}");
  }
}

static void handleDelete() {
  if (portalServer.hasArg("id")) {
    int id = portalServer.arg("id").toInt();
    if (wifiManagerDeleteNetwork((uint8_t)id)) {
      portalServer.send(200, "application/json", "{\"status\":\"ok\"}");
      return;
    }
  }
  portalServer.send(400, "application/json",
                    "{\"status\":\"error\",\"message\":\"Invalid id\"}");
}

static void handleReboot() {
  portalServer.send(200, "application/json", "{\"status\":\"rebooting\"}");
  delay(500);
  ESP.restart();
}

// ============================================================================
// PORTAL SETUP & LOOP
// ============================================================================
void wifiPortalInit() {
  portalServer.on("/", HTTP_GET, handleRoot);
  portalServer.on("/api/networks", HTTP_GET, handleGetNetworks);
  portalServer.on("/api/scan", HTTP_GET, handleScan);
  portalServer.on("/api/save", HTTP_POST, handleSave);
  portalServer.on("/api/delete", HTTP_POST, handleDelete);
  portalServer.on("/api/reboot", HTTP_GET, handleReboot);

  portalServer.begin();
  Serial.println(
      "[WiFiPortal] HTTP Configuration Server started on http://192.168.4.1");
}

void wifiPortalHandleClient() { portalServer.handleClient(); }

void wifiPortalStop() {
  portalServer.stop();
  Serial.println("[WiFiPortal] HTTP Configuration Server stopped.");
}

// ============================================================================
// AP MODE TFT SCREEN WITH QR CODE
// ============================================================================
void drawAPModeScreen() {
  fillRect(0, 0, 320, 240, PORTAL_BLACK);

  // Top header banner
  fillRect(0, 0, 320, 32, PORTAL_DARKBLUE);
  fillRect(0, 30, 320, 2, PORTAL_CYAN);
  drawCentered("MEDICARE+ WIFI SETUP", 10, PORTAL_CYAN, 1);

  // Center QR Box coordinates
  const int qrBoxSize = 99;
  const int qrBoxX = (320 - qrBoxSize) / 2;
  const int qrBoxY = 38;

  bool hasBase64Data = (strlen(MEDICARE_AP_QR_BASE64) > 50 &&
                        strncmp(MEDICARE_AP_QR_BASE64, "REPLACE", 7) != 0);

  if (hasBase64Data) {
    const int maxBuf = 2048;
    uint8_t *decBuf = (uint8_t *)malloc(maxBuf);
    if (decBuf) {
      int decBytes = base64Decode(MEDICARE_AP_QR_BASE64, decBuf, maxBuf);
      if (decBytes > 54 && decBuf[0] == 'B' && decBuf[1] == 'M') {
        int32_t width = *(int32_t *)&decBuf[18];
        int32_t height = *(int32_t *)&decBuf[22];
        uint32_t dataOffset = *(uint32_t *)&decBuf[10];

        if (width > 0 && height != 0 && dataOffset < (uint32_t)decBytes) {
          int absH = abs(height);
          bool topDown = (height < 0);
          int rowStride = ((width + 31) / 32) * 4;

          int scale = 3;
          int renderedW = width * scale;
          int renderedH = absH * scale;
          int startX = qrBoxX + (qrBoxSize - renderedW) / 2;
          int startY = qrBoxY + (qrBoxSize - renderedH) / 2;

          fillRect(qrBoxX, qrBoxY, qrBoxSize, qrBoxSize, PORTAL_WHITE);

          for (int r = 0; r < absH; r++) {
            int bmpRow = topDown ? r : (absH - 1 - r);
            int rowOffset = dataOffset + bmpRow * rowStride;
            if (rowOffset + rowStride > decBytes)
              break;

            for (int c = 0; c < width; c++) {
              uint8_t byteVal = decBuf[rowOffset + (c / 8)];
              int bitIdx = 7 - (c % 8);
              bool isWhite = (byteVal >> bitIdx) & 0x01;
              if (!isWhite) {
                fillRect(startX + c * scale, startY + r * scale, scale, scale,
                         PORTAL_BLACK);
              }
            }
          }
        }
      }
      free(decBuf);
    }
  } else {
    fillRect(qrBoxX, qrBoxY, qrBoxSize, qrBoxSize, PORTAL_WHITE);
    drawRect(qrBoxX + 2, qrBoxY + 2, qrBoxSize - 4, qrBoxSize - 4,
             PORTAL_BLACK);

    const int mSize = 18;
    fillRect(qrBoxX + 4, qrBoxY + 4, mSize, mSize, PORTAL_BLACK);
    fillRect(qrBoxX + 7, qrBoxY + 7, mSize - 6, mSize - 6, PORTAL_WHITE);
    fillRect(qrBoxX + 9, qrBoxY + 9, mSize - 10, mSize - 10, PORTAL_BLACK);

    fillRect(qrBoxX + qrBoxSize - mSize - 4, qrBoxY + 4, mSize, mSize,
             PORTAL_BLACK);
    fillRect(qrBoxX + qrBoxSize - mSize - 1, qrBoxY + 7, mSize - 6, mSize - 6,
             PORTAL_WHITE);
    fillRect(qrBoxX + qrBoxSize - mSize + 1, qrBoxY + 9, mSize - 10, mSize - 10,
             PORTAL_BLACK);

    fillRect(qrBoxX + 4, qrBoxY + qrBoxSize - mSize - 4, mSize, mSize,
             PORTAL_BLACK);
    fillRect(qrBoxX + 7, qrBoxY + qrBoxSize - mSize - 1, mSize - 6, mSize - 6,
             PORTAL_WHITE);
    fillRect(qrBoxX + 9, qrBoxY + qrBoxSize - mSize + 1, mSize - 10, mSize - 10,
             PORTAL_BLACK);

    drawCentered("SCAN TO", qrBoxY + 36, PORTAL_BLACK, 1);
    drawCentered("CONNECT", qrBoxY + 48, PORTAL_BLACK, 1);
  }

  // Border around QR container
  drawRect(qrBoxX - 2, qrBoxY - 2, qrBoxSize + 4, qrBoxSize + 4, PORTAL_CYAN);

  // Setup credentials card below QR code
  int cardY = 144;
  int cardH = 70;
  drawRect(16, cardY, 288, cardH, PORTAL_CYAN);
  fillRect(17, cardY + 1, 286, cardH - 2, PORTAL_CARD_BG);

  drawCentered("Network: MediCare+", cardY + 8, PORTAL_WHITE, 1);
  drawCentered("Password: medic@re", cardY + 24, PORTAL_YELLOW, 1);
  drawCentered("Open: 192.168.4.1", cardY + 40, PORTAL_CYAN, 1);
  drawCentered("Add your Wi-Fi from Phone Browser", cardY + 54,
               PORTAL_LIGHTGREY, 1);

  // Footer bar
  fillRect(0, 220, 320, 20, PORTAL_DARKBLUE);
  if (wifiManagerGetNetworkCount() == 0) {
    drawCentered("NO SAVED NETWORKS - SETUP REQUIRED", 226, PORTAL_YELLOW, 1);
  } else {
    drawCentered("HOLD BACK AT BOOT FOR THIS PORTAL", 226, PORTAL_LIGHTGREY, 1);
  }
}

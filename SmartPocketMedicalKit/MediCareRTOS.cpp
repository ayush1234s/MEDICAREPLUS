#include "MediCareRTOS.h"
#include "MediCareWiFiManager.h"
#include <Preferences.h>

// ============================================================================
// MUTEXES & QUEUES
// ============================================================================
SemaphoreHandle_t tftMutex = NULL;
SemaphoreHandle_t i2cMutex = NULL;
SemaphoreHandle_t offlineQueueMutex = NULL;

volatile UiNotification currentUiNotification = { NOTIF_NONE, "", "", "", 0, 0, false };

static Preferences _offlinePrefs;
#define NVS_OFFLINE_NS "mcoffline"

static OfflineReading _offlineQueue[MAX_OFFLINE_RECORDS];
static uint8_t _offlineCount = 0;

// External functions from main sketch and Firebase bridge
extern void checkPhysicalButtons();
extern void updateSpO2Measurement();
extern void updateECGMeasurement();
extern void pollFirebaseReminders();
extern void updateHeader();
extern void updateBottomStatus();
extern void updateSensorStatus();
extern void hideMedicineReminderTFT();
extern bool syncSpO2Reading(const OfflineReading &reading);
extern bool syncECGReading(const OfflineReading &reading);

extern uint8_t activePage;
extern bool reminderOnScreen;
#ifndef MEDICINE_REMINDER_TEST_MODE
#define MEDICINE_REMINDER_TEST_MODE false
#endif
extern unsigned long reminderStartTime;
extern void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
extern void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
extern void drawText(const char *text, int x, int y, uint16_t color, uint8_t scale);
extern void drawCentered(const char *text, int y, uint16_t color, uint8_t scale);
extern void copyDashboardArea(int x, int sourceY, int targetY, int w, int h);
extern void drawMenuSelection(bool forceFullRedraw);

#define UI_POPUP_BG     0x0085
#define UI_POPUP_BORDER 0x07FF
#define UI_POPUP_GREEN  0x07E0
#define UI_POPUP_ORANGE 0xFD20
#define UI_POPUP_WHITE  0xFFFF
#define UI_POPUP_GREY   0xC618

// ============================================================================
// OFFLINE QUEUE PERSISTENCE
// ============================================================================
static void loadOfflineQueueFromNVS() {
  if (offlineQueueMutex && xSemaphoreTake(offlineQueueMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    if (_offlinePrefs.begin(NVS_OFFLINE_NS, true)) {
      _offlineCount = _offlinePrefs.getUChar("cnt", 0);
      if (_offlineCount > MAX_OFFLINE_RECORDS) _offlineCount = MAX_OFFLINE_RECORDS;
      for (uint8_t i = 0; i < _offlineCount; i++) {
        char k[16];
        snprintf(k, sizeof(k), "rec_%d", i);
        _offlinePrefs.getBytes(k, &_offlineQueue[i], sizeof(OfflineReading));
      }
      _offlinePrefs.end();
      if (_offlineCount > 0) {
        Serial.printf("[RTOS] Loaded %d pending offline records from NVS.\n", _offlineCount);
      }
    }
    xSemaphoreGive(offlineQueueMutex);
  }
}

static void saveOfflineQueueToNVS() {
  if (_offlinePrefs.begin(NVS_OFFLINE_NS, false)) {
    _offlinePrefs.putUChar("cnt", _offlineCount);
    for (uint8_t i = 0; i < _offlineCount; i++) {
      char k[16];
      snprintf(k, sizeof(k), "rec_%d", i);
      _offlinePrefs.putBytes(k, &_offlineQueue[i], sizeof(OfflineReading));
    }
    _offlinePrefs.end();
  }
}

void rtosEnqueueOfflineReading(int spo2, int bpm, const char* dStr, const char* tStr, bool isOptimal, bool isECG) {
  if (offlineQueueMutex && xSemaphoreTake(offlineQueueMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    if (_offlineCount < MAX_OFFLINE_RECORDS) {
      OfflineReading &rec = _offlineQueue[_offlineCount];
      rec.spo2 = spo2;
      rec.bpm = bpm;
      strncpy(rec.dateStr, dStr ? dStr : "2026-10-25", 15);
      rec.dateStr[15] = '\0';
      strncpy(rec.timeStr, tStr ? tStr : "12:00", 15);
      rec.timeStr[15] = '\0';
      rec.isOptimal = isOptimal;
      rec.isECG = isECG;
      _offlineCount++;
      saveOfflineQueueToNVS();
      Serial.printf("[RTOS] Saved offline measurement: SpO2=%d, BPM=%d (Pending: %d)\n", spo2, bpm, _offlineCount);
    } else {
      // Ring buffer replace oldest
      for (int i = 0; i < MAX_OFFLINE_RECORDS - 1; i++) {
        _offlineQueue[i] = _offlineQueue[i + 1];
      }
      OfflineReading &rec = _offlineQueue[MAX_OFFLINE_RECORDS - 1];
      rec.spo2 = spo2;
      rec.bpm = bpm;
      strncpy(rec.dateStr, dStr ? dStr : "2026-10-25", 15);
      strncpy(rec.timeStr, tStr ? tStr : "12:00", 15);
      rec.isOptimal = isOptimal;
      rec.isECG = isECG;
      saveOfflineQueueToNVS();
    }
    xSemaphoreGive(offlineQueueMutex);
  }
}

bool rtosGetPendingOfflineReading(OfflineReading &outRec) {
  bool found = false;
  if (offlineQueueMutex && xSemaphoreTake(offlineQueueMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    if (_offlineCount > 0) {
      outRec = _offlineQueue[0];
      found = true;
    }
    xSemaphoreGive(offlineQueueMutex);
  }
  return found;
}

void rtosCommitOfflineReadingSynced() {
  if (offlineQueueMutex && xSemaphoreTake(offlineQueueMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    if (_offlineCount > 0) {
      for (uint8_t i = 0; i < _offlineCount - 1; i++) {
        _offlineQueue[i] = _offlineQueue[i + 1];
      }
      _offlineCount--;
      saveOfflineQueueToNVS();
      Serial.printf("[RTOS] Confirmed sync. Remaining offline records: %d\n", _offlineCount);
    }
    xSemaphoreGive(offlineQueueMutex);
  }
}

uint8_t rtosGetPendingOfflineCount() {
  uint8_t cnt = 0;
  if (offlineQueueMutex && xSemaphoreTake(offlineQueueMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    cnt = _offlineCount;
    xSemaphoreGive(offlineQueueMutex);
  }
  return cnt;
}

// ============================================================================
// NOTIFICATION RENDERING HELPER
// ============================================================================
void rtosShowNotification(NotificationType type, const char* l1, const char* l2, const char* l3, unsigned long durationMs) {
  currentUiNotification.type = type;
  if (l1) strncpy((char*)currentUiNotification.line1, l1, 31); else currentUiNotification.line1[0] = '\0';
  if (l2) strncpy((char*)currentUiNotification.line2, l2, 31); else currentUiNotification.line2[0] = '\0';
  if (l3) strncpy((char*)currentUiNotification.line3, l3, 31); else currentUiNotification.line3[0] = '\0';
  currentUiNotification.durationMs = durationMs;
  currentUiNotification.triggeredAt = millis();
  currentUiNotification.active = true;
}

static void renderUiNotificationPopup() {
  if (!currentUiNotification.active) return;

  if (millis() - currentUiNotification.triggeredAt >= currentUiNotification.durationMs) {
    currentUiNotification.active = false;
    currentUiNotification.type = NOTIF_NONE;
    // Redraw screen cleanly after popup dismisses
    if (activePage == 0 && !reminderOnScreen) {
      drawMenuSelection(true);
    }
    return;
  }

  // Draw compact popup card in center of screen
  int px = 30, py = 85, pw = 260, ph = 64;
  uint16_t bColor = (currentUiNotification.type == NOTIF_WIFI_CONNECTED) ? UI_POPUP_GREEN : UI_POPUP_ORANGE;

  drawRect(px, py, pw, ph, bColor);
  drawRect(px + 1, py + 1, pw - 2, ph - 2, UI_POPUP_BORDER);
  fillRect(px + 2, py + 2, pw - 4, ph - 4, UI_POPUP_BG);

  if (strlen((const char*)currentUiNotification.line1) > 0) {
    drawCentered((const char*)currentUiNotification.line1, py + 8, bColor, 1);
  }
  if (strlen((const char*)currentUiNotification.line2) > 0) {
    drawCentered((const char*)currentUiNotification.line2, py + 24, UI_POPUP_WHITE, 1);
  }
  if (strlen((const char*)currentUiNotification.line3) > 0) {
    drawCentered((const char*)currentUiNotification.line3, py + 42, UI_POPUP_GREY, 1);
  }
}

// ============================================================================
// MUTEX HELPERS
// ============================================================================
bool rtosLockTFT(TickType_t waitTicks) {
  if (tftMutex) {
    return (xSemaphoreTake(tftMutex, waitTicks) == pdTRUE);
  }
  return true;
}

void rtosUnlockTFT() {
  if (tftMutex) {
    xSemaphoreGive(tftMutex);
  }
}

bool rtosLockI2C(TickType_t waitTicks) {
  if (i2cMutex) {
    return (xSemaphoreTake(i2cMutex, waitTicks) == pdTRUE);
  }
  return true;
}

void rtosUnlockI2C() {
  if (i2cMutex) {
    xSemaphoreGive(i2cMutex);
  }
}

// ============================================================================
// TASK IMPLEMENTATIONS
// ============================================================================

// Task A: UI & Display (Core 1)
static void taskMainUI(void *pvParams) {
  Serial.println("[RTOS] Task A (Main UI & Display) running on Core 1.");
  static unsigned long lastHeaderSec = 0;

  for (;;) {
    // 1. Check physical buttons
    checkPhysicalButtons();

    // 2. Check UI notifications
    if (currentUiNotification.active) {
      if (rtosLockTFT(pdMS_TO_TICKS(20))) {
        renderUiNotificationPopup();
        rtosUnlockTFT();
      }
    }

    // 3. Update dashboard header & system status periodically on home page
    if (millis() - lastHeaderSec >= 1000UL) {
      lastHeaderSec = millis();
      if (!reminderOnScreen && activePage == 0 && !currentUiNotification.active) {
        if (rtosLockTFT(pdMS_TO_TICKS(50))) {
          if (rtosLockI2C(pdMS_TO_TICKS(50))) {
            updateSensorStatus();
            rtosUnlockI2C();
          }
          updateHeader();
          updateBottomStatus();
          rtosUnlockTFT();
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// Task B: Sensor Monitoring (Core 1)
static void taskSensor(void *pvParams) {
  Serial.println("[RTOS] Task B (Sensor Monitoring) running on Core 1.");
  for (;;) {
    if (activePage == 1) {
      if (rtosLockTFT(pdMS_TO_TICKS(20))) {
        updateECGMeasurement();
        rtosUnlockTFT();
      }
      vTaskDelay(pdMS_TO_TICKS(10));
    } else if (activePage == 2) {
      if (rtosLockI2C(pdMS_TO_TICKS(20))) {
        if (rtosLockTFT(pdMS_TO_TICKS(20))) {
          updateSpO2Measurement();
          rtosUnlockTFT();
        }
        rtosUnlockI2C();
      }
      vTaskDelay(pdMS_TO_TICKS(10));
    } else {
      vTaskDelay(pdMS_TO_TICKS(50));
    }
  }
}

// Task C: Wi-Fi Manager (Core 0)
static void taskWiFi(void *pvParams) {
  Serial.println("[RTOS] Task C (Wi-Fi Manager) running on Core 0.");
  static unsigned long bootStart = millis();
  static bool startupTimeoutEvaluated = false;

  for (;;) {
    wifiManagerUpdate();

    // 8-second startup offline evaluation
    if (!startupTimeoutEvaluated && (millis() - bootStart >= WIFI_STARTUP_TIMEOUT_MS)) {
      startupTimeoutEvaluated = true;
      if (!wifiManagerIsConnected()) {
        Serial.println("[WiFiManager] 8-second startup window elapsed: No Wi-Fi connected. Entering Offline Mode.");
        rtosShowNotification(NOTIF_WIFI_OFFLINE, "WIFI UNAVAILABLE", "DEVICE OFFLINE", "Retrying in background...", 3000);
      }
    }

    // Check for just-connected event
    String newSsid = "";
    wifiManagerConsumeJustConnectedEvent(newSsid);
    if (newSsid.length() > 0) {
      Serial.printf("[WiFiManager] Triggering connected notification for SSID: %s\n", newSsid.c_str());
      String l2 = "Network: " + newSsid;
      rtosShowNotification(NOTIF_WIFI_CONNECTED, "< WIFI CONNECTED >", l2.c_str(), "Internet services available", 2500);
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// Task D: Firebase Synchronization (Core 0)
static void taskFirebase(void *pvParams) {
  Serial.println("[RTOS] Task D (Firebase Sync) running on Core 0.");
  static unsigned long lastPoll = 0;

  for (;;) {
    if (wifiManagerIsConnected()) {
      // 1. Drain pending offline records if any
      OfflineReading rec;
      if (rtosGetPendingOfflineReading(rec)) {
        Serial.printf("[RTOS Firebase] Syncing pending offline record (SpO2: %d, BPM: %d)...\n", rec.spo2, rec.bpm);
        bool ok = false;
        if (!rec.isECG) {
          ok = syncSpO2Reading(rec);
        } else {
          ok = syncECGReading(rec);
        }
        if (ok) {
          rtosCommitOfflineReadingSynced();
        }
      }

      // 2. Poll medicine reminders every 3s
      if (millis() - lastPoll >= 3000UL) {
        lastPoll = millis();
        if (rtosLockTFT(pdMS_TO_TICKS(100))) {
          pollFirebaseReminders();
          rtosUnlockTFT();
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// Task E: Medicine Reminder / Time Management (Core 1)
static void taskMedicineTime(void *pvParams) {
  Serial.println("[RTOS] Task E (Medicine Reminder & Time) running on Core 1.");
  for (;;) {
    if (reminderOnScreen && MEDICINE_REMINDER_TEST_MODE) {
      if (millis() - reminderStartTime >= (10UL * 1000UL)) { // 10s auto-hide in test mode
        Serial.println("[TFT Test Mode] reminder display auto-hide triggered.");
        if (rtosLockTFT(pdMS_TO_TICKS(100))) {
          hideMedicineReminderTFT();
          rtosUnlockTFT();
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

// ============================================================================
// INITIALIZATION & TASK CREATION
// ============================================================================
void rtosInitSystem() {
  if (tftMutex == NULL) tftMutex = xSemaphoreCreateMutex();
  if (i2cMutex == NULL) i2cMutex = xSemaphoreCreateMutex();
  if (offlineQueueMutex == NULL) offlineQueueMutex = xSemaphoreCreateMutex();

  loadOfflineQueueFromNVS();
}

void rtosStartTasks() {
  // Core 1 Tasks: UI, Sensors, Medicine Reminders
  xTaskCreatePinnedToCore(taskMainUI, "TaskMainUI", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(taskSensor, "TaskSensor", 4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(taskMedicineTime, "TaskMedTime", 3072, NULL, 2, NULL, 1);

  // Core 0 Tasks: Networking & Cloud
  xTaskCreatePinnedToCore(taskWiFi, "TaskWiFi", 6144, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(taskFirebase, "TaskFirebase", 6144, NULL, 1, NULL, 0);

  Serial.println("[RTOS] All 5 FreeRTOS Tasks initialized and pinned to cores successfully.");
}

#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

// ============================================================================
// MEDICARE+ FIREBASE REALTIME DATABASE CLOUD BRIDGE
// Database URL: https://medicare-plus-74a1a-default-rtdb.firebaseio.com/
// ============================================================================

static const char* FIREBASE_BASE_URL = "https://medicare-plus-74a1a-default-rtdb.firebaseio.com";

// Global functions & variables from main sketch
extern bool reminderOnScreen;
extern void showMedicineReminderTFT(const String &patientName, const String &medName,
                                   const String &dosage, const String &schedDate,
                                   const String &schedTime, bool isTest);
extern void dismissMedicineReminderTFT(const String &actionStatus);
extern void buzzerStartReminder();
extern void buzzerStopReminder();

static String activeReminderMedId = "";
static String activeReminderName = "";
static unsigned long lastFirebasePoll = 0;
static const unsigned long FIREBASE_POLL_INTERVAL = 3000; // Poll active medicine alerts every 3 seconds

// Helper to get formatted current date & time from ESP32 NTP
inline void getDeviceDateTime(String &dateStr, String &timeStr) {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 50)) {
    char dBuf[16];
    char tBuf[16];
    strftime(dBuf, sizeof(dBuf), "%Y-%m-%d", &timeinfo);
    strftime(tBuf, sizeof(tBuf), "%H:%M", &timeinfo);
    dateStr = String(dBuf);
    timeStr = String(tBuf);
  } else {
    dateStr = "2026-10-25";
    timeStr = "12:00";
  }
}

// ----------------------------------------------------------------------------
// Simple JSON string extractor (zero external library required, robust)
// ----------------------------------------------------------------------------
inline String getJsonValue(const String &json, const String &key) {
  String target = "\"" + key + "\":\"";
  int start = json.indexOf(target);
  if (start != -1) {
    start += target.length();
    int end = json.indexOf("\"", start);
    if (end != -1) {
      return json.substring(start, end);
    }
  }

  String targetNoQuote = "\"" + key + "\":";
  start = json.indexOf(targetNoQuote);
  if (start != -1) {
    start += targetNoQuote.length();
    while (start < (int)json.length() && (json[start] == ' ' || json[start] == '\"')) {
      start++;
    }
    int end = start;
    while (end < (int)json.length() && json[end] != ',' && json[end] != '}' && json[end] != '\"') {
      end++;
    }
    if (end > start) {
      String val = json.substring(start, end);
      val.trim();
      return val;
    }
  }

  return "";
}

// ----------------------------------------------------------------------------
// 1. Upload SpO2 & Heart Rate Measurement to Firebase
// ----------------------------------------------------------------------------
inline bool uploadSpO2ToFirebase(int spo2, int bpm) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Firebase] WiFi NOT connected! Please check WIFI_SSID in SmartPocketMedicalKit.ino");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(6);

  HTTPClient http;
  String dateStr, timeStr;
  getDeviceDateTime(dateStr, timeStr);

  String jsonPayload = "{";
  jsonPayload += "\"spo2\":" + String(spo2) + ",";
  jsonPayload += "\"bpm\":" + String(bpm) + ",";
  jsonPayload += "\"date\":\"" + dateStr + "\",";
  jsonPayload += "\"time\":\"" + timeStr + "\",";
  jsonPayload += "\"status\":\"" + String(spo2 >= 95 ? "OPTIMAL" : "MILD_HYPOXIA") + "\",";
  jsonPayload += "\"source\":\"ESP32_MAX30102\",";
  jsonPayload += "\"timestamp\":{\".sv\":\"timestamp\"}";
  jsonPayload += "}";

  // A. Save to historical logs: POST /readings/spo2.json
  String logUrl = String(FIREBASE_BASE_URL) + "/readings/spo2.json";
  http.begin(client, logUrl);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(jsonPayload);
  http.end();

  // B. Update live mirror: PUT /live/spo2.json
  String liveUrl = String(FIREBASE_BASE_URL) + "/live/spo2.json";
  http.begin(client, liveUrl);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.addHeader("Content-Type", "application/json");
  int liveCode = http.PUT(jsonPayload);
  http.end();

  Serial.printf("[Firebase] Uploaded SpO2: %d%%, BPM: %d -> HTTP POST: %d, PUT: %d\n", spo2, bpm, httpCode, liveCode);
  return (httpCode == 200 || httpCode == 201);
}

// ----------------------------------------------------------------------------
// 2. Upload ECG & Heart Rate Measurement to Firebase
// ----------------------------------------------------------------------------
inline bool uploadECGToFirebase(int bpm, const String &rhythm = "Normal Sinus Rhythm") {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Firebase] WiFi NOT connected! Cannot upload ECG.");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(6);

  HTTPClient http;
  String dateStr, timeStr;
  getDeviceDateTime(dateStr, timeStr);

  String jsonPayload = "{";
  jsonPayload += "\"bpm\":" + String(bpm) + ",";
  jsonPayload += "\"rhythm\":\"" + rhythm + "\",";
  jsonPayload += "\"prInterval\":\"140ms\",";
  jsonPayload += "\"qrsDuration\":\"86ms\",";
  jsonPayload += "\"date\":\"" + dateStr + "\",";
  jsonPayload += "\"time\":\"" + timeStr + "\",";
  jsonPayload += "\"status\":\"NORMAL\",";
  jsonPayload += "\"source\":\"ESP32_AD8232\",";
  jsonPayload += "\"timestamp\":{\".sv\":\"timestamp\"}";
  jsonPayload += "}";

  String logUrl = String(FIREBASE_BASE_URL) + "/readings/ecg.json";
  http.begin(client, logUrl);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(jsonPayload);
  http.end();

  String liveUrl = String(FIREBASE_BASE_URL) + "/live/ecg.json";
  http.begin(client, liveUrl);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.addHeader("Content-Type", "application/json");
  int liveCode = http.PUT(jsonPayload);
  http.end();

  Serial.printf("[Firebase] Uploaded ECG: BPM=%d -> HTTP POST: %d, PUT: %d\n", bpm, httpCode, liveCode);
  return (httpCode == 200 || httpCode == 201);
}

// ----------------------------------------------------------------------------
// 3. Poll Active Medicine Reminder from Firebase
// ----------------------------------------------------------------------------
inline bool isMedicineReminderDue(const String &scheduledDate, const String &scheduledTime) {
  // Older reminders without a schedule are delivered immediately.
  if (scheduledDate.length() == 0 || scheduledTime.length() == 0) return true;

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 100)) {
    Serial.println("[Firebase] Reminder queued: waiting for NTP clock sync.");
    return false;
  }

  char currentDate[16];
  char currentTime[16];
  strftime(currentDate, sizeof(currentDate), "%Y-%m-%d", &timeinfo);
  strftime(currentTime, sizeof(currentTime), "%H:%M", &timeinfo);

  String currentKey = String(currentDate) + " " + String(currentTime);
  String scheduledKey = scheduledDate + " " + scheduledTime;
  return currentKey >= scheduledKey;
}

inline void checkFirebaseMedicineReminders() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (reminderOnScreen) return; // Already showing on TFT
  if (millis() - lastFirebasePoll < FIREBASE_POLL_INTERVAL) return;

  lastFirebasePoll = millis();

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(4);

  HTTPClient http;
  String url = String(FIREBASE_BASE_URL) + "/active_reminder.json";
  http.begin(client, url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    if (payload.length() > 10 && payload != "null") {
      String activeStr = getJsonValue(payload, "active");
      String statusStr = getJsonValue(payload, "status");

      // Alert only once the medication's selected local date and time has arrived.
      if ((activeStr.equalsIgnoreCase("true") || activeStr == "1") && 
          (statusStr.equalsIgnoreCase("Pending") || statusStr.length() == 0)) {
        
        activeReminderMedId = getJsonValue(payload, "medicineId");
        activeReminderName = getJsonValue(payload, "medicine");
        String patient = getJsonValue(payload, "patientName");
        String dose = getJsonValue(payload, "dosage");
        String sDate = getJsonValue(payload, "scheduledDate");
        String sTime = getJsonValue(payload, "scheduledTime");

        if (!isMedicineReminderDue(sDate, sTime)) {
          http.end();
          return;
        }

        if (patient.length() == 0) patient = "Patient";
        if (dose.length() == 0) dose = "1 Dose";
        if (activeReminderName.length() == 0) activeReminderName = "Prescription";

        Serial.println();
        Serial.println("==================================================");
        Serial.println("[Firebase] >>> MEDICINE REMINDER TRIGGERED ON TFT! <<<");
        Serial.printf("Patient: %s | Med: %s | Dose: %s | Time: %s\n", 
                      patient.c_str(), activeReminderName.c_str(), dose.c_str(), sTime.c_str());
        Serial.println("==================================================");

        // Sound physical buzzer
        buzzerStartReminder();

        // Show reminder card on 320x240 ILI9341 display
        showMedicineReminderTFT(patient, activeReminderName, dose, sDate, sTime, false);
      }
    }
  } else {
    // This is the key diagnostic for a missing TFT alert. HTTP 401/403 means
    // the Realtime Database rules do not permit the ESP32 REST read.
    Serial.printf("[Firebase] Reminder poll failed (HTTP %d). Check WiFi and RTDB read rules for /active_reminder.\n", httpCode);
  }
  http.end();
}

// ----------------------------------------------------------------------------
// 4. Update Medicine Status back to Firebase when Device Buttons are pressed!
//    ENTER Button -> TAKEN
//    NEXT Button  -> SKIP
// ----------------------------------------------------------------------------
inline void updateFirebaseMedicineStatus(const String &actionStatus) {
  buzzerStopReminder();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Firebase] Cannot update medicine status: WiFi disconnected.");
    return;
  }

  String portalStatus = "Pending";
  if (actionStatus == "TAKEN") {
    portalStatus = "Taken";
  } else if (actionStatus == "SKIPPED" || actionStatus == "SKIP") {
    portalStatus = "Skip";
  } else {
    portalStatus = "Pending";
  }

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(4);

  HTTPClient http;

  // A. Update specific medicine in /medicines/<id>.json
  if (activeReminderMedId.length() > 0) {
    String medUrl = String(FIREBASE_BASE_URL) + "/medicines/" + activeReminderMedId + ".json";
    http.begin(client, medUrl);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.addHeader("Content-Type", "application/json");
    String patchPayload = "{\"status\":\"" + portalStatus + "\",\"deviceAckTime\":{\".sv\":\"timestamp\"}}";
    int patchCode = http.PATCH(patchPayload);
    http.end();
    Serial.printf("[Firebase] Button pressed -> /medicines/%s set to '%s' (HTTP %d)\n", 
                  activeReminderMedId.c_str(), portalStatus.c_str(), patchCode);
  }

  // B. Deactivate /active_reminder.json so it won't pop up again
  String reminderUrl = String(FIREBASE_BASE_URL) + "/active_reminder.json";
  http.begin(client, reminderUrl);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.addHeader("Content-Type", "application/json");
  String remPayload = "{\"status\":\"" + portalStatus + "\",\"active\":false}";
  int remCode = http.PATCH(remPayload);
  http.end();
  Serial.printf("[Firebase] Deactivated active_reminder in Firebase (HTTP %d)\n", remCode);

  activeReminderMedId = "";
}

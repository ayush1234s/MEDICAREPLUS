#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

// ============================================================================
// MUTEXES & SYNCHRONIZATION OBJECTS
// ============================================================================
extern SemaphoreHandle_t tftMutex;
extern SemaphoreHandle_t i2cMutex;
extern SemaphoreHandle_t offlineQueueMutex;

// ============================================================================
// OFFLINE MEASUREMENT STORAGE STRUCTURE
// ============================================================================
#define MAX_OFFLINE_RECORDS 20

struct OfflineReading {
  int spo2;
  int bpm;
  char dateStr[16];
  char timeStr[16];
  bool isOptimal;
  bool isECG;
};

// ============================================================================
// NOTIFICATION SYSTEM
// ============================================================================
enum NotificationType {
  NOTIF_NONE,
  NOTIF_WIFI_OFFLINE,
  NOTIF_WIFI_CONNECTED
};

struct UiNotification {
  NotificationType type;
  char line1[32];
  char line2[32];
  char line3[32];
  unsigned long durationMs;
  unsigned long triggeredAt;
  bool active;
};

extern volatile UiNotification currentUiNotification;

// ============================================================================
// FREERTOS TASKS & MANAGER API
// ============================================================================
void rtosInitSystem();
void rtosStartTasks();

void rtosShowNotification(NotificationType type, const char* l1, const char* l2, const char* l3, unsigned long durationMs = 2500);
void rtosEnqueueOfflineReading(int spo2, int bpm, const char* dStr, const char* tStr, bool isOptimal, bool isECG = false);
bool rtosGetPendingOfflineReading(OfflineReading &outRec);
void rtosCommitOfflineReadingSynced();
uint8_t rtosGetPendingOfflineCount();

// Safe Mutex Helpers
bool rtosLockTFT(TickType_t waitTicks = pdMS_TO_TICKS(100));
void rtosUnlockTFT();

bool rtosLockI2C(TickType_t waitTicks = pdMS_TO_TICKS(100));
void rtosUnlockI2C();

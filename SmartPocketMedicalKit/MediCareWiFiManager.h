#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// ============================================================================
// MEDICARE+ WI-FI MANAGER CONSTANTS
// ============================================================================
#define MEDICARE_AP_SSID          "MediCare+"
#define MEDICARE_AP_PASS          "medic@re"
#define MEDICARE_AP_IP            IPAddress(192, 168, 4, 1)
#define MEDICARE_AP_GW            IPAddress(192, 168, 4, 1)
#define MEDICARE_AP_SUBNET        IPAddress(255, 255, 255, 0)

#define WIFI_MAX_NETWORKS         10
#define WIFI_RETRY_INTERVAL_MS    5000UL
#define WIFI_STARTUP_TIMEOUT_MS   8000UL

#define NVS_WIFI_NAMESPACE        "mcwifi"
#define NVS_WIFI_VERSION          1

// Payload format: WIFI:T:WPA;S:MediCare+;P:medic@re;;
#define MEDICARE_AP_QR_PAYLOAD    "WIFI:T:WPA;S:MediCare+;P:medic@re;;"

// ============================================================================
// DATA STRUCTURES
// ============================================================================
struct WiFiCredential {
  char ssid[33];      // Max 32 chars + null
  char password[65];  // Max 64 chars + null
};

struct WiFiConfigStore {
  uint8_t version;
  uint8_t count;
  WiFiCredential networks[WIFI_MAX_NETWORKS];
};

enum WiFiMgrState {
  WIFI_MGR_DISCONNECTED,
  WIFI_MGR_SCANNING,
  WIFI_MGR_CONNECTING,
  WIFI_MGR_CONNECTED,
  WIFI_MGR_AP_MODE
};

// ============================================================================
// PUBLIC EXTERNALS & STATE
// ============================================================================
extern volatile WiFiMgrState wifiMgrState;
extern volatile bool wifiConnectedFlag;
extern SemaphoreHandle_t wifiStateMutex;
extern char wifiConnectedSSID[33];
extern bool wifiJustConnectedEvent;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================
void wifiManagerInit(const char* initialSeedSSID = nullptr, const char* initialSeedPass = nullptr);
bool wifiManagerLoadConfig(WiFiConfigStore &store);
bool wifiManagerSaveConfig(const WiFiConfigStore &store);
bool wifiManagerDeleteNetwork(uint8_t index);
void wifiManagerStartSTA();
void wifiManagerUpdate();
bool wifiManagerStartAP();
void wifiManagerStopAP();
bool wifiManagerIsConnected();
WiFiMgrState wifiManagerGetState();
uint8_t wifiManagerGetNetworkCount();
bool wifiManagerHasSavedNetworks();
bool wifiManagerGetNetwork(uint8_t index, char* outSsid, char* outPass);
String wifiManagerGetConnectedSSID();
void wifiManagerConsumeJustConnectedEvent(String &outSsid);
String wifiManagerScanNetworksJSON();

#pragma once

#include <Arduino.h>
#include <WebServer.h>

// ============================================================================
// BASE64 QR CODE PLACEHOLDER
// Paste your BASE64 encoded QR image string below when available.
// Supported format: BASE64 encoded 1-bit monochrome bitmap / BMP or raw byte stream.
// ============================================================================
extern const char MEDICARE_AP_QR_BASE64[];

// ============================================================================
// WEB SERVER & PORTAL FUNCTIONS
// ============================================================================
extern WebServer portalServer;

void wifiPortalInit();
void wifiPortalHandleClient();
void wifiPortalStop();
void drawAPModeScreen();

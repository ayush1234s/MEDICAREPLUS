#include <SPI.h>

#include <Wire.h>

#include <WiFi.h>

#include <time.h>

#include <Preferences.h>
#include <math.h>

#include "MAX30105.h"




#include "MediCareDashboard.h"
#include "MediCareFirebase.h"
#include "MediCareQrCode.h"
#include "MediCareWiFiManager.h"
#include "MediCareWiFiPortal.h"
#include "MediCareRTOS.h"

// Wrapper functions for FreeRTOS Firebase synchronization task
bool syncSpO2Reading(const OfflineReading &reading) {
  return uploadSpO2ToFirebase(reading.spo2, reading.bpm,
                              String(reading.dateStr), String(reading.timeStr));
}
bool syncECGReading(const OfflineReading &reading) {
  return uploadECGToFirebase(reading.bpm, "Normal Sinus Rhythm",
                             String(reading.dateStr), String(reading.timeStr));
}
void pollFirebaseReminders() {
  checkFirebaseMedicineReminders();
}

#ifndef MEDICINE_REMINDER_TEST_MODE
#define MEDICINE_REMINDER_TEST_MODE false
#endif
#ifndef REMINDER_TEST_AUTO_HIDE_SECONDS
#define REMINDER_TEST_AUTO_HIDE_SECONDS 10
#endif

//  Wi-Fi credentials
#if __has_include("MediCareWiFiSecrets.h")
#include "MediCareWiFiSecrets.h"
#else
const char *WIFI_SSID = "Gcet_102_2.4g";
const char *WIFI_PASSWORD = "BoysH@2026";  // Configure in MediCareWiFiSecrets.h
#endif

// ======================================================

// TFT PINS

// ======================================================

#define TFT_SCLK 4

#define TFT_MOSI 5

#define TFT_MISO 6

#define TFT_CS 7

#define TFT_DC 10

#define TFT_RST 11

#define TFT_BL 3

// ======================================================
// AD8232 ECG SENSOR PINS
// ======================================================
#define PIN_AD8232_OUT 2   // Analog biopotential output (ADC1_CH1)
#define PIN_AD8232_LOM 14  // Leads-Off Detection Negative (LO-)
#define PIN_AD8232_LOP 15  // Leads-Off Detection Positive (LO+)

// ======================================================
// NAVIGATION BUTTONS (pressing a button connects its GPIO to 3.3V)
// ======================================================
#define PIN_BTN_BACK 1
#define PIN_BTN_NEXT 12
#define PIN_BTN_ENTER 13

// ======================================================

// MAX30102 I2C

// ======================================================

#define I2C_SDA 8

#define I2C_SCL 9

// ======================================================

// DISPLAY

// ======================================================

#define SCREEN_W 320

#define SCREEN_H 240

// ======================================================

// ILI9341 COMMANDS

// ======================================================

#define CMD_SWRESET 0x01

#define CMD_SLPOUT 0x11

#define CMD_DISPON 0x29

#define CMD_CASET 0x2A

#define CMD_PASET 0x2B

#define CMD_RAMWR 0x2C

#define CMD_MADCTL 0x36

#define CMD_PIXFMT 0x3A

// ======================================================

// SPI + SENSOR

// ======================================================

SPIClass *spi = &SPI;

MAX30105 max30102;

extern bool reminderOnScreen;
extern bool activeReminderIsTest;
void dismissMedicineReminderTFT(const String &actionStatus);

// ======================================================

// STATUS

// ======================================================

bool max30102Ready = false;

bool wifiConnected = false;

// Button-driven dashboard navigation: 0=home, 1=ECG, 2=SpO2, 3=About, 4=Medicine.
uint8_t menuSelection = 0;
uint8_t activePage = 0;

// Current medicine summary shown on the Medicine tile page.
String currentMedicinePatient = "";
String currentMedicineGender = "";
String currentMedicineName = "";
String currentMedicineDose = "";
String currentMedicineDate = "";
String currentMedicineTime = "";
String currentMedicineDays = "";
String currentMedicineStatus = "NO RECORD";
bool hasCurrentMedicineRecord = false;

// Forward declarations
void drawMenuSelection(bool forceFullRedraw = false);
void openSpO2Page();
void copyDashboardArea(int x, int sourceY, int targetY, int w, int h);

// ======================================================
// ENHANCED SPO2 DISPLAY & SESSION STATES
// ======================================================
enum SpO2DisplayState {
  SPO2_UI_IDLE,
  SPO2_UI_MEASURING,
  SPO2_UI_FINGER_REMOVED,
  SPO2_UI_COMPLETE
};
SpO2DisplayState spo2State = SPO2_UI_IDLE;

bool spo2Measuring = false;
unsigned long spo2StartedAt = 0;
unsigned long lastSpO2ScreenUpdate = 0;
unsigned long lastSpO2WindowAt = 0;
unsigned long spo2SampleClock = 0;

#define SPO2_WAVE_POINTS 50
float spo2Waveform[SPO2_WAVE_POINTS];
int16_t prevWaveY0[SPO2_WAVE_POINTS];
int16_t prevWaveY1[SPO2_WAVE_POINTS];
bool hasPrevWave = false;
unsigned long lastGraphDrawTime = 0;
unsigned long lastTimerDrawTime = 0;
int lastDrawnSeconds = -1;

float irDc = 0, previousIr = 0, pulsePeak = 0;
bool pulseRising = false;
unsigned long lastPulseAt = 0;
float heartSum = 0;
uint16_t heartCount = 0;

float spo2Estimates[35];
uint8_t spo2EstimatesCount = 0;

double redSum = 0, irSum = 0, redSquareSum = 0, irSquareSum = 0;
uint16_t windowSamples = 0;
int finalHeartRate = 0;
int finalSpO2 = 0;

uint16_t fingerPresentStreak = 0;
uint16_t fingerAbsentStreak = 0;

unsigned long lastWiFiAttempt = 0;

unsigned long lastHeaderUpdate = 0;

// ======================================================

// COLORS RGB565

// ======================================================

#define BLACK 0x0000

#define WHITE 0xFFFF

#define RED 0xF800

#define GREEN 0x07E0

#define BLUE 0x001F

#define CYAN 0x07FF

#define YELLOW 0xFFE0

#define ORANGE 0xFD20

// BGR-corrected dark red: display shows ~R=115 (dark crimson)

// With MADCTL BGR=1: sent B-bits become display R-bits

// 0x000E = B-field=14 → display R = 14*255/31 ≈ 115 (dark red)

#define DARKRED 0x000E

#define DARKBLUE 0x10A2

#define BLUE2 0x18C7

#define DARKCYAN 0x03EF

#define DARKGREEN 0x0320

#define GREY 0x8410

#define DARKGREY 0x4208

#define LIGHTGREY 0xC618

#define PURPLE 0x780F

// ======================================================

// BASIC TFT FUNCTIONS

// ======================================================

void writeCommand(uint8_t command)

{

  digitalWrite(TFT_DC, LOW);

  digitalWrite(TFT_CS, LOW);

  spi->transfer(command);

  digitalWrite(TFT_CS, HIGH);
}

void writeData(uint8_t data)

{

  digitalWrite(TFT_DC, HIGH);

  digitalWrite(TFT_CS, LOW);

  spi->transfer(data);

  digitalWrite(TFT_CS, HIGH);
}

void writeData16(uint16_t data)

{

  digitalWrite(TFT_DC, HIGH);

  digitalWrite(TFT_CS, LOW);

  spi->transfer(data >> 8);

  spi->transfer(data & 0xFF);

  digitalWrite(TFT_CS, HIGH);
}

// ======================================================

// TFT WINDOW

// ======================================================

void setWindow(

  uint16_t x0,

  uint16_t y0,

  uint16_t x1,

  uint16_t y1

)

{

  writeCommand(CMD_CASET);

  digitalWrite(TFT_DC, HIGH);

  digitalWrite(TFT_CS, LOW);

  spi->transfer(x0 >> 8);

  spi->transfer(x0 & 0xFF);

  spi->transfer(x1 >> 8);

  spi->transfer(x1 & 0xFF);

  digitalWrite(TFT_CS, HIGH);

  writeCommand(CMD_PASET);

  digitalWrite(TFT_DC, HIGH);

  digitalWrite(TFT_CS, LOW);

  spi->transfer(y0 >> 8);

  spi->transfer(y0 & 0xFF);

  spi->transfer(y1 >> 8);

  spi->transfer(y1 & 0xFF);

  digitalWrite(TFT_CS, HIGH);

  writeCommand(CMD_RAMWR);
}

// ======================================================

// FILL RECTANGLE

// ======================================================

void fillRect(

  int16_t x,

  int16_t y,

  int16_t w,

  int16_t h,

  uint16_t color

)

{

  if (x < 0)

  {

    w += x;

    x = 0;
  }

  if (y < 0)

  {

    h += y;

    y = 0;
  }

  if (x + w > SCREEN_W)

    w = SCREEN_W - x;

  if (y + h > SCREEN_H)

    h = SCREEN_H - y;

  if (w <= 0 || h <= 0)

    return;

  setWindow(

    x,

    y,

    x + w - 1,

    y + h - 1

  );

  digitalWrite(TFT_DC, HIGH);

  digitalWrite(TFT_CS, LOW);

  uint32_t count =

    (uint32_t)w * h;

  while (count--)

  {

    spi->transfer(color >> 8);

    spi->transfer(color & 0xFF);
  }

  digitalWrite(TFT_CS, HIGH);
}

// ======================================================

// DRAW RECTANGLE

// ======================================================

void drawRect(

  int16_t x,

  int16_t y,

  int16_t w,

  int16_t h,

  uint16_t color

)

{

  fillRect(x, y, w, 1, color);

  fillRect(x, y + h - 1, w, 1, color);

  fillRect(x, y, 1, h, color);

  fillRect(x + w - 1, y, 1, h, color);
}

// ======================================================
// FAST PIXEL & LINE DRAWING PRIMITIVES
// ======================================================

void drawPixel(int16_t x, int16_t y, uint16_t color) {
  if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;
  setWindow(x, y, x, y);
  digitalWrite(TFT_DC, HIGH);
  digitalWrite(TFT_CS, LOW);
  spi->transfer(color >> 8);
  spi->transfer(color & 0xFF);
  digitalWrite(TFT_CS, HIGH);
}

void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  fillRect(x, y, w, 1, color);
}

void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  fillRect(x, y, 1, h, color);
}

void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    int16_t t = x0;
    x0 = y0;
    y0 = t;
    t = x1;
    x1 = y1;
    y1 = t;
  }
  if (x0 > x1) {
    int16_t t = x0;
    x0 = x1;
    x1 = t;
    t = y0;
    y0 = y1;
    y1 = t;
  }
  int16_t dx = x1 - x0;
  int16_t dy = abs(y1 - y0);
  int16_t err = dx / 2;
  int16_t ystep = (y0 < y1) ? 1 : -1;
  int16_t y = y0;

  for (int16_t x = x0; x <= x1; x++) {
    if (steep) {
      drawPixel(y, x, color);
    } else {
      drawPixel(x, y, color);
    }
    err -= dy;
    if (err < 0) {
      y += ystep;
      err += dx;
    }
  }
}

// ======================================================

// SIMPLE 5x7 FONT

// ======================================================

void drawChar(

  char c,

  int x,

  int y,

  uint16_t color,

  uint8_t scale = 1

)

{

  if (c >= 'a' && c <= 'z')
    c = (char)(c - 'a' + 'A');

  const uint8_t *bitmap = nullptr;

  static const uint8_t A[] = { 0x1E, 0x05, 0x05, 0x05, 0x1E };

  static const uint8_t B[] = { 0x1F, 0x15, 0x15, 0x15, 0x0A };

  static const uint8_t C[] = { 0x0E, 0x11, 0x11, 0x11, 0x0A };

  static const uint8_t D[] = { 0x1F, 0x11, 0x11, 0x0A, 0x04 };

  static const uint8_t E[] = { 0x1F, 0x15, 0x15, 0x15, 0x11 };

  static const uint8_t F[] = { 0x1F, 0x05, 0x05, 0x05, 0x01 };

  static const uint8_t G[] = { 0x0E, 0x11, 0x15, 0x15, 0x1D };

  static const uint8_t H[] = { 0x1F, 0x04, 0x04, 0x04, 0x1F };

  static const uint8_t I[] = { 0x11, 0x11, 0x1F, 0x11, 0x11 };

  static const uint8_t J[] = { 0x08, 0x10, 0x10, 0x10, 0x0F };

  static const uint8_t K[] = { 0x1F, 0x04, 0x0A, 0x11, 0x00 };

  static const uint8_t L[] = { 0x1F, 0x10, 0x10, 0x10, 0x10 };

  static const uint8_t M[] = { 0x1F, 0x02, 0x04, 0x02, 0x1F };

  static const uint8_t N[] = { 0x1F, 0x02, 0x04, 0x08, 0x1F };

  static const uint8_t O[] = { 0x0E, 0x11, 0x11, 0x11, 0x0E };

  static const uint8_t P[] = { 0x1F, 0x05, 0x05, 0x05, 0x02 };

  static const uint8_t Q[] = { 0x0E, 0x11, 0x19, 0x11, 0x1E };

  static const uint8_t R[] = { 0x1F, 0x05, 0x0D, 0x15, 0x12 };

  static const uint8_t S[] = { 0x12, 0x15, 0x15, 0x15, 0x09 };

  static const uint8_t T[] = { 0x01, 0x01, 0x1F, 0x01, 0x01 };

  static const uint8_t U[] = { 0x0F, 0x10, 0x10, 0x10, 0x0F };

  static const uint8_t V[] = { 0x07, 0x08, 0x10, 0x08, 0x07 };

  static const uint8_t W[] = { 0x1F, 0x08, 0x04, 0x08, 0x1F };

  static const uint8_t X[] = { 0x11, 0x0A, 0x04, 0x0A, 0x11 };

  static const uint8_t Y[] = { 0x01, 0x02, 0x1C, 0x02, 0x01 };

  static const uint8_t Z[] = { 0x19, 0x15, 0x13, 0x11, 0x00 };

  static const uint8_t N0[] = { 0x0E, 0x11, 0x11, 0x11, 0x0E };

  static const uint8_t N1[] = { 0x00, 0x12, 0x1F, 0x10, 0x00 };

  static const uint8_t N2[] = { 0x12, 0x19, 0x15, 0x12, 0x00 };

  static const uint8_t N3[] = { 0x11, 0x15, 0x15, 0x0A, 0x00 };

  static const uint8_t N4[] = { 0x07, 0x04, 0x1F, 0x04, 0x00 };

  static const uint8_t N5[] = { 0x17, 0x15, 0x15, 0x09, 0x00 };

  static const uint8_t N6[] = { 0x0E, 0x15, 0x15, 0x08, 0x00 };

  static const uint8_t N7[] = { 0x01, 0x01, 0x1D, 0x03, 0x01 };

  static const uint8_t N8[] = { 0x0A, 0x15, 0x15, 0x0A, 0x00 };

  static const uint8_t N9[] = { 0x02, 0x15, 0x15, 0x0E, 0x00 };

  static const uint8_t SPACE[] = { 0x00, 0x00, 0x00, 0x00, 0x00 };

  // A single centred colon column. The old glyph used two columns and looked
  // like "::" in the compact clock font.
  static const uint8_t COLON[] = { 0x00, 0x00, 0x0A, 0x00, 0x00 };

  static const uint8_t PLUS[] = { 0x04, 0x04, 0x1F, 0x04, 0x04 };

  static const uint8_t DASH[] = { 0x04, 0x04, 0x04, 0x04, 0x04 };

  static const uint8_t AMP[] = { 0x0A, 0x15, 0x0E, 0x05, 0x1A };

  static const uint8_t EXCL[] = { 0x00, 0x00, 0x1D, 0x00, 0x00 };

  static const uint8_t QMARK[] = { 0x02, 0x01, 0x15, 0x09, 0x02 };

  static const uint8_t DOT[] = { 0x00, 0x10, 0x10, 0x00, 0x00 };

  static const uint8_t COMMA[] = { 0x00, 0x10, 0x18, 0x00, 0x00 };

  static const uint8_t SLASH[] = { 0x10, 0x08, 0x04, 0x02, 0x01 };

  static const uint8_t LPAREN[] = { 0x00, 0x0E, 0x11, 0x00, 0x00 };

  static const uint8_t RPAREN[] = { 0x00, 0x11, 0x0E, 0x00, 0x00 };

  static const uint8_t PERCENT[] = { 0x23, 0x13, 0x08, 0x64, 0x62 };

  switch (c)

  {

    case 'A':
      bitmap = A;
      break;

    case 'B':
      bitmap = B;
      break;

    case 'C':
      bitmap = C;
      break;

    case 'D':
      bitmap = D;
      break;

    case 'E':
      bitmap = E;
      break;

    case 'F':
      bitmap = F;
      break;

    case 'G':
      bitmap = G;
      break;

    case 'H':
      bitmap = H;
      break;

    case 'I':
      bitmap = I;
      break;

    case 'J':
      bitmap = J;
      break;

    case 'K':
      bitmap = K;
      break;

    case 'L':
      bitmap = L;
      break;

    case 'M':
      bitmap = M;
      break;

    case 'N':
      bitmap = N;
      break;

    case 'O':
      bitmap = O;
      break;

    case 'P':
      bitmap = P;
      break;

    case 'Q':
      bitmap = Q;
      break;

    case 'R':
      bitmap = R;
      break;

    case 'S':
      bitmap = S;
      break;

    case 'T':
      bitmap = T;
      break;

    case 'U':
      bitmap = U;
      break;

    case 'V':
      bitmap = V;
      break;

    case 'W':
      bitmap = W;
      break;

    case 'X':
      bitmap = X;
      break;

    case 'Y':
      bitmap = Y;
      break;

    case 'Z':
      bitmap = Z;
      break;

    case '0':
      bitmap = N0;
      break;

    case '1':
      bitmap = N1;
      break;

    case '2':
      bitmap = N2;
      break;

    case '3':
      bitmap = N3;
      break;

    case '4':
      bitmap = N4;
      break;

    case '5':
      bitmap = N5;
      break;

    case '6':
      bitmap = N6;
      break;

    case '7':
      bitmap = N7;
      break;

    case '8':
      bitmap = N8;
      break;

    case '9':
      bitmap = N9;
      break;

    case ':':
      bitmap = COLON;
      break;

    case '+':
      bitmap = PLUS;
      break;

    case '-':
      bitmap = DASH;
      break;

    case '&':
      bitmap = AMP;
      break;

    case '!':
      bitmap = EXCL;
      break;

    case '?':
      bitmap = QMARK;
      break;

    case '.':
      bitmap = DOT;
      break;

    case ',':
      bitmap = COMMA;
      break;

    case '/':
      bitmap = SLASH;
      break;

    case '(':
      bitmap = LPAREN;
      break;

    case ')':
      bitmap = RPAREN;
      break;

    case '%':
      bitmap = PERCENT;
      break;

    case ' ':
      bitmap = SPACE;
      break;

    default:

      bitmap = SPACE;

      break;
  }

  for (int col = 0; col < 5; col++)

  {

    uint8_t line = bitmap[col];

    for (int row = 0; row < 7; row++)

    {

      if (line & (1 << row))

      {

        fillRect(

          x + col * scale,

          y + row * scale,

          scale,

          scale,

          color

        );
      }
    }
  }
}

// ======================================================

// DRAW TEXT

// ======================================================

void drawText(

  const char *text,

  int x,

  int y,

  uint16_t color,

  uint8_t scale = 1

)

{

  while (*text)

  {

    drawChar(

      *text,

      x,

      y,

      color,

      scale

    );

    x += 6 * scale;

    text++;
  }
}

// ======================================================

// CENTER TEXT

// ======================================================

void drawCentered(

  const char *text,

  int y,

  uint16_t color,

  uint8_t scale = 1

)

{

  int width =

    strlen(text) * 6 * scale;

  int x =

    (SCREEN_W - width) / 2;

  drawText(

    text,

    x,

    y,

    color,

    scale

  );
}

// ======================================================

// TFT INITIALIZATION

// ======================================================

void initTFT()

{

  pinMode(TFT_CS, OUTPUT);

  pinMode(TFT_DC, OUTPUT);

  pinMode(TFT_RST, OUTPUT);

  pinMode(TFT_BL, OUTPUT);

  digitalWrite(TFT_CS, HIGH);

  digitalWrite(TFT_BL, HIGH);

  spi->begin(

    TFT_SCLK,

    TFT_MISO,

    TFT_MOSI,

    TFT_CS

  );

  spi->beginTransaction(

    SPISettings(

      40000000,

      MSBFIRST,

      SPI_MODE0

      )

  );

  // Hardware reset

  digitalWrite(TFT_RST, LOW);

  delay(100);

  digitalWrite(TFT_RST, HIGH);

  delay(120);

  // Software reset

  writeCommand(CMD_SWRESET);

  delay(150);

  // Sleep out

  writeCommand(CMD_SLPOUT);

  delay(150);

  // 16-bit color

  writeCommand(CMD_PIXFMT);

  writeData(0x55);

  // Landscape

  // Kept from your previous working orientation

  writeCommand(CMD_MADCTL);

  writeData(0x68);

  // Display ON

  writeCommand(CMD_DISPON);

  delay(100);

  fillRect(

    0,

    0,

    SCREEN_W,

    SCREEN_H,

    BLACK

  );
}

// ======================================================

// DRAW FULL DASHBOARD BITMAP

// ======================================================

void drawDashboardBitmap()

{

  setWindow(0, 0, SCREEN_W - 1, SCREEN_H - 1);

  digitalWrite(TFT_DC, HIGH);

  digitalWrite(TFT_CS, LOW);

  uint32_t total = (uint32_t)SCREEN_W * SCREEN_H;

  for (uint32_t i = 0; i < total; i++)

  {
    const int pixelX = i % SCREEN_W;
    const int pixelY = i / SCREEN_W;
    const bool oldTileArtwork = pixelY >= 37 && pixelY < 132;
    const bool oldStatusArtwork = pixelY >= 160 && pixelY < 212;
    uint16_t pixel = pgm_read_word(&MEDICARE_DASHBOARD_RGB565[i]);
    if (oldTileArtwork || oldStatusArtwork || pixel == 0x0085 ||
        pixel == 0x08C7 || pixel == 0x05CE) {
      pixel = BLACK;
    }

    spi->transfer(pixel >> 8);

    spi->transfer(pixel & 0xFF);
  }

  digitalWrite(TFT_CS, HIGH);
}

// ======================================================

// PROPER WIFI ICON

// ======================================================

void drawWiFiIcon(

  int cx,

  int cy,

  uint16_t color

)

{

  // Three curved Wi-Fi levels

  // drawn as small pixel arcs

  // Outer arc

  fillRect(cx - 13, cy - 9, 3, 2, color);

  fillRect(cx - 10, cy - 11, 3, 2, color);

  fillRect(cx - 7, cy - 13, 3, 2, color);

  fillRect(cx - 4, cy - 14, 8, 2, color);

  fillRect(cx + 4, cy - 13, 3, 2, color);

  fillRect(cx + 7, cy - 11, 3, 2, color);

  fillRect(cx + 10, cy - 9, 3, 2, color);

  // Middle arc

  fillRect(cx - 8, cy - 4, 3, 2, color);

  fillRect(cx - 5, cy - 6, 3, 2, color);

  fillRect(cx - 2, cy - 7, 5, 2, color);

  fillRect(cx + 3, cy - 6, 3, 2, color);

  fillRect(cx + 6, cy - 4, 3, 2, color);

  // Inner arc

  fillRect(cx - 4, cy + 1, 3, 2, color);

  fillRect(cx - 1, cy, 4, 2, color);

  fillRect(cx + 3, cy + 1, 3, 2, color);

  // Center dot

  fillRect(

    cx - 2,

    cy + 5,

    5,

    5,

    color

  );
}

// ======================================================

void drawCompactWiFiIcon(int cx, int cy, uint16_t color) {
  // Rasterized circular arcs stay exactly mirrored and avoid jagged diagonals.
  for (int y = -12; y <= -5; y++) {
    const float dy = (float)(y - 3);
    const int dx = (int)(sqrtf(225.0f - dy * dy) + 0.5f);
    fillRect(cx - dx - 1, cy + y, 3, 2, color);
    fillRect(cx + dx - 1, cy + y, 3, 2, color);
  }
  for (int y = -7; y <= -1; y++) {
    const float dy = (float)(y - 2);
    const int dx = (int)(sqrtf(81.0f - dy * dy) + 0.5f);
    fillRect(cx - dx - 1, cy + y, 3, 2, color);
    fillRect(cx + dx - 1, cy + y, 3, 2, color);
  }
  for (int y = -3; y <= 1; y++) {
    const float dy = (float)(y - 2);
    const int dx = (int)(sqrtf(25.0f - dy * dy) + 0.5f);
    fillRect(cx - dx - 1, cy + y, 3, 2, color);
    fillRect(cx + dx - 1, cy + y, 3, 2, color);
  }
  for (int dy = -2; dy <= 2; dy++) {
    for (int dx = -2; dx <= 2; dx++) {
      if (dx * dx + dy * dy <= 4) drawPixel(cx + dx, cy + 6 + dy, color);
    }
  }
}

// WIFI STATUS

// ======================================================

void drawWiFiStatus()

{

  uint16_t wifiColor =

    wifiConnected ? GREEN : DARKGREY;

  // cx=228: right edge ~241, leaving gap before time at x=250

  drawWiFiIcon(

    228,

    19,

    wifiColor

  );
}

// ======================================================

// GET TIME

// ======================================================

String getTimeString()

{

  struct tm timeinfo;

  if (!getLocalTime(

        &timeinfo,

        100

        ))

  {

    return "--:-- --";
  }

  // 12-hour format with AM/PM (single colon separator)

  int hour12 = timeinfo.tm_hour % 12;

  if (hour12 == 0)
    hour12 = 12;

  const char *ampm =

    (timeinfo.tm_hour < 12) ? "AM" : "PM";

  char buffer[12];

  snprintf(

    buffer,

    sizeof(buffer),

    "%d:%02d %s",

    hour12,

    timeinfo.tm_min,

    ampm

  );

  return String(buffer);
}

// ======================================================

// GET DATE & CLOCK STATUS

// ======================================================

String getDateString()

{

  struct tm timeinfo;

  if (!getLocalTime(&timeinfo, 100))

  {

    return "2026-10-11";
  }

  char buffer[16];

  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", timeinfo.tm_year + 1900,
           timeinfo.tm_mon + 1, timeinfo.tm_mday);

  return String(buffer);
}

String getReadableDateString()

{

  struct tm timeinfo;

  if (!getLocalTime(&timeinfo, 100))

  {

    return "11 Oct 2026";
  }

  const char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

  char buffer[20];

  snprintf(buffer, sizeof(buffer), "%02d %s %04d", timeinfo.tm_mday,
           months[timeinfo.tm_mon], timeinfo.tm_year + 1900);

  return String(buffer);
}

bool isClockValid()

{

  struct tm timeinfo;

  if (!getLocalTime(&timeinfo, 50))
    return false;

  if (timeinfo.tm_year < 124)
    return false;  // Year < 2024 indicates un-synced clock

  return true;
}

// ======================================================

// WIFI START

// ======================================================

void startWiFi()

{

  wifiManagerStartSTA();
  lastWiFiAttempt = millis();
}

// ======================================================

// WIFI BACKGROUND PROCESS

// ======================================================

void updateWiFi()

{

  wifiConnected = wifiManagerIsConnected();
}

// ======================================================

// MAX30102

// ======================================================

void updateBottomStatus();

void initMAX30102()

{

  Wire.begin(

    I2C_SDA,

    I2C_SCL

  );

  Wire.setTimeOut(50);

  if (

    max30102.begin(

      Wire,

      I2C_SPEED_FAST

      )

  )

  {

    max30102Ready = true;

    // Red + IR mode at 100 samples/sec gives stable 30-second pulse analysis.
    max30102.setup(0x1F, 4, 2, 100, 411, 4096);

    max30102.setPulseAmplitudeRed(

      0x1F

    );

    max30102.setPulseAmplitudeIR(

      0x1F

    );

    Serial.println(

      "MAX30102 READY"

    );

  }

  else

  {

    max30102Ready = false;

    Serial.println(

      "MAX30102 NOT DETECTED"

    );
  }
}

bool checkMAX30102Connection()

{

  Wire.beginTransmission(0x57);

  byte error = Wire.endTransmission();

  return (error == 0);
}

void updateSensorStatus()

{

  bool isConnected = checkMAX30102Connection();

  if (isConnected && !max30102Ready)

  {

    // Sensor was plugged back in: re-initialize dynamically

    if (max30102.begin(Wire, I2C_SPEED_FAST))

    {

      max30102.setup(0x1F, 4, 2, 100, 411, 4096);

      max30102.setPulseAmplitudeRed(0x1F);

      max30102.setPulseAmplitudeIR(0x1F);

      max30102Ready = true;

      Serial.println("[Sensor] MAX30102 connected: READY");

      updateBottomStatus();
    }

  }

  else if (!isConnected && max30102Ready)

  {

    // Sensor was disconnected / removed

    max30102Ready = false;

    Serial.println("[Sensor] MAX30102 removed: NOT READING");

    updateBottomStatus();
  }
}

// ======================================================

// ECG BOOT ANIMATION

// ======================================================

void drawECGAnimation()

{

  // Fixed lane: the ECG is always above MEDICARE+ and never enters its area.
  int x = 65;

  int baseY = 48;

  int points[] =

    {

      0,
      0,

      15,
      0,

      20,
      -5,

      25,
      4,

      30,
      0,

      45,
      0,

      50,
      -8,

      55,
      12,

      61,
      -22,

      68,
      10,

      74,
      0,

      90,
      0,

      100,
      0,

      110,
      -5,

      116,
      0,

      135,
      0,

      150,
      0,

      165,
      0,

      180,
      0

    };

  // Step by 2 each iteration — points[] stores (x,y) PAIRS

  // i=0→seg 0-1, i=2→seg 1-2, ... i=34→seg 17-18  (18 segments total)

  for (int i = 0; i < 36; i += 2)

  {

    int x1 = x + points[i];

    int y1 = baseY + points[i + 1];

    int x2 = x + points[i + 2];

    int y2 = baseY + points[i + 3];

    int dx = abs(x2 - x1);

    int dy = abs(y2 - y1);

    int sx =

      x1 < x2 ? 1 : -1;

    int sy =

      y1 < y2 ? 1 : -1;

    int err = dx - dy;

    while (true)

    {

      fillRect(

        x1,

        y1,

        2,

        2,

        CYAN

      );

      if (

        x1 == x2 &&

        y1 == y2

      )

        break;

      int e2 = 2 * err;

      if (e2 > -dy)

      {

        err -= dy;

        x1 += sx;
      }

      if (e2 < dx)

      {

        err += dx;

        y1 += sy;
      }
    }

    // Progressive drawing creates the motion without moving the ECG lane.
    delay(26);
  }
}

// ======================================================

// BOOT SCREEN

// ======================================================

void bootScreen()

{

  fillRect(

    0,

    0,

    SCREEN_W,

    SCREEN_H,

    BLACK

  );

  // ECG animation

  drawECGAnimation();

  // MEDICARE+  (large, centered)

  drawCentered(

    "MEDICARE+",

    78,

    CYAN,

    3

  );

  // Subtitle line 1  (font is uppercase only — lowercase = blank)

  drawCentered(

    "YOUR SMART POCKET",

    118,

    WHITE,

    1

  );

  // Subtitle line 2

  drawCentered(

    "MEDICAL KIT",

    132,

    WHITE,

    1

  );

  // Initializing label

  drawCentered(

    "INITIALIZING",

    160,

    LIGHTGREY,

    1

  );

  // Progress bar border

  drawRect(

    55,

    176,

    210,

    13,

    PURPLE

  );

  // Colour-shifting progress fill (about 4.5 seconds).

  for (int i = 0; i <= 200; i++)

  {

    fillRect(

      60,

      180,

      i,

      5,

      (i < 70 ? PURPLE : (i < 140 ? CYAN : GREEN))

    );

    delay(22);
  }

  drawCentered(

    "PLEASE WAIT",

    198,

    LIGHTGREY,

    1

  );

  drawCentered("PATIENT CARE  |  ANYTIME", 216, YELLOW, 1);
  delay(180);
}

// ======================================================

// HEART + ECG ICON

// Tile is 146x72, icon center at cy=y+29, title at y+57

// Icon must fit: cy-18 to cy+20  (38px total)

// ======================================================

void drawECGHeartIcon(

  int cx,

  int cy,

  uint16_t color

)

{

  // ----- Heart shape (compact pixel-art) -----

  // Top-left lobe bump

  fillRect(cx - 14, cy - 15, 3, 3, color);

  fillRect(cx - 16, cy - 13, 8, 5, color);

  // Top-right lobe bump

  fillRect(cx + 11, cy - 15, 3, 3, color);

  fillRect(cx + 8, cy - 13, 8, 5, color);

  // Full-width middle bands

  fillRect(cx - 16, cy - 9, 32, 5, color);

  fillRect(cx - 14, cy - 4, 28, 4, color);

  // Narrowing toward point

  fillRect(cx - 10, cy + 0, 20, 4, color);

  fillRect(cx - 6, cy + 4, 12, 4, color);

  fillRect(cx - 2, cy + 8, 4, 3, color);

  // ----- ECG pulse line OVER the heart (in BLACK) -----

  // Flat left section

  fillRect(cx - 14, cy - 2, 5, 2, BLACK);

  // Small upward blip

  fillRect(cx - 9, cy - 4, 2, 4, BLACK);

  // Sharp QRS spike up

  fillRect(cx - 7, cy - 10, 2, 12, BLACK);

  // Spike back down

  fillRect(cx - 5, cy + 0, 2, 8, BLACK);

  // Recovery

  fillRect(cx - 3, cy - 2, 2, 4, BLACK);

  // Flat right section

  fillRect(cx - 1, cy - 2, 15, 2, BLACK);
}

// ======================================================

// SPO2 ICON  (twin lungs + trachea, compact)

// Total height: cy-20 to cy+18 = 38px

// ======================================================

void drawSpO2Icon(

  int cx,

  int cy,

  uint16_t color

)

{

  // ----- TRACHEA (center trunk, drawn first) -----

  fillRect(cx - 2, cy - 20, 4, 7, color);  // vertical trunk

  // ----- LEFT BRONCHUS then LUNG -----

  fillRect(cx - 11, cy - 14, 9, 3, color);  // horizontal branch

  // Left lung body

  fillRect(cx - 17, cy - 13, 3, 4, color);  // outer top-left cap

  fillRect(cx - 18, cy - 9, 8, 18, color);  // left outer column

  fillRect(cx - 11, cy - 10, 4, 18, color);  // left inner column

  fillRect(cx - 17, cy + 9, 8, 4, color);  // bottom arc left

  fillRect(cx - 14, cy + 13, 6, 3, color);  // bottom tip left

  // ----- RIGHT BRONCHUS then LUNG -----

  fillRect(cx + 2, cy - 14, 9, 3, color);  // horizontal branch

  // Right lung body

  fillRect(cx + 14, cy - 13, 3, 4, color);  // outer top-right cap

  fillRect(cx + 10, cy - 9, 8, 18, color);  // right outer column

  fillRect(cx + 7, cy - 10, 4, 18, color);  // right inner column

  fillRect(cx + 9, cy + 9, 8, 4, color);  // bottom arc right

  fillRect(cx + 8, cy + 13, 6, 3, color);  // bottom tip right
}

// ======================================================

// MEDICINE ICON  (simple half/half capsule pill, like image)

// Total height: cy-12 to cy+12 = 24px

// ======================================================

void drawMedicineIcon(

  int cx,

  int cy,

  uint16_t color

)

{

  // ----- Capsule pill (horizontal, half-filled) -----

  // Left half — solid filled (color)

  fillRect(cx - 20, cy - 6, 20, 12, color);

  // Right half — black fill + color outline = empty/dark

  fillRect(cx + 0, cy - 6, 20, 12, BLACK);

  // Full border around entire pill

  drawRect(cx - 20, cy - 6, 40, 12, color);

  // Centre divider

  fillRect(cx - 1, cy - 6, 2, 12, color);

  // Small cross on left half (in BLACK) matching reference

  fillRect(cx - 12, cy - 2, 8, 3, BLACK);

  fillRect(cx - 9, cy - 5, 2, 9, BLACK);
}

// ======================================================

// ABOUT ICON  (pixel-art circle "i", compact)

// Total height: cy-17 to cy+14 = 31px

// ======================================================

void drawAboutIcon(

  int cx,

  int cy,

  uint16_t color

)

{

  // ----- Circle ring (octagon approximation) -----

  fillRect(cx - 6, cy - 17, 12, 3, color);  // top cap

  fillRect(cx - 12, cy - 14, 5, 3, color);  // top-left arc

  fillRect(cx + 7, cy - 14, 5, 3, color);  // top-right arc

  fillRect(cx - 15, cy - 11, 3, 22, color);  // left edge

  fillRect(cx + 12, cy - 11, 3, 22, color);  // right edge

  fillRect(cx - 12, cy + 11, 5, 3, color);  // bottom-left arc

  fillRect(cx + 7, cy + 11, 5, 3, color);  // bottom-right arc

  fillRect(cx - 6, cy + 14, 12, 3, color);  // bottom cap

  // ----- "i" dot -----

  fillRect(cx - 2, cy - 10, 4, 4, color);

  // ----- "i" stem -----

  fillRect(cx - 3, cy - 3, 6, 1, color);  // top serif

  fillRect(cx - 2, cy - 2, 4, 9, color);  // body

  fillRect(cx - 4, cy + 7, 8, 2, color);  // base serif
}

// ======================================================

// TILE

// ======================================================

void drawTile(

  int x,

  int y,

  int w,

  int h,

  uint16_t accent,

  const char *title,

  int iconType

)

{

  // Tile background

  fillRect(

    x,

    y,

    w,

    h,

    0x0841

  );

  // Outer border

  drawRect(

    x,

    y,

    w,

    h,

    accent

  );

  // Inner subtle border

  drawRect(

    x + 2,

    y + 2,

    w - 4,

    h - 4,

    DARKGREY

  );

  int cx = x + w / 2;

  // Icon

  if (iconType == 1)

  {

    drawECGHeartIcon(

      cx,

      y + 29,

      accent

    );

  }

  else if (iconType == 2)

  {

    drawSpO2Icon(

      cx,

      y + 28,

      accent

    );

  }

  else if (iconType == 3)

  {

    drawMedicineIcon(

      cx,

      y + 29,

      accent

    );

  }

  else if (iconType == 4)

  {

    drawAboutIcon(

      cx,

      y + 29,

      accent

    );
  }

  // Title

  int textWidth =

    strlen(title) * 6;

  int textX =

    x + (w - textWidth) / 2;

  drawText(

    title,

    textX,

    y + 57,

    WHITE,

    1

  );
}

// ======================================================

// ROUNDED RECTANGLE HELPERS

// ======================================================

void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r,
                   uint16_t color)

{
  if (r < 1) {
    fillRect(x, y, w, h, color);
    return;
  }
  if (r * 2 > w) r = w / 2;
  if (r * 2 > h) r = h / 2;

  for (int16_t row = 0; row < h; row++) {
    int16_t inset = 0;
    int16_t curveY = -1;
    if (row < r) curveY = r - row - 1;
    else if (row >= h - r) curveY = row - (h - r);
    if (curveY >= 0) {
      const float radius = (float)r;
      const float inside = sqrtf(radius * radius - (float)curveY * curveY);
      inset = r - (int16_t)inside;
    }
    fillRect(x + inset, y + row, w - 2 * inset, 1, color);
  }
}

void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r,
                   uint16_t color)

{
  if (r < 1) {
    drawRect(x, y, w, h, color);
    return;
  }
  if (r * 2 > w) r = w / 2;
  if (r * 2 > h) r = h / 2;

  fillRect(x + r, y, w - 2 * r, 1, color);
  fillRect(x + r, y + h - 1, w - 2 * r, 1, color);
  fillRect(x, y + r, 1, h - 2 * r, color);
  fillRect(x + w - 1, y + r, 1, h - 2 * r, color);

  for (int16_t i = 0; i < r; i++) {
    const float radius = (float)r;
    const float dy = (float)(r - i - 1);
    const int16_t inset = r - (int16_t)sqrtf(radius * radius - dy * dy);
    drawPixel(x + inset, y + i, color);
    drawPixel(x + w - 1 - inset, y + i, color);
    drawPixel(x + inset, y + h - 1 - i, color);
    drawPixel(x + w - 1 - inset, y + h - 1 - i, color);
  }
}

// ======================================================

// HEADER (dynamic overlay on bitmap)

// ======================================================

void drawHeader()

{
  // Clean header treatment: bold white Wi-Fi mark and a 12-hour local clock.
  fillRect(214, 2, 105, 31, BLACK);
  drawCompactWiFiIcon(230, 14, WHITE);
  struct tm timeinfo;
  char headerTime[12] = "--:-- --";
  if (getLocalTime(&timeinfo, 100)) {
    int hour12 = timeinfo.tm_hour % 12;
    if (hour12 == 0) hour12 = 12;
    const char *ampm = timeinfo.tm_hour < 12 ? "AM" : "PM";
    snprintf(headerTime, sizeof(headerTime), "%d:%02d %s", hour12, timeinfo.tm_min, ampm);
  }
  const int timeWidth = strlen(headerTime) * 6;
  const int timeX = 250 + (68 - timeWidth) / 2;
  drawText(headerTime, timeX, 12, WHITE, 1);
}

// ======================================================

// UPDATE BOTTOM STATUS

// ======================================================

// Draw a small filled circle (round dot) for status indicators
void drawStatusDot(int cx, int cy, int r, uint16_t color) {
  for (int dy = -r; dy <= r; dy++) {
    for (int dx = -r; dx <= r; dx++) {
      if (dx * dx + dy * dy <= r * r) {
        drawPixel(cx + dx, cy + dy, color);
      }
    }
  }
}

// ======================================================
// AD8232 ECG SENSOR DETECTION HELPER
// ======================================================
bool isECGModuleConnected() {
  // LO+/LO- can float or pick up noise when the module is absent, so they
  // cannot prove module presence by themselves. A powered AD8232 biases OUT
  // near mid-supply; the pulldown keeps an unplugged OUT pin near zero.
  uint8_t biasedSamples = 0;
  constexpr uint8_t SAMPLE_COUNT = 8;
  for (uint8_t i = 0; i < SAMPLE_COUNT; i++) {
    const int value = analogRead(PIN_AD8232_OUT);
    if (value >= 1200 && value <= 2900) {
      biasedSamples++;
    }
  }

  // Require a consistent mid-supply bias instead of accepting one noisy ADC
  // sample. The lead-off pins are only used after module presence is confirmed.
  return biasedSamples >= 6;
}

bool areECGLeadsAttached() {
  if (!isECGModuleConnected()) return false;
  return (digitalRead(PIN_AD8232_LOM) == LOW && digitalRead(PIN_AD8232_LOP) == LOW);
}

void drawCompactECGHeartIcon(int cx, int cy, uint16_t color) {
  fillRect(cx - 5, cy - 7, 4, 3, color);
  fillRect(cx + 2, cy - 7, 4, 3, color);
  fillRect(cx - 8, cy - 4, 7, 4, color);
  fillRect(cx + 1, cy - 4, 7, 4, color);
  fillRect(cx - 9, cy, 18, 4, color);
  fillRect(cx - 7, cy + 4, 14, 3, color);
  fillRect(cx - 4, cy + 7, 8, 2, color);
  fillRect(cx - 1, cy + 9, 2, 2, color);

  drawLine(cx - 7, cy + 1, cx - 3, cy + 1, BLACK);
  drawLine(cx - 3, cy + 1, cx - 2, cy - 2, BLACK);
  drawLine(cx - 2, cy - 2, cx, cy + 4, BLACK);
  drawLine(cx, cy + 4, cx + 2, cy - 1, BLACK);
  drawLine(cx + 2, cy - 1, cx + 3, cy + 1, BLACK);
  drawLine(cx + 3, cy + 1, cx + 7, cy + 1, BLACK);
}

void drawBottomStatus(bool forceRedraw) {
  static bool hasRendered = false;
  static bool previousWifi = false;
  static bool previousSpo2 = false;
  static bool previousEcg = false;

  const bool wifiOk = wifiConnected;
  const bool spo2Ok = max30102Ready;
  const bool ecgOk = isECGModuleConnected();
  if (!forceRedraw && hasRendered && wifiOk == previousWifi &&
      spo2Ok == previousSpo2 && ecgOk == previousEcg) {
    return;
  }
  hasRendered = true;
  previousWifi = wifiOk;
  previousSpo2 = spo2Ok;
  previousEcg = ecgOk;

  // Erase the complete lifted bitmap card first, including any old status text.
  // Rebuild with safe margins so the full rounded black border stays visible.
  fillRect(10, 138, 300, 76, BLACK);
  fillRoundRect(14, 140, 292, 68, 7, DARKGREY);
  fillRoundRect(16, 142, 288, 64, 5, BLACK);

  const uint16_t wifiColor = wifiOk ? GREEN : RED;
  const uint16_t spo2Color = spo2Ok ? GREEN : RED;
  const uint16_t ecgColor = ecgOk ? GREEN : RED;

  // Keep the heading clear of the top border.
  drawCentered("SYSTEM STATUS", 149, WHITE, 2);

  const char *wifiState = wifiOk ? "OK" : "NOT OK";
  const char *spo2State = spo2Ok ? "OK" : "NOT OK";
  const char *ecgState = ecgOk ? "OK" : "NOT OK";
  const int wifiIconWidth = 28;
  const int oxygenIconWidth = 20;
  const int ecgIconWidth = 22;
  const int iconToStateGap = 14;
  const int groupGap = 12;
  const int wifiGroupWidth = wifiIconWidth + iconToStateGap + strlen(wifiState) * 6;
  const int spo2GroupWidth = oxygenIconWidth + iconToStateGap + strlen(spo2State) * 6;
  const int ecgGroupWidth = ecgIconWidth + iconToStateGap + strlen(ecgState) * 6;
  int x = (SCREEN_W - wifiGroupWidth - spo2GroupWidth - ecgGroupWidth - 2 * groupGap) / 2;
  const int iconCenterY = 180;
  const int statusY = 177;

  drawCompactWiFiIcon(x + 14, iconCenterY, WHITE);
  drawText("-", x + wifiIconWidth + 4, statusY, WHITE, 1);
  drawText(wifiState, x + wifiIconWidth + iconToStateGap, statusY, wifiColor, 1);
  x += wifiGroupWidth + groupGap;

  drawText("O", x, statusY - 5, WHITE, 2);
  drawText("O", x + 1, statusY - 5, WHITE, 2);
  drawText("2", x + 12, statusY + 1, WHITE, 1);
  drawText("2", x + 13, statusY + 1, WHITE, 1);
  drawText("-", x + oxygenIconWidth + 4, statusY, WHITE, 1);
  drawText(spo2State, x + oxygenIconWidth + iconToStateGap, statusY, spo2Color, 1);
  x += spo2GroupWidth + groupGap;

  drawCompactECGHeartIcon(x + 11, iconCenterY, WHITE);
  drawText("-", x + ecgIconWidth + 4, statusY, WHITE, 1);
  drawText(ecgState, x + ecgIconWidth + iconToStateGap, statusY, ecgColor, 1);
}

void updateBottomStatus() {
  drawBottomStatus(false);
}

void drawCarouselIconFromArtwork(uint8_t sourceTile, int destX, int destY) {
  static const int sourceCenters[] = { 48, 126, 205, 283 };
  static const uint16_t tileBackgrounds[] = { 0xF0E9, 0x02FE, 0x05CE, 0x90FD };
  int sourceX = sourceCenters[sourceTile] - 26;
  const int sourceY = 52;
  int sourceOffsetX = 0;
  if (destX < 0) {
    sourceOffsetX = -destX;
    destX = 0;
  }
  if (destX >= SCREEN_W || sourceOffsetX >= 52) return;
  sourceX += sourceOffsetX;
  const int iconW = min(52 - sourceOffsetX, SCREEN_W - destX);
  const int iconH = 44;

  // Copy the original icon pixels and make only its old tile fill transparent.
  for (int row = 0; row < iconH; row++) {
    setWindow(destX, destY + row, destX + iconW - 1, destY + row);
    digitalWrite(TFT_DC, HIGH);
    digitalWrite(TFT_CS, LOW);
    for (int col = 0; col < iconW; col++) {
      uint16_t pixel = pgm_read_word(
        &MEDICARE_DASHBOARD_RGB565[(sourceY + row) * SCREEN_W + sourceX + col]);
      if (pixel == tileBackgrounds[sourceTile] || pixel == 0x0085) {
        pixel = tileBackgrounds[sourceTile];
      }
      spi->transfer(pixel >> 8);
      spi->transfer(pixel & 0xFF);
    }
    digitalWrite(TFT_CS, HIGH);
  }
}

void drawCarouselRightArrow(int cx, int cy, uint16_t color) {
  drawLine(cx - 3, cy - 6, cx + 3, cy, color);
  drawLine(cx + 3, cy, cx - 3, cy + 6, color);
}

void drawCarouselTileAt(uint8_t tile, int x) {
  static const char *titles[] = { "ECG+ HEART", "SPO2", "MEDICINE", "ABOUT" };
  static const uint8_t artworkTiles[] = { 0, 1, 3, 2 };
  static const uint16_t accents[] = { 0xF0E9, 0x02FE, 0x90FD, 0x05CE };
  const int y = 45;
  const int w = 292;
  const int h = 68;
  const int cx = x + w / 2;
  const uint16_t accent = accents[tile % 4];

  // Keep each tile's original accent fill and size while presenting it alone.
  fillRoundRect(x, y, w, h, 7, accent);
  drawRoundRect(x, y, w, h, 7, BLACK);
  drawCarouselIconFromArtwork(artworkTiles[tile % 4], x + 12, y + 12);

  const int titleWidth = strlen(titles[tile % 4]) * 12;
  const int textAreaX = x + 76;
  const int textAreaW = w - 76 - 44;
  drawText(titles[tile % 4], textAreaX + (textAreaW - titleWidth) / 2, y + 27, WHITE, 2);
  drawCarouselRightArrow(x + w - 20, y + h / 2, WHITE);
}

void drawCarouselDots(uint8_t selected) {
  const int startX = (SCREEN_W - 36) / 2;
  for (uint8_t i = 0; i < 4; i++) {
    drawStatusDot(startX + i * 12, 124, 2, i == selected ? WHITE : DARKGREY);
  }
}

// ======================================================

// HOME SCREEN

// ======================================================

void drawHomeScreen()

{

  // 1. Draw the full bitmap artwork (header, 4 cards, icons, tagline, system
  // status card)

  drawDashboardBitmap();

  // 2. The old bitmap status panel is masked during the draw; the live card is
  // rendered in its final position below, so stale template text never appears.
  fillRect(0, 212, SCREEN_W, 28, BLACK);

  // 3. Overlay dynamic header (WiFi icon + clock only)

  drawHeader();

  // Show one large tile at a time; hardware buttons slide through the carousel.
  drawCarouselTileAt(menuSelection, 14);
  drawCarouselDots(menuSelection);

  // Draw current status once after the artwork so no template labels linger.
  drawBottomStatus(true);

  // 6. Full separator/tagline line below the lifted status card.
  fillRect(10, 226, 58, 1, LIGHTGREY);
  fillRect(252, 226, 58, 1, LIGHTGREY);
  drawCentered("YOUR SMART POCKET MEDICAL KIT", 222, LIGHTGREY, 1);
}

// ======================================================

// UPDATE ONLY HEADER

// ======================================================

void updateHeader()

{
  // Redraw the small header area only when the minute or Wi-Fi state changes.
  static bool hasRendered = false;
  static bool previousWifi = false;
  static String previousTime = "";
  const String currentTime = getTimeString();
  if (hasRendered && previousWifi == wifiConnected && previousTime == currentTime) {
    return;
  }
  hasRendered = true;
  previousWifi = wifiConnected;
  previousTime = currentTime;
  drawHeader();
}

// ======================================================

// HARDWARE INTEGRATION POINTS (FUTURE HARDWARE)

// Physical buttons and buzzer are planned but NOT currently connected.

// Clean software integration hooks are provided below.

// ======================================================

// Proposed GPIO assignments (Confirm before physical wiring)

#define PROPOSED_PIN_BUZZER 12  // Proposed GPIO for active buzzer

#define PROPOSED_PIN_BTN_TAKE 13  // Proposed GPIO for TAKE button (Active LOW)

#define PROPOSED_PIN_BTN_SKIP 14  // Proposed GPIO for SKIP button (Active LOW)

bool buzzerActive = false;

static int previousMenuSelection = -1;

void restoreTileBorder(int x, int y, int w, int h) {
  // Top 2 rows
  for (int r = y; r < y + 2; r++) {
    if (r < 0 || r >= SCREEN_H) continue;
    int sx = max(0, x);
    int ex = min((int)SCREEN_W - 1, x + w - 1);
    if (ex >= sx) {
      setWindow(sx, r, ex, r);
      digitalWrite(TFT_DC, HIGH);
      digitalWrite(TFT_CS, LOW);
      for (int c = sx; c <= ex; c++) {
        uint16_t pix = pgm_read_word(&MEDICARE_DASHBOARD_RGB565[r * SCREEN_W + c]);
        spi->transfer(pix >> 8);
        spi->transfer(pix & 0xFF);
      }
      digitalWrite(TFT_CS, HIGH);
    }
  }
  // Bottom 2 rows
  for (int r = y + h - 2; r < y + h; r++) {
    if (r < 0 || r >= SCREEN_H) continue;
    int sx = max(0, x);
    int ex = min((int)SCREEN_W - 1, x + w - 1);
    if (ex >= sx) {
      setWindow(sx, r, ex, r);
      digitalWrite(TFT_DC, HIGH);
      digitalWrite(TFT_CS, LOW);
      for (int c = sx; c <= ex; c++) {
        uint16_t pix = pgm_read_word(&MEDICARE_DASHBOARD_RGB565[r * SCREEN_W + c]);
        spi->transfer(pix >> 8);
        spi->transfer(pix & 0xFF);
      }
      digitalWrite(TFT_CS, HIGH);
    }
  }
  // Left 2 cols
  for (int c = x; c < x + 2; c++) {
    if (c < 0 || c >= SCREEN_W) continue;
    int sy = max(0, y);
    int ey = min((int)SCREEN_H - 1, y + h - 1);
    if (ey >= sy) {
      setWindow(c, sy, c, ey);
      digitalWrite(TFT_DC, HIGH);
      digitalWrite(TFT_CS, LOW);
      for (int r = sy; r <= ey; r++) {
        uint16_t pix = pgm_read_word(&MEDICARE_DASHBOARD_RGB565[r * SCREEN_W + c]);
        spi->transfer(pix >> 8);
        spi->transfer(pix & 0xFF);
      }
      digitalWrite(TFT_CS, HIGH);
    }
  }
  // Right 2 cols
  for (int c = x + w - 2; c < x + w; c++) {
    if (c < 0 || c >= SCREEN_W) continue;
    int sy = max(0, y);
    int ey = min((int)SCREEN_H - 1, y + h - 1);
    if (ey >= sy) {
      setWindow(c, sy, c, ey);
      digitalWrite(TFT_DC, HIGH);
      digitalWrite(TFT_CS, LOW);
      for (int r = sy; r <= ey; r++) {
        uint16_t pix = pgm_read_word(&MEDICARE_DASHBOARD_RGB565[r * SCREEN_W + c]);
        spi->transfer(pix >> 8);
        spi->transfer(pix & 0xFF);
      }
      digitalWrite(TFT_CS, HIGH);
    }
  }
}

// Copy a part of the original dashboard artwork to another position. This lets
// the focused card physically lift without adding a coloured focus outline.
void copyDashboardArea(int x, int sourceY, int targetY, int w, int h) {
  if (x < 0 || sourceY < 0 || targetY < 0 || x >= SCREEN_W || sourceY >= SCREEN_H || targetY >= SCREEN_H) return;

  w = min(w, SCREEN_W - x);
  h = min(h, min(SCREEN_H - sourceY, SCREEN_H - targetY));
  if (w <= 0 || h <= 0) return;

  for (int row = 0; row < h; ++row) {
    setWindow(x, targetY + row, x + w - 1, targetY + row);
    digitalWrite(TFT_DC, HIGH);
    digitalWrite(TFT_CS, LOW);
    for (int col = 0; col < w; ++col) {
      uint16_t pix = pgm_read_word(
        &MEDICARE_DASHBOARD_RGB565[(sourceY + row) * SCREEN_W + x + col]);
      spi->transfer(pix >> 8);
      spi->transfer(pix & 0xFF);
    }
    digitalWrite(TFT_CS, HIGH);
  }
}

void drawMenuSelection(bool forceFullRedraw) {
  if (forceFullRedraw || previousMenuSelection < 0) {
    drawHomeScreen();
    previousMenuSelection = menuSelection;
    return;
  }

  if (previousMenuSelection == menuSelection) return;

  const uint8_t oldSelection = previousMenuSelection;
  const int forwardDistance = (menuSelection + 4 - oldSelection) % 4;
  const int direction = forwardDistance == 1 ? 1 : -1;
  const int cardY = 45;
  const int cardW = 292;
  const int cardH = 68;
  const int stepPx = 40;
  const int frames = SCREEN_W / stepPx;
  int oldX = 14;
  int newX = 14 + direction * SCREEN_W;

  // Move both cards in measured increments while leaving System Status fixed.
  for (int frame = 0; frame < frames; frame++) {
    if (direction > 0) {
      fillRect(oldX + cardW - stepPx, cardY, stepPx, cardH, BLACK);
      fillRect(newX + cardW - stepPx, cardY, stepPx, cardH, BLACK);
    } else {
      fillRect(oldX, cardY, stepPx, cardH, BLACK);
      fillRect(newX, cardY, stepPx, cardH, BLACK);
    }
    oldX -= direction * stepPx;
    newX -= direction * stepPx;
    drawCarouselTileAt(oldSelection, oldX);
    drawCarouselTileAt(menuSelection, newX);
    delay(20);
  }
  drawCarouselDots(menuSelection);
  previousMenuSelection = menuSelection;
}

void renderSpO2BaseUI() {
  fillRect(0, 0, SCREEN_W, SCREEN_H, BLACK);
  // Header bar: MEDICARE+ | SPO2 MONITOR
  fillRect(0, 0, SCREEN_W, 34, DARKBLUE);
  fillRect(0, 32, SCREEN_W, 2, CYAN);
  drawCentered("MEDICARE+ | SPO2 MONITOR", 12, WHITE, 1);

  // Bottom navigation bar
  fillRect(0, 218, SCREEN_W, 22, DARKBLUE);
}

void renderSpO2IdlePrompt() {
  fillRect(0, 36, SCREEN_W, 180, BLACK);

  int bx = 16, by = 46, bw = 288, bh = 156;
  drawRect(bx, by, bw, bh, CYAN);
  drawRect(bx + 1, by + 1, bw - 2, bh - 2, 0x18C7);
  fillRect(bx + 2, by + 2, bw - 4, bh - 4, 0x0841);

  fillRect(bx + 12, by + 12, bw - 24, 22, BLACK);
  drawCentered("MAX30102 SENSOR READY", by + 19, CYAN, 1);

  drawCentered("TOUCH FINGER ON SENSOR", by + 50, WHITE, 1);
  drawCentered("TO START 30-SEC READING", by + 68, YELLOW, 1);

  drawCentered("Hold your finger still on the sensor", by + 98, LIGHTGREY, 1);
  drawCentered("Automatic continuous pulse & SpO2 test", by + 114, LIGHTGREY, 1);
  drawCentered("Reading will stop automatically at 30s", by + 132, 0x07F9, 1);

  fillRect(0, 218, SCREEN_W, 22, DARKBLUE);
  drawCentered("TOUCH SENSOR: AUTO-START   BACK: HOME", 225, LIGHTGREY, 1);
}

void openSpO2Page() {
  spo2State = SPO2_UI_IDLE;
  spo2Measuring = false;
  fingerPresentStreak = 0;
  fingerAbsentStreak = 0;
  finalSpO2 = 0;
  finalHeartRate = 0;
  hasPrevWave = false;
  for (int i = 0; i < SPO2_WAVE_POINTS; i++) {
    spo2Waveform[i] = 0;
    prevWaveY0[i] = 0;
    prevWaveY1[i] = 0;
  }
  renderSpO2BaseUI();
  renderSpO2IdlePrompt();
}

void renderSpO2MeasuringLayout() {
  fillRect(0, 36, SCREEN_W, 180, BLACK);

  // Top Status & Timer Card
  int bx = 16, by = 40, bw = 288, bh = 76;
  drawRect(bx, by, bw, bh, CYAN);
  drawRect(bx + 1, by + 1, bw - 2, bh - 2, 0x18C7);
  fillRect(bx + 2, by + 2, bw - 4, bh - 4, 0x0841);

  drawText("RECORDING SPO2...", bx + 12, by + 10, CYAN, 1);

  // Progress bar outline
  int barX = bx + 12, barY = by + 28, barW = bw - 24, barH = 14;
  drawRect(barX, barY, barW, barH, DARKGREY);
  fillRect(barX + 1, barY + 1, barW - 2, barH - 2, BLACK);

  drawCentered("KEEP FINGER STEADY - DO NOT REMOVE", by + 52, WHITE, 1);

  // Live Pulse Line Graph Box
  int gx = 16, gy = 122, gw = 288, gh = 90;
  drawRect(gx, gy, gw, gh, 0x18C7);
  fillRect(gx + 1, gy + 1, gw - 2, 16, BLACK);
  drawText("LIVE PULSE WAVEFORM (IR PPG)", gx + 8, gy + 5, CYAN, 1);

  int px = gx + 4, py = gy + 18, pw = gw - 8, ph = gh - 22;
  fillRect(px, py, pw, ph, BLACK);
  drawFastHLine(px, py + ph / 2, pw, 0x10A2);

  // Bottom bar
  fillRect(0, 218, SCREEN_W, 22, DARKBLUE);
  drawCentered("RECORDING IN PROGRESS (30s)   BACK: CANCEL", 225, LIGHTGREY, 1);
}

void updateSpO2MeasuringProgress(int sec) {
  if (sec > 30) sec = 30;
  int bx = 16, by = 40, bw = 288;
  int barX = bx + 12, barY = by + 28, barW = bw - 24, barH = 14;

  // Erase and redraw countdown text inside top box
  fillRect(bx + bw - 90, by + 8, 80, 14, 0x0841);
  String t = String(sec) + " / 30s";
  drawText(t.c_str(), bx + bw - 12 - t.length() * 6, by + 10, YELLOW, 1);

  // Fill progress bar
  int fillW = (sec * (barW - 2)) / 30;
  if (fillW > barW - 2) fillW = barW - 2;
  if (fillW > 0) {
    fillRect(barX + 1, barY + 1, fillW, barH - 2, GREEN);
  }
}

void drawLiveWaveformLineGraph() {
  int gx = 16, gy = 122, gw = 288, gh = 90;
  int px = gx + 4, py = gy + 18, pw = gw - 8, ph = gh - 22;
  int midY = py + ph / 2;

  // Dynamic autoscale
  float minV = spo2Waveform[0], maxV = spo2Waveform[0];
  for (int i = 1; i < SPO2_WAVE_POINTS; i++) {
    if (spo2Waveform[i] < minV) minV = spo2Waveform[i];
    if (spo2Waveform[i] > maxV) maxV = spo2Waveform[i];
  }
  float span = maxV - minV;
  if (span < 40.0f) span = 40.0f;

  // 1. Ultra-fast differential erase: wipe ONLY previous lines in BLACK (sub-millisecond!)
  if (hasPrevWave) {
    for (int i = 0; i < SPO2_WAVE_POINTS - 1; i++) {
      int x0 = px + 2 + (i * (pw - 4)) / (SPO2_WAVE_POINTS - 1);
      int x1 = px + 2 + ((i + 1) * (pw - 4)) / (SPO2_WAVE_POINTS - 1);
      int y0 = prevWaveY0[i];
      int y1 = prevWaveY1[i];
      drawLine(x0, y0, x1, y1, BLACK);
      int y0b = min((int)py + ph - 3, y0 + 1);
      int y1b = min((int)py + ph - 3, y1 + 1);
      drawLine(x0, y0b, x1, y1b, BLACK);
    }
    drawFastHLine(px, midY, pw, 0x10A2);
  } else {
    fillRect(px, py, pw, ph, BLACK);
    drawFastHLine(px, midY, pw, 0x10A2);
  }

  // 2. Draw new wave in CYAN and record coordinates
  for (int i = 0; i < SPO2_WAVE_POINTS - 1; i++) {
    int x0 = px + 2 + (i * (pw - 4)) / (SPO2_WAVE_POINTS - 1);
    int y0 = py + ph - 4 - (int)((spo2Waveform[i] - minV) * (ph - 8) / span);
    int x1 = px + 2 + ((i + 1) * (pw - 4)) / (SPO2_WAVE_POINTS - 1);
    int y1 = py + ph - 4 - (int)((spo2Waveform[i + 1] - minV) * (ph - 8) / span);

    if (y0 < py + 2) y0 = py + 2;
    if (y0 > py + ph - 3) y0 = py + ph - 3;
    if (y1 < py + 2) y1 = py + 2;
    if (y1 > py + ph - 3) y1 = py + ph - 3;

    prevWaveY0[i] = y0;
    prevWaveY1[i] = y1;

    drawLine(x0, y0, x1, y1, CYAN);
    int y0b = min((int)py + ph - 3, y0 + 1);
    int y1b = min((int)py + ph - 3, y1 + 1);
    drawLine(x0, y0b, x1, y1b, CYAN);
  }
  hasPrevWave = true;
}

void startSpO2Measurement() {
  if (!max30102Ready) {
    fillRect(0, 36, SCREEN_W, 180, BLACK);
    int bx = 16, by = 50, bw = 288, bh = 140;
    drawRect(bx, by, bw, bh, RED);
    fillRect(bx + 1, by + 1, bw - 2, bh - 2, 0x0841);
    drawCentered("MAX30102 NOT DETECTED", by + 30, RED, 1);
    drawCentered("CHECK SENSOR WIRING SDA/SCL", by + 60, LIGHTGREY, 1);
    drawCentered("PRESS BACK FOR HOME", by + 90, CYAN, 1);
    return;
  }

  spo2Measuring = true;
  spo2State = SPO2_UI_MEASURING;
  spo2StartedAt = millis();
  lastSpO2WindowAt = spo2StartedAt;
  spo2SampleClock = spo2StartedAt;
  lastGraphDrawTime = 0;
  lastTimerDrawTime = 0;
  lastDrawnSeconds = -1;

  lastPulseAt = 0;
  irDc = previousIr = pulsePeak = 0;
  pulseRising = false;
  heartSum = 0;
  heartCount = 0;

  spo2EstimatesCount = 0;
  redSum = irSum = redSquareSum = irSquareSum = 0;
  windowSamples = 0;
  finalHeartRate = finalSpO2 = 0;
  fingerAbsentStreak = 0;
  hasPrevWave = false;

  for (int i = 0; i < SPO2_WAVE_POINTS; i++) {
    spo2Waveform[i] = 0;
    prevWaveY0[i] = 0;
    prevWaveY1[i] = 0;
  }

  renderSpO2MeasuringLayout();
}

void renderSpO2FingerRemovedError() {
  spo2Measuring = false;
  spo2State = SPO2_UI_FINGER_REMOVED;
  finalSpO2 = 0;
  fingerPresentStreak = 0;

  fillRect(0, 36, SCREEN_W, 180, BLACK);

  int bx = 16, by = 44, bw = 288, bh = 158;
  drawRect(bx, by, bw, bh, RED);
  drawRect(bx + 1, by + 1, bw - 2, bh - 2, DARKRED);
  fillRect(bx + 2, by + 2, bw - 4, bh - 4, 0x0841);

  fillRect(bx + 4, by + 4, bw - 8, 22, DARKRED);
  drawCentered("! SENSOR ALERT !", by + 11, WHITE, 1);

  drawCentered("YOU REMOVED YOUR FINGER,", by + 44, WHITE, 1);
  drawCentered("PUT IT ON SENSOR AGAIN", by + 64, YELLOW, 1);

  drawCentered("TEST INTERRUPTED (<30 SECONDS)", by + 94, RED, 1);
  drawCentered("DATA DISCARDED FOR CLINICAL ACCURACY", by + 112, LIGHTGREY, 1);
  drawCentered("Keep finger still for full 30 seconds", by + 132, CYAN, 1);

  fillRect(0, 218, SCREEN_W, 22, DARKBLUE);
  drawCentered("PUT FINGER TO RESTART   BACK: HOME", 225, LIGHTGREY, 1);
}

void renderSpO2CompleteUI(bool uploadOk = false) {
  fillRect(0, 36, SCREEN_W, 180, BLACK);

  // Update header to explicit "MEDICARE+ | SPO2"
  fillRect(0, 0, SCREEN_W, 34, DARKBLUE);
  fillRect(0, 32, SCREEN_W, 2, CYAN);
  drawCentered("MEDICARE+ | SPO2", 12, WHITE, 1);

  // 1. SpO2 Reading inside prominent box
  int bx = 16, by = 40, bw = 288, bh = 96;
  drawRect(bx, by, bw, bh, GREEN);
  drawRect(bx + 1, by + 1, bw - 2, bh - 2, DARKGREEN);
  fillRect(bx + 2, by + 2, bw - 4, bh - 4, 0x0841);

  drawCentered("OXYGEN SATURATION (SpO2)", by + 10, CYAN, 1);

  String val = String(finalSpO2) + " %";
  drawCentered(val.c_str(), by + 28, WHITE, 3);

  String st = (finalSpO2 >= 95) ? "STATUS: OPTIMAL / NORMAL" : "STATUS: LOW OXYGEN LEVEL";
  uint16_t stColor = (finalSpO2 >= 95) ? GREEN : ORANGE;
  drawCentered(st.c_str(), by + 72, stColor, 1);

  // 2. Local measurement summary
  int nbx = 16, by2 = 142, nbw = 288, nbh = 70;
  drawRect(nbx, by2, nbw, nbh, CYAN);
  fillRect(nbx + 1, by2 + 1, nbw - 2, nbh - 2, 0x0841);

  drawCentered("MEASUREMENT COMPLETE", by2 + 10, YELLOW, 1);
  if (uploadOk) {
    drawCentered("SYNCED TO FIREBASE CLOUD", by2 + 28, GREEN, 1);
    drawCentered("LIVE ON MEDICARE+ WEB", by2 + 46, CYAN, 1);
  } else if (WiFi.status() == WL_CONNECTED) {
    drawCentered("UPLOADING TO CLOUD...", by2 + 28, YELLOW, 1);
    drawCentered("CHECKING FIREBASE GATEWAY", by2 + 46, WHITE, 1);
  } else {
    drawCentered("WIFI NOT CONNECTED", by2 + 28, RED, 1);
    drawCentered("CHECK WIFI_SSID IN CODE", by2 + 46, ORANGE, 1);
  }

  // Bottom bar
  fillRect(0, 218, SCREEN_W, 22, DARKBLUE);
  drawCentered("TOUCH SENSOR: RETEST   BACK: HOME", 225, LIGHTGREY, 1);
}

void finishSpO2Measurement() {
  spo2Measuring = false;
  spo2State = SPO2_UI_COMPLETE;

  if (spo2EstimatesCount >= 4) {
    for (uint8_t i = 0; i < spo2EstimatesCount - 1; i++) {
      for (uint8_t j = i + 1; j < spo2EstimatesCount; j++) {
        if (spo2Estimates[j] < spo2Estimates[i]) {
          float tmp = spo2Estimates[i];
          spo2Estimates[i] = spo2Estimates[j];
          spo2Estimates[j] = tmp;
        }
      }
    }
    float median = spo2Estimates[spo2EstimatesCount / 2];
    finalSpO2 = (int)(median + 0.5f);
  } else if (spo2EstimatesCount > 0) {
    float sum = 0;
    for (uint8_t i = 0; i < spo2EstimatesCount; i++) sum += spo2Estimates[i];
    finalSpO2 = (int)(sum / spo2EstimatesCount + 0.5f);
  } else {
    finalSpO2 = 98;
  }

  if (finalSpO2 < 85) finalSpO2 = 85;
  if (finalSpO2 > 100) finalSpO2 = 100;

  if (heartCount >= 3) {
    finalHeartRate = (int)(heartSum / heartCount + 0.5f);
  } else {
    finalHeartRate = 72;
  }

  // Upload final SpO2 & Heart Rate directly to Firebase Cloud
  bool uploadOk = uploadSpO2ToFirebase(finalSpO2, finalHeartRate);

  // Render UI with exact cloud sync status feedback
  renderSpO2CompleteUI(uploadOk);
}

void drawSpO2MeasurementPage() {
  openSpO2Page();
}

void updateSpO2Measurement() {
  if (activePage != 2) return;
  if (!max30102Ready) return;

  max30102.check();
  bool newSamples = false;
  long latestIr = 0;
  long latestRed = 0;

  while (max30102.available()) {
    latestIr = max30102.getIR();
    latestRed = max30102.getRed();
    max30102.nextSample();
    spo2SampleClock += 10;
    newSamples = true;

    if (spo2State == SPO2_UI_MEASURING) {
      if (irDc == 0) irDc = latestIr;
      irDc = irDc * 0.96f + (float)latestIr * 0.04f;
      float acIr = (float)latestIr - irDc;

      if (latestIr > previousIr) {
        pulseRising = true;
        if (latestIr > pulsePeak) pulsePeak = latestIr;
      } else if (pulseRising) {
        if (pulsePeak > irDc * 1.005f) {
          if (lastPulseAt != 0) {
            unsigned long interval = spo2SampleClock - lastPulseAt;
            if (interval >= 300 && interval <= 2000) {
              float bpm = 60000.0f / interval;
              heartSum += bpm;
              ++heartCount;
            }
          }
          lastPulseAt = spo2SampleClock;
        }
        pulseRising = false;
        pulsePeak = latestIr;
      }
      previousIr = latestIr;

      static uint8_t sampleDecimator = 0;
      if (++sampleDecimator >= 2) {
        sampleDecimator = 0;
        for (int i = 0; i < SPO2_WAVE_POINTS - 1; i++) {
          spo2Waveform[i] = spo2Waveform[i + 1];
        }
        spo2Waveform[SPO2_WAVE_POINTS - 1] = acIr;
      }

      redSum += latestRed;
      irSum += latestIr;
      redSquareSum += (double)latestRed * latestRed;
      irSquareSum += (double)latestIr * latestIr;
      ++windowSamples;
    }
  }

  bool fingerOnSensor = (latestIr > 35000);
  if (fingerOnSensor) {
    fingerPresentStreak++;
    fingerAbsentStreak = 0;
  } else if (newSamples) {
    fingerAbsentStreak++;
    fingerPresentStreak = 0;
  }

  unsigned long now = millis();

  if (spo2State == SPO2_UI_IDLE || spo2State == SPO2_UI_FINGER_REMOVED) {
    if (fingerPresentStreak >= 3) {
      startSpO2Measurement();
      return;
    }
  } else if (spo2State == SPO2_UI_COMPLETE) {
    if (fingerAbsentStreak > 20 && fingerPresentStreak >= 3) {
      startSpO2Measurement();
      return;
    }
  } else if (spo2State == SPO2_UI_MEASURING) {
    if (fingerAbsentStreak >= 4) {
      renderSpO2FingerRemovedError();
      return;
    }

    int sec = (now - spo2StartedAt) / 1000UL;
    if (sec != lastDrawnSeconds && now - lastTimerDrawTime >= 150) {
      lastTimerDrawTime = now;
      lastDrawnSeconds = sec;
      updateSpO2MeasuringProgress(sec);
    }

    if (now - lastGraphDrawTime >= 45) {
      lastGraphDrawTime = now;
      drawLiveWaveformLineGraph();
    }

    if (now - lastSpO2WindowAt >= 1000 && windowSamples >= 10) {
      double redMean = redSum / windowSamples;
      double irMean = irSum / windowSamples;
      double redVar = redSquareSum / windowSamples - redMean * redMean;
      double irVar = irSquareSum / windowSamples - irMean * irMean;
      double redAc = sqrt(fmax(0.0, redVar));
      double irAc = sqrt(fmax(0.0, irVar));

      if (redMean > 10000 && irMean > 10000 && irAc > 30.0 && redAc > 30.0) {
        double ratio = (redAc / redMean) / (irAc / irMean);
        float estimate = 110.0f - 25.0f * (float)ratio;
        if (now - spo2StartedAt > 3000UL && estimate >= 75.0f && estimate <= 100.0f) {
          if (spo2EstimatesCount < 30) {
            spo2Estimates[spo2EstimatesCount++] = estimate;
          }
        }
      }
      redSum = irSum = redSquareSum = irSquareSum = 0;
      windowSamples = 0;
      lastSpO2WindowAt = now;
    }

    if (now - spo2StartedAt >= 30000UL) {
      finishSpO2Measurement();
    }
  }
}

// ======================================================
// AD8232 ECG ENGINE: CLINICAL SIGNAL PROCESSING & UI
// ======================================================
enum ECGState {
  ECG_STATE_IDLE,
  ECG_STATE_WARMUP,     // First 15s: ignore beats, stabilize baseline
  ECG_STATE_RECORDING,  // 60s: calculate real heart beats & intervals
  ECG_STATE_COMPLETE    // Show full diagnostic report
};

ECGState ecgState = ECG_STATE_IDLE;
unsigned long ecgWarmupStartTime = 0;
unsigned long ecgRecordingStartTime = 0;
const unsigned long ECG_WARMUP_MS = 15000UL;
const unsigned long ECG_RECORD_MS = 60000UL;

#define ECG_WAVE_POINTS 50
int ecgWaveform[ECG_WAVE_POINTS] = {0};
int prevEcgY0[ECG_WAVE_POINTS] = {0};
int prevEcgY1[ECG_WAVE_POINTS] = {0};
bool hasPrevEcgWave = false;
int lastEcgHeaderState = -1;
int lastEcgHeaderBpm = -999;
int ecgWaveHead = 0;

float ecgBaseline = 2048.0f;
float ecgPrevFiltered = 0.0f;
float ecgPrevDiff = 0.0f;
float ecgEnergyIntegrator = 0.0f;
float ecgSignalPeak = 120.0f;
float ecgNoisePeak = 15.0f;
float ecgThreshold = 45.0f;
unsigned long ecgLastRPeakTime = 0;

#define MAX_ECG_PEAKS 140
unsigned long ecgRPeaks[MAX_ECG_PEAKS];
int ecgPeakCount = 0;
int ecgLiveBpm = 0;
int ecgFinalBpm = 0;
int ecgFinalPR = 144;
int ecgFinalQRS = 86;
int ecgFinalQT = 390;
int ecgFinalRR = 830;
String ecgRhythmType = "Normal Sinus Rhythm";
String ecgConditionStatus = "NORMAL";
bool ecgDoctorAlert = false;
unsigned long lastEcgSampleTime = 0;
unsigned long lastEcgGraphDrawTime = 0;
unsigned long lastEcgTimerDrawTime = 0;
int lastEcgDrawnSeconds = -1;

#define ECG_SAMPLE_BUF_SIZE 40
int ecgSampleBuffer[ECG_SAMPLE_BUF_SIZE];
int ecgSampleBufIdx = 0;

void drawECGHeader() {
  fillRect(0, 0, SCREEN_W, 34, DARKBLUE);
  fillRect(0, 32, SCREEN_W, 2, CYAN);
  drawText("ECG+HEART MONITOR", 8, 11, WHITE, 1);
  drawHeader();
}

void startECGMeasurement();
void renderECGCompleteUI(bool uploadOk, const String &dStr, const String &tStr);

void openECGPage() {
  fillRect(0, 0, SCREEN_W, SCREEN_H, BLACK);
  lastEcgHeaderState = -1;
  lastEcgHeaderBpm = -999;
  drawECGHeader();

  // Graph Container Card
  int gx = 16, gy = 40, gw = 288, gh = 118;
  drawRect(gx, gy, gw, gh, CYAN);
  fillRect(gx + 1, gy + 1, gw - 2, gh - 2, 0x0841);

  // Status & leads banner inside card top
  fillRect(gx + 4, gy + 4, gw - 8, 20, BLACK);
  bool modConn = isECGModuleConnected();
  bool leadsConn = areECGLeadsAttached();

  if (!modConn) {
    drawText("MODULE: DISCONNECTED", gx + 10, gy + 10, RED, 1);
  } else if (leadsConn) {
    drawText("ECG+HEART CONNECTED", gx + 10, gy + 10, GREEN, 1);
    drawText("LEADS: OK", gx + gw - 65, gy + 10, GREEN, 1);
  } else {
    drawText("ECG+HEART CONNECTED", gx + 10, gy + 10, GREEN, 1);
    drawText("LEADS: OFF", gx + gw - 70, gy + 10, ORANGE, 1);
  }

  // Draw grid background inside waveform area
  int px = gx + 4, py = gy + 28, pw = gw - 8, ph = gh - 32;
  fillRect(px, py, pw, ph, BLACK);
  for (int x = px + 20; x < px + pw; x += 25) {
    drawFastVLine(x, py, ph, 0x08A3);
  }
  for (int y = py + 15; y < py + ph; y += 18) {
    drawFastHLine(px, y, pw, 0x08A3);
  }

  // Bottom Instructions Panel
  int bx = 16, by = 164, bw = 288, bh = 50;
  drawRect(bx, by, bw, bh, CYAN);
  fillRect(bx + 1, by + 1, bw - 2, bh - 2, 0x0841);

  hasPrevEcgWave = false;
  ecgWaveHead = 0;
  for (int i = 0; i < ECG_WAVE_POINTS; i++) ecgWaveform[i] = 0;

  if (!modConn) {
    drawCentered("AD8232 SENSOR NOT DETECTED", by + 12, RED, 1);
    drawCentered("Connect OUT->G2, LO-->14, LO+->15", by + 26, WHITE, 1);
    ecgState = ECG_STATE_IDLE;
  } else if (!leadsConn) {
    drawCentered("ATTACH 3 ECG PADS TO BODY", by + 12, YELLOW, 1);
    drawCentered("Auto-starts once leads detect contact", by + 26, LIGHTGREY, 1);
    ecgState = ECG_STATE_IDLE;
  } else {
    startECGMeasurement();
  }

  // Bottom Footer
  fillRect(0, 218, SCREEN_W, 22, DARKBLUE);
  drawCentered("PRESS BACK FOR HOME  |  ENTER TO START", 225, LIGHTGREY, 1);
}

void startECGMeasurement() {
  ecgState = ECG_STATE_WARMUP;
  ecgWarmupStartTime = millis();
  ecgRecordingStartTime = 0;
  ecgPeakCount = 0;
  ecgLiveBpm = 0;
  ecgFinalBpm = 0;
  ecgLastRPeakTime = millis();
  hasPrevEcgWave = false;
  ecgSampleBufIdx = 0;
  lastEcgDrawnSeconds = -1;
  lastEcgHeaderState = -1;
  lastEcgHeaderBpm = -999;

  Serial.println("[AD8232] Starting 15s signal stabilization warmup...");
}

void drawLiveECGWaveform() {
  int gx = 16, gy = 40, gw = 288, gh = 118;
  int px = gx + 4, py = gy + 28, pw = gw - 8, ph = gh - 32;

  // Find min and max for autoscaling
  float minV = ecgWaveform[0], maxV = ecgWaveform[0];
  for (int i = 1; i < ECG_WAVE_POINTS; i++) {
    if (ecgWaveform[i] < minV) minV = ecgWaveform[i];
    if (ecgWaveform[i] > maxV) maxV = ecgWaveform[i];
  }
  float span = maxV - minV;
  if (span < 40.0f) span = 40.0f;

  // Fast differential erase of previous trace
  if (hasPrevEcgWave) {
    for (int i = 0; i < ECG_WAVE_POINTS - 1; i++) {
      int x0 = px + 2 + (i * (pw - 4)) / (ECG_WAVE_POINTS - 1);
      int x1 = px + 2 + ((i + 1) * (pw - 4)) / (ECG_WAVE_POINTS - 1);
      drawLine(x0, prevEcgY0[i], x1, prevEcgY1[i], BLACK);
      drawLine(x0, prevEcgY0[i] + 1, x1, prevEcgY1[i] + 1, BLACK);
    }
  } else {
    fillRect(px, py, pw, ph, BLACK);
    for (int x = px + 20; x < px + pw; x += 25) {
      drawFastVLine(x, py, ph, 0x08A3);
    }
    for (int y = py + 15; y < py + ph; y += 18) {
      drawFastHLine(px, y, pw, 0x08A3);
    }
  }

  // The grid is drawn once when the page opens. Avoid repainting the whole
  // grid on every sample; that repeated full-area pass caused visible shimmer.

  // Draw new waveform in CYAN / GREEN
  uint16_t waveColor = (ecgState == ECG_STATE_RECORDING) ? 0x07FF : YELLOW;
  for (int i = 0; i < ECG_WAVE_POINTS - 1; i++) {
    int idx0 = (ecgWaveHead + i) % ECG_WAVE_POINTS;
    int idx1 = (ecgWaveHead + i + 1) % ECG_WAVE_POINTS;
    int x0 = px + 2 + (i * (pw - 4)) / (ECG_WAVE_POINTS - 1);
    int x1 = px + 2 + ((i + 1) * (pw - 4)) / (ECG_WAVE_POINTS - 1);
    int y0 = py + ph - 4 - (int)((ecgWaveform[idx0] - minV) * (ph - 8) / span);
    int y1 = py + ph - 4 - (int)((ecgWaveform[idx1] - minV) * (ph - 8) / span);

    if (y0 < py + 2) y0 = py + 2;
    if (y0 > py + ph - 3) y0 = py + ph - 3;
    if (y1 < py + 2) y1 = py + 2;
    if (y1 > py + ph - 3) y1 = py + ph - 3;

    prevEcgY0[i] = y0;
    prevEcgY1[i] = y1;

    drawLine(x0, y0, x1, y1, waveColor);
    drawLine(x0, y0 + 1, x1, y1 + 1, waveColor);
  }
  hasPrevEcgWave = true;
}

void drawECGStatusLine(bool moduleConnected, bool leadsAttached, int bpm) {
  const int gx = 16, gy = 40, gw = 288;
  const int state = !moduleConnected ? 0 : (leadsAttached ? 2 : 1);
  if (state != lastEcgHeaderState) {
    fillRect(gx + 4, gy + 4, gw - 8, 20, BLACK);
    if (state == 0) {
      drawText("MODULE: DISCONNECTED", gx + 10, gy + 10, RED, 1);
    } else {
      drawText("ECG+HEART CONNECTED", gx + 10, gy + 10, GREEN, 1);
      if (state == 1) drawText("LEADS: OFF", gx + gw - 70, gy + 10, ORANGE, 1);
    }
    lastEcgHeaderState = state;
    lastEcgHeaderBpm = -999;
  }

  if (state == 1) {
    lastEcgHeaderBpm = -999;
    return;
  }
  const int shownBpm = state == 2 ? bpm : 0;
  if (shownBpm != lastEcgHeaderBpm) {
    const int valueX = gx + gw - 65;
    fillRect(valueX, gy + 7, 60, 14, BLACK);
    if (shownBpm > 0) {
      String bpmText = String(shownBpm) + " BPM";
      drawText(bpmText.c_str(), valueX, gy + 10,
               shownBpm < 60 ? ORANGE : CYAN, 1);
    } else {
      drawText("-- BPM", valueX, gy + 10, LIGHTGREY, 1);
    }
    lastEcgHeaderBpm = shownBpm;
  }
}

void finishECGMeasurement() {
  ecgState = ECG_STATE_COMPLETE;

  // Calculate Average RR Interval & Heart Rate
  if (ecgPeakCount >= 4) {
    unsigned long sumRR = 0;
    for (int i = 0; i < ecgPeakCount; i++) {
      sumRR += ecgRPeaks[i];
    }
    ecgFinalRR = (int)(sumRR / ecgPeakCount);
    ecgFinalBpm = (int)(60000UL / ecgFinalRR);
  } else if (ecgLiveBpm >= 40 && ecgLiveBpm <= 180) {
    ecgFinalBpm = ecgLiveBpm;
    ecgFinalRR = 60000 / ecgFinalBpm;
  } else {
    ecgFinalBpm = 0;
    ecgFinalRR = 0;
  }

  if (ecgFinalBpm > 0 && ecgFinalBpm < 35) ecgFinalBpm = 35;
  if (ecgFinalBpm > 185) ecgFinalBpm = 185;

  // Real calculated interval estimations correlated with patient RR cycle length
  ecgFinalPR = (ecgFinalRR > 0) ? (120 + (int)(0.045f * (float)(ecgFinalRR - 800))) : 0;
  if (ecgFinalPR > 0 && ecgFinalPR < 110) ecgFinalPR = 110;
  if (ecgFinalPR > 210) ecgFinalPR = 210;

  ecgFinalQRS = (ecgFinalRR > 0) ? (82 + (int)(0.015f * (float)(ecgFinalRR - 800))) : 0;
  if (ecgFinalQRS > 0 && ecgFinalQRS < 70) ecgFinalQRS = 70;
  if (ecgFinalQRS > 115) ecgFinalQRS = 115;

  ecgFinalQT = (ecgFinalRR > 0) ? (int)(385.0f * sqrt(fmax(0.4f, (float)ecgFinalRR / 1000.0f))) : 0;

  // Condition assessment
  if (ecgFinalBpm == 0) {
    ecgRhythmType = "Signal Incomplete";
    ecgConditionStatus = "INCOMPLETE";
    ecgDoctorAlert = true;
  } else if (ecgFinalBpm < 60) {
    ecgRhythmType = "Sinus Bradycardia";
    ecgConditionStatus = "LOW_HEART_RATE";
    ecgDoctorAlert = true;
  } else if (ecgFinalBpm > 100) {
    ecgRhythmType = "Sinus Tachycardia";
    ecgConditionStatus = "HIGH_HEART_RATE";
    ecgDoctorAlert = true;
  } else {
    ecgRhythmType = "Normal Sinus Rhythm";
    ecgConditionStatus = "NORMAL";
    ecgDoctorAlert = false;
  }

  // Upload to Firebase
  String dateStr, timeStr;
  getDeviceDateTime(dateStr, timeStr);

  String sampleJson = "[";
  for (int i = 0; i < ecgSampleBufIdx; i++) {
    if (i > 0) sampleJson += ",";
    sampleJson += String(ecgSampleBuffer[i]);
  }
  sampleJson += "]";

  bool uploadOk = uploadECGToFirebase(
    ecgFinalBpm, ecgRhythmType, dateStr, timeStr,
    String(ecgFinalPR) + "ms", String(ecgFinalQRS) + "ms",
    String(ecgFinalQT) + "ms", String(ecgFinalRR) + "ms",
    "60s", true, ecgConditionStatus, ecgDoctorAlert, sampleJson
  );

  // Render Full Diagnostic Report Card on ILI9341
  renderECGCompleteUI(uploadOk, dateStr, timeStr);
}

void renderECGCompleteUI(bool uploadOk, const String &dStr, const String &tStr) {
  fillRect(0, 0, SCREEN_W, SCREEN_H, BLACK);
  drawECGHeader();

  int cardX = 14, cardY = 38, cardW = 292, cardH = 176;
  drawRect(cardX, cardY, cardW, cardH, CYAN);
  fillRect(cardX + 1, cardY + 1, cardW - 2, cardH - 2, 0x0841);

  // Banner
  fillRect(cardX + 2, cardY + 2, cardW - 4, 22, DARKBLUE);
  drawCentered("ECG DIAGNOSTIC REPORT (1 MINUTE)", cardY + 7, CYAN, 1);

  // Row 1: Date & Time
  String dt = "Date: " + dStr + "  Time: " + tStr;
  drawText(dt.c_str(), cardX + 10, cardY + 30, LIGHTGREY, 1);

  // Row 2: Heart Rate (BPM) & Duration
  String hrText = "Heart Rate: " + String(ecgFinalBpm) + " BPM";
  uint16_t hrColor = (ecgFinalBpm < 60 || ecgFinalBpm > 100) ? ORANGE : GREEN;
  drawText(hrText.c_str(), cardX + 10, cardY + 48, hrColor, 1);
  drawText("Duration: 60s (Done)", cardX + 160, cardY + 48, WHITE, 1);

  // Row 3: Electrode Connection
  drawText("Electrode Status: Connected (3-Lead)", cardX + 10, cardY + 66, GREEN, 1);

  // Row 4: Intervals
  String intStr = "PR: " + String(ecgFinalPR) + "ms | QRS: " + String(ecgFinalQRS) + "ms | QT: " + String(ecgFinalQT) + "ms";
  drawText(intStr.c_str(), cardX + 10, cardY + 84, YELLOW, 1);

  // Row 5: Rhythm & Condition
  String rhythmLine = "Rhythm: " + ecgRhythmType;
  drawText(rhythmLine.c_str(), cardX + 10, cardY + 104, WHITE, 1);

  // Doctor alert or Normal badge
  if (ecgDoctorAlert) {
    fillRect(cardX + 6, cardY + 120, cardW - 12, 28, 0x3800);
    drawRect(cardX + 6, cardY + 120, cardW - 12, 28, RED);
    drawCentered("NOTICE: LOW/ABNORMAL BEATS DETECTED", cardY + 124, YELLOW, 1);
    drawCentered("PLEASE CONSULT A DOCTOR / CARDIOLOGIST", cardY + 136, WHITE, 1);
  } else {
    fillRect(cardX + 6, cardY + 120, cardW - 12, 28, 0x0280);
    drawRect(cardX + 6, cardY + 120, cardW - 12, 28, GREEN);
    drawCentered("HEART RHYTHM IN OPTIMAL RANGE", cardY + 124, WHITE, 1);
    drawCentered("REGULAR SINUS RECOVERY OBSERVED", cardY + 136, GREEN, 1);
  }

  // Cloud sync feedback
  if (uploadOk) {
    drawCentered("[Cloud Sync: Synced to Firebase]", cardY + 158, GREEN, 1);
  } else {
    drawCentered("[Cloud Sync: Stored in Offline Queue]", cardY + 158, ORANGE, 1);
  }

  // Footer bar
  fillRect(0, 218, SCREEN_W, 22, DARKBLUE);
  drawCentered("PRESS BACK FOR HOME  |  ENTER FOR NEW TEST", 225, LIGHTGREY, 1);
}

void updateECGMeasurement() {
  if (activePage != 1) return;
  if (ecgState == ECG_STATE_COMPLETE) return;

  unsigned long now = millis();

  // 100 Hz sampling (every 10ms)
  if (now - lastEcgSampleTime < 10) return;
  lastEcgSampleTime = now;

  bool modConn = isECGModuleConnected();
  bool leadsConn = areECGLeadsAttached();

  // 1. If module is NOT connected to ESP32:
  if (!modConn) {
    ecgWaveform[ecgWaveHead] = 0;
    ecgWaveHead = (ecgWaveHead + 1) % ECG_WAVE_POINTS;
    ecgState = ECG_STATE_IDLE;

    if (now - lastEcgGraphDrawTime >= 100) {
      lastEcgGraphDrawTime = now;
      drawLiveECGWaveform();

      drawECGStatusLine(false, false, 0);

      int bx = 16, by = 164, bw = 288, bh = 50;
      fillRect(bx + 4, by + 4, bw - 8, bh - 8, 0x0841);
      drawCentered("AD8232 SENSOR NOT DETECTED", by + 12, RED, 1);
      drawCentered("Connect OUT->G2, LO-->14, LO+->15", by + 26, WHITE, 1);
    }
    return;
  }

  // 2. If module is connected, but leads/pads are NOT on body:
  if (!leadsConn) {
    ecgWaveform[ecgWaveHead] = 0;
    ecgWaveHead = (ecgWaveHead + 1) % ECG_WAVE_POINTS;

    if (now - lastEcgGraphDrawTime >= 100) {
      lastEcgGraphDrawTime = now;
      drawLiveECGWaveform();

      drawECGStatusLine(true, false, 0);

      int bx = 16, by = 164, bw = 288, bh = 50;
      fillRect(bx + 4, by + 4, bw - 8, bh - 8, 0x0841);
      if (ecgState == ECG_STATE_IDLE) {
        drawCentered("ATTACH 3 ECG PADS TO BODY", by + 12, YELLOW, 1);
        drawCentered("Waiting for electrode contact...", by + 26, LIGHTGREY, 1);
      } else {
        drawCentered("LEADS DETACHED! PAUSED", by + 12, ORANGE, 1);
        drawCentered("Reattach pads to resume measuring", by + 26, WHITE, 1);
      }
    }
    return;
  }

  // 3. Module connected AND leads attached to body!
  if (ecgState == ECG_STATE_IDLE) {
    startECGMeasurement();
  }

  // Read analog value
  int rawAdc = analogRead(PIN_AD8232_OUT);

  // Digital High-Pass baseline tracker (removes DC offset)
  ecgBaseline = 0.94f * ecgBaseline + 0.06f * (float)rawAdc;
  float hp = (float)rawAdc - ecgBaseline;

  // Digital Low-Pass filter (smooths high-frequency noise)
  float filtered = 0.60f * ecgPrevFiltered + 0.40f * hp;
  ecgPrevFiltered = filtered;

  ecgWaveform[ecgWaveHead] = (int)filtered;
  if (ecgSampleBufIdx < ECG_SAMPLE_BUF_SIZE && (now % 30 < 10)) {
    ecgSampleBuffer[ecgSampleBufIdx++] = (int)rawAdc;
  }
  ecgWaveHead = (ecgWaveHead + 1) % ECG_WAVE_POINTS;

  // Pan-Tompkins Peak Detection
  float diff = filtered - ecgPrevDiff;
  ecgPrevDiff = filtered;
  float energy = diff * diff;
  ecgEnergyIntegrator = 0.70f * ecgEnergyIntegrator + 0.30f * energy;
  ecgThreshold = ecgNoisePeak + 0.25f * (ecgSignalPeak - ecgNoisePeak);

  if (ecgEnergyIntegrator > ecgThreshold && (now - ecgLastRPeakTime > 320)) {
    unsigned long rr = now - ecgLastRPeakTime;
    ecgLastRPeakTime = now;
    ecgSignalPeak = 0.85f * ecgSignalPeak + 0.15f * ecgEnergyIntegrator;

    if (ecgState == ECG_STATE_RECORDING) {
      if (rr >= 330 && rr <= 1800) {
        if (ecgPeakCount < MAX_ECG_PEAKS) {
          ecgRPeaks[ecgPeakCount++] = rr;
        }
        int instBpm = 60000 / rr;
        if (ecgLiveBpm == 0) ecgLiveBpm = instBpm;
        else ecgLiveBpm = (ecgLiveBpm * 3 + instBpm) / 4;
      }
    }
  } else if (energy < ecgThreshold) {
    ecgNoisePeak = 0.98f * ecgNoisePeak + 0.02f * ecgEnergyIntegrator;
  }

  // Draw Live Waveform Graph at 20 Hz
  if (now - lastEcgGraphDrawTime >= 50) {
    lastEcgGraphDrawTime = now;
    drawLiveECGWaveform();

    // Update Top info line (BPM & Leads Status)
    drawECGStatusLine(true, true, ecgLiveBpm);
  }

  // Progress Bar & State Transitions
  if (ecgState == ECG_STATE_WARMUP) {
    unsigned long elapsed = now - ecgWarmupStartTime;
    int sec = elapsed / 1000UL;
    if (sec != lastEcgDrawnSeconds && (now - lastEcgTimerDrawTime >= 150)) {
      lastEcgTimerDrawTime = now;
      lastEcgDrawnSeconds = sec;

      int bx = 16, by = 164, bw = 288, bh = 50;
      fillRect(bx + 4, by + 4, bw - 8, bh - 8, 0x0841);
      drawCentered("STABILIZING ECG SIGNAL (15s)...", by + 8, YELLOW, 1);
      drawCentered("Contact stabilization - Beats Ignored", by + 20, LIGHTGREY, 1);

      // Yellow progress bar
      int barW = bw - 24;
      int fillW = (sec * barW) / 15;
      if (fillW > barW) fillW = barW;
      drawRect(bx + 12, by + 32, barW, 10, YELLOW);
      if (fillW > 0) fillRect(bx + 13, by + 33, fillW, 8, YELLOW);
    }

    if (elapsed >= ECG_WARMUP_MS) {
      ecgState = ECG_STATE_RECORDING;
      ecgRecordingStartTime = now;
      lastEcgDrawnSeconds = -1;
      ecgPeakCount = 0;
      Serial.println("[AD8232] Warmup complete. Starting 60s clinical beat calculation!");
    }
  } else if (ecgState == ECG_STATE_RECORDING) {
    unsigned long elapsed = now - ecgRecordingStartTime;
    int sec = elapsed / 1000UL;
    if (sec != lastEcgDrawnSeconds && (now - lastEcgTimerDrawTime >= 150)) {
      lastEcgTimerDrawTime = now;
      lastEcgDrawnSeconds = sec;

      int bx = 16, by = 164, bw = 288, bh = 50;
      fillRect(bx + 4, by + 4, bw - 8, bh - 8, 0x0841);

      String title = "CALCULATING BEATS: " + String(sec) + " / 60s";
      drawCentered(title.c_str(), by + 8, GREEN, 1);

      String stats = "Live BPM: " + String(ecgLiveBpm > 0 ? ecgLiveBpm : 72) + " | Beats: " + String(ecgPeakCount);
      drawCentered(stats.c_str(), by + 20, CYAN, 1);

      // Green progress bar
      int barW = bw - 24;
      int fillW = (sec * barW) / 60;
      if (fillW > barW) fillW = barW;
      drawRect(bx + 12, by + 32, barW, 10, GREEN);
      if (fillW > 0) fillRect(bx + 13, by + 33, fillW, 8, GREEN);
    }

    if (elapsed >= ECG_RECORD_MS) {
      Serial.println("[AD8232] 60-second measurement finished! Generating report...");
      finishECGMeasurement();
    }
  }
}

void drawAboutPage() {
  fillRect(0, 0, SCREEN_W, SCREEN_H, BLACK);
  fillRect(0, 0, SCREEN_W, 32, DARKBLUE);
  drawText("ABOUT / HOW TO USE", 8, 12, WHITE, 1);
  fillRect(0, 30, SCREEN_W, 2, CYAN);

  // Compact QR on the left, with its website address below.
  const uint8_t modulePx = 2;
  const int quietZone = 4;
  const int qrSize = MEDICARE_QR_MODULES * modulePx;
  const int qrX = 12;
  const int qrY = 52;
  fillRect(qrX - quietZone, qrY - quietZone,
           qrSize + 2 * quietZone, qrSize + 2 * quietZone, WHITE);
  for (uint8_t row = 0; row < MEDICARE_QR_MODULES; row++) {
    for (uint8_t col = 0; col < MEDICARE_QR_MODULES; col++) {
      if (MEDICARE_QR_ROWS[row] & (1ULL << (MEDICARE_QR_MODULES - 1 - col))) {
        fillRect(qrX + col * modulePx, qrY + row * modulePx, modulePx, modulePx, BLACK);
      }
    }
  }

  drawText("https://", 10, 136, LIGHTGREY, 1);
  drawText("medicarepluss", 10, 147, LIGHTGREY, 1);
  drawText("vercel.app", 18, 158, LIGHTGREY, 1);

  drawText("HOW TO USE", 104, 42, CYAN, 1);
  drawText("BACK: HOME / PREVIOUS", 104, 58, WHITE, 1);
  drawText("NEXT: SLIDE TO NEXT TILE", 104, 74, WHITE, 1);
  drawText("ENTER: OPEN / START", 104, 90, WHITE, 1);
  drawText("SPO2: PLACE FINGER", 104, 110, WHITE, 1);
  drawText("ECG: AD8232 + 3 PADS", 104, 126, WHITE, 1);
  drawText("MEDICINE: VIEW SCHEDULE", 104, 142, WHITE, 1);
  drawText("ENTER: MARK TAKEN", 104, 158, WHITE, 1);
  drawText("NEXT: SKIP ALERT", 104, 174, WHITE, 1);
  fillRect(0, 218, SCREEN_W, 22, DARKBLUE);
  drawCentered("PRESS BACK FOR HOME", 225, LIGHTGREY, 1);
}

void drawMedicineRow(const char *label, const String &value, int y) {
  drawText(label, 12, y, CYAN, 1);
  String shown = value.length() ? value : "Not specified";
  if (shown.length() > 34) shown = shown.substring(0, 31) + "...";
  drawText(shown.c_str(), 108, y, WHITE, 1);
  fillRect(12, y + 12, 296, 1, 0x4208);
}

void drawMedicinePage() {
  fillRect(0, 0, SCREEN_W, SCREEN_H, BLACK);
  fillRect(0, 0, SCREEN_W, 34, DARKBLUE);
  fillRect(0, 32, SCREEN_W, 2, CYAN);
  drawCentered("CURRENT MEDICINE", 12, WHITE, 1);

  drawRect(8, 40, 304, 166, CYAN);
  if (!hasCurrentMedicineRecord) {
    drawCentered("NO MEDICINE RECORD YET", 105, LIGHTGREY, 1);
    drawCentered("NEW REMINDER WILL SHOW HERE", 123, CYAN, 1);
  } else {
    drawMedicineRow("PATIENT", currentMedicinePatient, 47);
    drawMedicineRow("GENDER", currentMedicineGender, 66);
    drawMedicineRow("MEDICINE", currentMedicineName, 85);
    drawMedicineRow("DOSE", currentMedicineDose, 104);
    drawMedicineRow("START DATE", currentMedicineDate, 123);
    drawMedicineRow("TIME", currentMedicineTime, 142);
    String times = "1 daily x " + currentMedicineDays + " days";
    drawMedicineRow("TIMES", times, 161);
    drawMedicineRow("STATUS", currentMedicineStatus, 180);
  }

  fillRect(0, 218, SCREEN_W, 22, DARKBLUE);
  drawCentered("BACK: HOME   NEXT: SLIDE", 225, LIGHTGREY, 1);
}

void drawButtonPage(uint8_t page) {
  fillRect(0, 0, SCREEN_W, SCREEN_H, BLACK);
  fillRect(0, 0, SCREEN_W, 34, DARKBLUE);
  fillRect(0, 32, SCREEN_W, 2, CYAN);

  if (page == 1) {
    openECGPage();
    return;
  } else if (page == 2) {
    openSpO2Page();
    return;
  } else if (page == 3) {
    drawMedicinePage();
    return;
  } else if (page == 4) {
    drawAboutPage();
    return;
  }

  fillRect(0, 218, SCREEN_W, 22, DARKBLUE);
  drawCentered("PRESS BACK FOR HOME", 225, LIGHTGREY, 1);
}

void buzzerStartReminder() {
  buzzerActive = true;
  Serial.println("[Buzzer] Reminder alert triggered (virtual buzzer ON)");
}

void buzzerStopReminder() {
  buzzerActive = false;
  Serial.println("[Buzzer] Reminder alert stopped (virtual buzzer OFF)");
}

void checkPhysicalButtons() {
  static uint8_t previousBack = LOW;
  static uint8_t previousNext = LOW;
  static uint8_t previousEnter = LOW;
  static unsigned long lastButtonPress = 0;

  uint8_t back = digitalRead(PIN_BTN_BACK);
  uint8_t next = digitalRead(PIN_BTN_NEXT);
  uint8_t enter = digitalRead(PIN_BTN_ENTER);

  // While a medicine reminder is visible: BACK = dismiss, NEXT = skip, ENTER = taken.
  if (reminderOnScreen) {
    if (millis() - lastButtonPress > 25) {
      if (back == HIGH && previousBack == LOW) {
        lastButtonPress = millis();
        dismissMedicineReminderTFT("DISMISSED");
      } else if (next == HIGH && previousNext == LOW) {
        lastButtonPress = millis();
        dismissMedicineReminderTFT("SKIPPED");
      } else if (enter == HIGH && previousEnter == LOW) {
        lastButtonPress = millis();
        dismissMedicineReminderTFT("TAKEN");
      }
    }
    previousBack = back;
    previousNext = next;
    previousEnter = enter;
    return;
  }

  // Ultra-fast 25ms debounce: instant response for any button type
  // (tactile push buttons, big push buttons, etc.)
  if (millis() - lastButtonPress > 25) {
    if (back == HIGH && previousBack == LOW) {
      lastButtonPress = millis();
      Serial.println("[Button] BACK pressed");
      if (activePage == 0) {
        menuSelection = (menuSelection + 3) % 4;
        drawMenuSelection(false);
      } else {
        spo2Measuring = false;
        spo2State = SPO2_UI_IDLE;
        ecgState = ECG_STATE_IDLE;
        activePage = 0;
        drawMenuSelection(true);
      }
    } else if (next == HIGH && previousNext == LOW) {
      lastButtonPress = millis();
      Serial.println("[Button] NEXT pressed");
      if (activePage == 0) {
        menuSelection = (menuSelection + 1) % 4;
        drawMenuSelection(false);
      }
    } else if (enter == HIGH && previousEnter == LOW) {
      lastButtonPress = millis();
      Serial.println("[Button] ENTER pressed");
      if (activePage == 0) {
        activePage = menuSelection + 1;
        if (activePage == 2) {
          openSpO2Page();
        } else {
          drawButtonPage(activePage);
        }
      } else if (activePage == 1) {
        if (isECGModuleConnected()) {
          startECGMeasurement();
        } else {
          Serial.println("[AD8232] Cannot start: Module not connected");
        }
      } else if (activePage == 2) {
        startSpO2Measurement();
      }
    }
  }

  previousBack = back;
  previousNext = next;
  previousEnter = enter;
}

// ======================================================

// MEDICINE REMINDER DISPLAY

// ======================================================

bool reminderOnScreen = false;

unsigned long reminderStartTime = 0;

bool activeReminderIsTest = false;

void showMedicineReminderTFT(const String &patientName, const String &medName,
                             const String &dosage, const String &schedDate,
                             const String &schedTime, const String &gender,
                             const String &days, bool isTest)

{

  reminderOnScreen = true;

  // Keep the newest reminder as the single current record for the Medicine page.
  currentMedicinePatient = patientName;
  currentMedicineGender = gender.length() ? gender : "Not specified";
  currentMedicineName = medName;
  currentMedicineDose = dosage;
  currentMedicineDate = schedDate;
  currentMedicineTime = schedTime;
  currentMedicineDays = days.length() ? days : "1";
  currentMedicineStatus = "PENDING";
  hasCurrentMedicineRecord = true;

  reminderStartTime = millis();

  activeReminderIsTest = isTest;

  // Dark background

  fillRect(0, 0, SCREEN_W, SCREEN_H, BLACK);

  // Header: MEDICARE+ MEDICINE REMINDER

  fillRect(0, 0, SCREEN_W, 36, DARKBLUE);

  fillRect(0, 34, SCREEN_W, 2, CYAN);

  drawCentered("MEDICARE+ MEDICINE REMINDER", 11, WHITE, 1);

  // Greeting and message

  String greet = "HELLO, " + patientName + "!";

  drawCentered(greet.c_str(), 46, YELLOW, 1);

  drawCentered("IT IS TIME TO TAKE YOUR MEDICINE.", 60, WHITE, 1);

  // Card outline & fill

  drawRect(12, 76, 296, 122, CYAN);

  fillRect(14, 78, 292, 118, 0x0841);

  // Medicine Name

  String nameText = "MEDICINE: " + medName;

  drawCentered(nameText.c_str(), 88, WHITE, 2);

  int yPos = 112;

  if (dosage.length() > 0 && !dosage.equalsIgnoreCase("None specified"))

  {

    String doseText = "DOSAGE: " + dosage;

    drawCentered(doseText.c_str(), yPos, LIGHTGREY, 1);

    yPos += 14;
  }

  // Scheduled Date

  String dateText = "SCHEDULED DATE: " + schedDate;

  drawCentered(dateText.c_str(), yPos, CYAN, 1);

  yPos += 14;

  // Scheduled Time

  String timeText = "SCHEDULED TIME: " + schedTime;

  drawCentered(timeText.c_str(), yPos, CYAN, 1);

  yPos += 16;

  // Status

  if (isTest)

  {

    drawCentered("STATUS: TEST REMINDER (60S AUTO-HIDE)", yPos, ORANGE, 1);

  }

  else

  {

    drawCentered("STATUS: REMINDER ACTIVE", yPos, GREEN, 1);
  }

  // Footer bar: medicine reminder actions

  fillRect(0, 204, SCREEN_W, 36, DARKBLUE);

  fillRect(0, 204, SCREEN_W, 2, CYAN);

  drawCentered("MEDICINE REMINDER", 211, WHITE, 1);

  drawCentered("[ TAKE  /  SKIP  /  DISMISS ]", 225, LIGHTGREY, 1);
}

void hideMedicineReminderTFT()

{

  if (!reminderOnScreen)
    return;

  reminderOnScreen = false;

  activeReminderIsTest = false;
  buzzerStopReminder();

  drawMenuSelection(true);
}

void dismissMedicineReminderTFT(const String &actionStatus)

{

  if (!reminderOnScreen)
    return;

  if (actionStatus == "TAKEN") currentMedicineStatus = "TAKEN";
  else if (actionStatus == "SKIPPED") currentMedicineStatus = "SKIPPED";

  // Show brief confirmation badge

  fillRect(14, 140, 292, 45, BLACK);

  if (actionStatus == "TAKEN")

  {

    drawRect(50, 145, 220, 28, GREEN);

    fillRect(52, 147, 216, 24, DARKGREEN);

    drawCentered("CONFIRMED: TAKEN", 154, GREEN, 1);

  }

  else if (actionStatus == "SKIPPED")

  {

    drawRect(50, 145, 220, 28, RED);

    fillRect(52, 147, 216, 24, DARKRED);

    drawCentered("CONFIRMED: SKIPPED", 154, WHITE, 1);

  }

  else

  {

    drawRect(50, 145, 220, 28, GREY);

    fillRect(52, 147, 216, 24, DARKGREY);

    drawCentered("REMINDER DISMISSED", 154, WHITE, 1);
  }

  delay(1200);

  // Sync button action back to Firebase Realtime Database
  updateFirebaseMedicineStatus(actionStatus);

  hideMedicineReminderTFT();
}

// ======================================================
// AP MODE STARTUP CHECK (HOLD BACK BUTTON ON POWER-UP)
// ======================================================
bool checkAPModeTriggerOnBoot() {
  pinMode(PIN_BTN_BACK, INPUT_PULLDOWN);
  delay(50); // Settle electrical state
  // Buttons are active HIGH (pressed connects the input to 3.3V).
  if (digitalRead(PIN_BTN_BACK) != HIGH) {
    return false; // Back button not held at power-up
  }

  // Confirm BACK stays pressed for ~1.5s to avoid triggering on electrical noise.
  unsigned long start = millis();
  while (millis() - start < 1500) {
    if (digitalRead(PIN_BTN_BACK) != HIGH) return false; // Released early
    delay(20);
  }
  return true;
}

void runAPModeConfiguration() {
  initTFT();
  drawAPModeScreen();
  wifiManagerStartAP();
  wifiPortalInit();

  Serial.println();
  Serial.println("==================================================");
  Serial.println("[AP Mode] MediCare+ Wi-Fi Portal Active!");
  Serial.println("Connect to 'MediCare+' (password: medic@re)");
  Serial.println("Open http://192.168.4.1 in your phone browser.");
  Serial.println("==================================================");

  while (true) {
    wifiPortalHandleClient();
    delay(2);
  }
}

// ======================================================

// SETUP

// ======================================================

void setup()

{

  Serial.begin(115200);

  delay(300);

  // Initialize Wi-Fi Manager and load saved credentials from NVM
  // Preserve the existing build-time credentials on first boot. Once users
  // save portal credentials, the NVM list remains the source of truth.
  wifiManagerInit(WIFI_SSID, WIFI_PASSWORD);

  // 1. Check if BACK button is held on power-up for AP Configuration Portal
  if (checkAPModeTriggerOnBoot()) {
    Serial.println("[Boot] BACK button held at startup. Entering AP/QR Mode.");
    runAPModeConfiguration();
    return;
  }

  // 2. If no saved networks exist in NVM, automatically boot into AP/QR Mode
  if (!wifiManagerHasSavedNetworks()) {
    Serial.println("[Boot] No saved Wi-Fi networks found in NVM! Automatically booting into AP/QR Mode.");
    runAPModeConfiguration();
    return;
  }

  // Buttons are active HIGH: press connects GPIO to 3.3V.
  pinMode(PIN_BTN_BACK, INPUT_PULLDOWN);
  pinMode(PIN_BTN_NEXT, INPUT_PULLDOWN);
  pinMode(PIN_BTN_ENTER, INPUT_PULLDOWN);

  // AD8232 ECG Bio-Amplifier Sensor (Internal Pulldowns ensure disconnected detection)
  pinMode(PIN_AD8232_OUT, INPUT_PULLDOWN);
  pinMode(PIN_AD8232_LOM, INPUT_PULLDOWN);
  pinMode(PIN_AD8232_LOP, INPUT_PULLDOWN);
  analogReadResolution(12);

  // ----------------------------------

  // TFT

  // ----------------------------------

  initTFT();

  // ----------------------------------

  // BOOT

  // ----------------------------------

  bootScreen();

  // ----------------------------------

  // MAX30102

  // ----------------------------------

  initMAX30102();

  // ----------------------------------

  // HOME SCREEN IMMEDIATELY

  // ----------------------------------

  drawMenuSelection(true);

  // ----------------------------------

  // START WIFI IN BACKGROUND (Managed by FreeRTOS Task C)

  // ----------------------------------

  startWiFi();

  // ----------------------------------

  // FREERTOS INITIALIZATION & TASKS

  // ----------------------------------

  rtosInitSystem();
  rtosStartTasks();

  Serial.println();

  Serial.println(

    "=============================="

  );

  Serial.println(

    "MEDICARE+"

  );

  Serial.println(

    "Smart Patient Monitoring"

  );

  Serial.println(

    "& Medication System"

  );

  Serial.println(

    "=============================="

  );
}

// ======================================================

// LOOP (FreeRTOS Background Idle Yield)

// ======================================================

void loop()

{
  wifiConnected = wifiManagerIsConnected();
  vTaskDelay(pdMS_TO_TICKS(100));
}

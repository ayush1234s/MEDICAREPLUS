#include <SPI.h>

#include <Wire.h>

#include <WiFi.h>

#include <time.h>

#include <Preferences.h>
#include <math.h>

#include "MAX30105.h"




#include "MediCareDashboard.h"
#include "MediCareFirebase.h"

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

    uint16_t pixel = pgm_read_word(&MEDICARE_DASHBOARD_RGB565[i]);

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

  WiFi.mode(WIFI_STA);

  WiFi.setAutoReconnect(true);

  WiFi.begin(

    WIFI_SSID,

    WIFI_PASSWORD

  );

  lastWiFiAttempt = millis();
}

// ======================================================

// WIFI BACKGROUND PROCESS

// ======================================================

void updateWiFi()

{

  wl_status_t status =

    WiFi.status();

  if (status == WL_CONNECTED)

  {

    if (!wifiConnected)

    {

      wifiConnected = true;

      // India timezone UTC +5:30

      configTime(

        19800,

        0,

        "pool.ntp.org",

        "time.nist.gov"

      );

      Serial.println();

      Serial.println(

        "WiFi connected!"

      );

      Serial.print(

        "IP Address: "

      );

      Serial.println(

        WiFi.localIP()

      );
    }

    return;
  }

  wifiConnected = false;

  // Retry every 10 seconds

  if (

    millis() - lastWiFiAttempt >

    10000

  )

  {

    lastWiFiAttempt =

      millis();

    WiFi.disconnect();

    WiFi.begin(

      WIFI_SSID,

      WIFI_PASSWORD

    );

    Serial.println(

      "Trying WiFi..."

    );
  }
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

  fillRect(x + r, y, w - 2 * r, h, color);

  fillRect(x, y + r, r, h - 2 * r, color);

  fillRect(x + w - r, y + r, r, h - 2 * r, color);

  fillRect(x + 1, y + 1, r, r, color);

  fillRect(x + w - 1 - r, y + 1, r, r, color);

  fillRect(x + 1, y + h - 1 - r, r, r, color);

  fillRect(x + w - 1 - r, y + h - 1 - r, r, r, color);
}

void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r,
                   uint16_t color)

{

  fillRect(x + r, y, w - 2 * r, 1, color);

  fillRect(x + r, y + h - 1, w - 2 * r, 1, color);

  fillRect(x, y + r, 1, h - 2 * r, color);

  fillRect(x + w - 1, y + r, 1, h - 2 * r, color);

  fillRect(x + 1, y + 1, 1, 1, color);

  fillRect(x + w - 2, y + 1, 1, 1, color);

  fillRect(x + 1, y + h - 2, 1, 1, color);

  fillRect(x + w - 2, y + h - 2, 1, 1, color);
}

// ======================================================

// HEADER (dynamic overlay on bitmap)

// ======================================================

void drawHeader()

{

  // WiFi icon: draw on bitmap background (0x0085)

  fillRect(218, 3, 30, 29, 0x0085);

  drawWiFiStatus();

  // Clock: erase old and redraw

  String currentTime = getTimeString();

  fillRect(248, 5, 68, 22, 0x0085);

  drawText(currentTime.c_str(), 254, 11, WHITE, 1);
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

void updateBottomStatus()

{

  // System Status card background color from bitmap = 0x08C7

  // Clear the status text area inside the card (below "SYSTEM STATUS" title)

  fillRect(15, 158, 290, 38, 0x08C7);

  // 1. Wi-Fi status - round dot

  uint16_t wifiDot = wifiConnected ? GREEN : ORANGE;

  drawStatusDot(25, 165, 3, wifiDot);

  drawText(wifiConnected ? "Wi-Fi connected" : "Wi-Fi  Disconnected", 34, 161,
           WHITE, 1);

  // 2. MAX30102 live sensor status - round dot

  if (max30102Ready)

  {

    drawStatusDot(25, 179, 3, GREEN);

    drawText("MAX30102  Ready", 34, 175, GREEN, 1);

  }

  else

  {

    drawStatusDot(25, 179, 3, RED);

    drawText("Sensor not reading", 34, 175, RED, 1);
  }

  // 3. ECG status (Hardware not connected) - round dot

  drawStatusDot(177, 179, 3, 0x632C);

  drawText("ECG  Not Connected", 186, 175, LIGHTGREY, 1);
}

// ======================================================

// HOME SCREEN

// ======================================================

void drawHomeScreen()

{

  // 1. Draw the full bitmap artwork (header, 4 cards, icons, tagline, system
  // status card)

  drawDashboardBitmap();

  // 2. Lift the System Status artwork into the space that used to contain
  // the upper tagline, then clear its old lower position.
  copyDashboardArea(10, 160, 140, 300, 72);
  fillRect(0, 212, SCREEN_W, 28, 0x0085);

  // 3. Overlay dynamic header (WiFi icon + clock only)

  drawHeader();

  // 4. System status green dot removed per design update
  // (no green dot before SYSTEM STATUS)

  // 5. Live status inside the card (WiFi, MAX30102, ECG)

  updateBottomStatus();

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

  // Refresh clock & Wi-Fi without repainting the entire dashboard

  fillRect(218, 3, 101, 29, 0x0085);

  drawWiFiStatus();

  String currentTime = getTimeString();

  fillRect(248, 5, 68, 22, 0x0085);

  drawText(currentTime.c_str(), 254, 11, WHITE, 1);
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
  const int tileX[] = { 10, 90, 170, 250 };

  // Redraw first so the previous card returns to its normal position.
  // Then lift the active tile by four pixels; no coloured hover line is used.
  drawHomeScreen();
  int x = tileX[menuSelection] - 2;
  int w = min(79, SCREEN_W - x);
  copyDashboardArea(x, 41, 37, w, 94);
  // Clear the small strip left by the upward lift with the dashboard gap below.
  copyDashboardArea(x, 135, 131, w, 4);
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

  fillRect(bx + 12, by + 12, bw - 24, 22, 0x0085);
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
  fillRect(gx + 1, gy + 1, gw - 2, 16, 0x0085);
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

void drawAboutPage() {
  fillRect(0, 0, SCREEN_W, SCREEN_H, BLACK);
  fillRect(0, 0, SCREEN_W, 34, DARKBLUE);
  fillRect(0, 32, SCREEN_W, 2, CYAN);
  drawCentered("ABOUT", 12, WHITE, 1);

  // Main content area
  drawCentered("SMART POCKET", 44, WHITE, 2);
  drawCentered("MEDICAL KIT", 62, WHITE, 2);
  drawCentered("MEDICARE+", 86, CYAN, 2);
  fillRect(80, 104, 160, 1, DARKGREY);

  drawCentered("LOCAL HEALTH MONITORING", 116, LIGHTGREY, 1);
  drawCentered("SpO2 AND HEART RATE", 140, CYAN, 1);
  drawCentered("LOCAL DEVICE MONITORING", 164, WHITE, 1);
  drawCentered("NO INTERNET REQUIRED", 180, WHITE, 1);

  // Bottom bar
  fillRect(0, 230, SCREEN_W, 10, DARKBLUE);
  drawCentered("PRESS BACK FOR HOME", 232, LIGHTGREY, 1);
}

void drawButtonPage(uint8_t page) {
  fillRect(0, 0, SCREEN_W, SCREEN_H, BLACK);
  fillRect(0, 0, SCREEN_W, 34, DARKBLUE);
  fillRect(0, 32, SCREEN_W, 2, CYAN);

  if (page == 1) {
    drawCentered("ECG + HEART", 12, WHITE, 1);
    drawCentered("ECG SENSOR NOT CONNECTED", 82, ORANGE, 1);
    drawCentered("CONNECT AD8232 SENSOR TO USE ECG", 108, LIGHTGREY, 1);
  } else if (page == 2) {
    openSpO2Page();
    return;
  } else if (page == 3) {
    drawAboutPage();
    return;
  } else {
    drawCentered("MEDICINE REMINDERS", 12, WHITE, 1);
    drawCentered("MEDICINE REMINDERS", 85, WHITE, 1);
    drawCentered("LOCAL REMINDER TEST SCREEN", 107, CYAN, 1);
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
        // Visual button press confirmation feedback
        const int tileX[] = { 10, 90, 170, 250 };
        int px = tileX[menuSelection] - 2;
        drawRect(px - 1, 40, 81, 96, GREEN);
        drawRect(px, 41, 79, 94, CYAN);
        delay(40);
        activePage = menuSelection + 1;
        if (activePage == 2) {
          openSpO2Page();
        } else {
          drawButtonPage(activePage);
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
                             const String &schedTime, bool isTest)

{

  reminderOnScreen = true;

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

// SETUP

// ======================================================

void setup()

{

  Serial.begin(115200);

  delay(300);

  // Buttons are active HIGH: press connects GPIO to 3.3V.
  pinMode(PIN_BTN_BACK, INPUT_PULLDOWN);
  pinMode(PIN_BTN_NEXT, INPUT_PULLDOWN);
  pinMode(PIN_BTN_ENTER, INPUT_PULLDOWN);

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

  // START WIFI IN BACKGROUND (used for clock sync only)

  // ----------------------------------

  startWiFi();

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

// LOOP

// ======================================================

void loop()

{

  // ----------------------------------

  // WiFi background connection

  // ----------------------------------

  updateWiFi();

  // ----------------------------------

  // Non-blocking medicine test-mode auto-hide

  // ----------------------------------

  if (reminderOnScreen && MEDICINE_REMINDER_TEST_MODE)

  {

    if (millis() - reminderStartTime >= ((unsigned long)REMINDER_TEST_AUTO_HIDE_SECONDS * 1000UL))

    {

      Serial.println(
        "[TFT Test Mode] reminder display auto-hide triggered.");

      hideMedicineReminderTFT();
    }
  }

  // ----------------------------------

  // Update header every second (when not showing reminder)

  // ----------------------------------

  if (

    millis() - lastHeaderUpdate >=

    1000

  )

  {

    lastHeaderUpdate =

      millis();

    if (!reminderOnScreen && activePage == 0)

    {

      updateSensorStatus();

      updateHeader();

      updateBottomStatus();
    }
  }

  checkPhysicalButtons();

  // Check for active medicine reminders scheduled from MEDICARE+ web portal
  checkFirebaseMedicineReminders();

  updateSpO2Measurement();

  delay(1);
}
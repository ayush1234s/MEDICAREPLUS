# MEDICARE+

MEDICARE+ has two parts:

1. An ESP32 pocket device with a display, MAX30102-compatible pulse sensor, Wi-Fi, and physical buttons.
2. A React website for signing in, viewing readings, scheduling medicines, and making reports.

Both parts use Firebase Realtime Database. The website and device must be configured to use the same Firebase project and database.

> **Health notice:** This project is for learning and general wellness. It is not a medical device and does not diagnose, treat, or replace a doctor. For severe symptoms or an emergency, contact a doctor or emergency service.

## Contents

- [Project files](#project-files)
- [How the system works](#how-the-system-works)
- [Run the website](#run-the-website)
- [Use the website](#use-the-website)
- [Build and flash the ESP32](#build-and-flash-the-esp32)
- [Connect the device to Wi-Fi](#connect-the-device-to-wi-fi)
- [Set up the device hotspot](#set-up-the-device-hotspot)
- [Firebase data and setup](#firebase-data-and-setup)
- [Hardware pins and controls](#hardware-pins-and-controls)
- [Troubleshooting](#troubleshooting)
- [Important limits and security notes](#important-limits-and-security-notes)

## Project files

```text
MEDICARE+/
├── README.md
├── SmartPocketMedicalKit/
│   ├── SmartPocketMedicalKit.ino   Main ESP32 sketch and display code
│   ├── MediCareFirebase.h          Firebase REST calls and medicine reminders
│   ├── MediCareWiFiManager.h/.cpp  Saved Wi-Fi networks, connection, and AP mode
│   ├── MediCareWiFiPortal.h/.cpp   Phone browser setup page and HTTP routes
│   ├── MediCareRTOS.h/.cpp         FreeRTOS tasks and offline reading queue
│   ├── MediCareQrCode.h            Dashboard QR bitmap data
│   └── MediCareDashboard.h         Dashboard image data
└── medicare-web/
    ├── src/App.jsx                 Sign-in state, navigation, and page loading
    ├── src/firebase.js             Firebase web app, authentication, and data
    ├── src/pages/                  Home, Test, Report, Medicine, Help, Profile
    └── src/components/             Sign-in, splash screen, and sidebar
```

## How the system works

```text
MAX30102 sensor ──> ESP32 ──Wi-Fi/HTTPS──> Firebase Realtime Database
                                           ▲               │
                                           │               ├──> Website live readings
                                           │               ├──> Website history and reports
                                           │               └──> Medicine reminder
                                           └── ESP32 polls the reminder and sends button status
```

The device reads the pulse sensor and can send SpO2 and heart-rate readings to Firebase. The website listens for live values and loads saved history when you open the Test or Report pages. The website writes medicine schedules to Firebase. The device checks the active reminder and shows it on its display when its date and time arrive.

The firmware stores up to 10 Wi-Fi networks in ESP32 non-volatile storage (NVS). It can queue up to 20 readings in NVS when an upload is attempted without Wi-Fi, then retry them after reconnecting.

## Run the website

### 1. Install Node.js

Install a current Node.js version supported by Vite, then check that both commands work:

```sh
node --version
npm --version
```

### 2. Configure Firebase

Create or select a Firebase project. Add a Web App in the Firebase project settings and enable the Firebase products the code uses:

- Authentication: Email/Password and Google sign-in.
- Realtime Database.

In `medicare-web/`, create a file named `.env.local` with the web app settings:

```dotenv
VITE_FIREBASE_API_KEY=your_api_key
VITE_FIREBASE_AUTH_DOMAIN=your_project.firebaseapp.com
VITE_FIREBASE_DATABASE_URL=https://your_database_url
VITE_FIREBASE_PROJECT_ID=your_project_id
VITE_FIREBASE_STORAGE_BUCKET=your_project.appspot.com
VITE_FIREBASE_MESSAGING_SENDER_ID=your_sender_id
VITE_FIREBASE_APP_ID=your_app_id
VITE_FIREBASE_MEASUREMENT_ID=your_measurement_id
```

Use the values shown in Firebase project settings. The database URL must point to the same Realtime Database used by the ESP32. The firmware currently has its own database URL in `SmartPocketMedicalKit/MediCareFirebase.h`; change it there if your database URL is different.

Do not commit `.env.local`. The repository `.gitignore` already excludes `.env` files.

### 3. Install packages and start the development server

From the repository folder:

```sh
cd medicare-web
npm ci
npm run dev
```

Open the local address printed by Vite, usually `http://localhost:5173`. To make a production build or run the linter:

```sh
npm run build
npm run lint
```

### 4. Sign in

The website shows a short splash screen, then the sign-in page. Create an account with email and password or use Google. On small/mobile browsers, Google sign-in uses a redirect; on desktop it tries a popup first.

After sign-in, the app loads the user profile and listens for live readings and medicine updates. If Google sign-in fails on a deployed site, add that site’s domain to the Firebase Authentication authorized domains list.

## Use the website

The signed-in website has five main sections in the sidebar or mobile bottom bar. Profile and Settings are available from the profile menu in the top bar.

### Home

- Shows an overview, links to other sections, and the latest live values when available.
- The Critical Health Advisory note has English and हिन्दी buttons. English is selected by default.
- The page also explains the kit and its basic workflow.

Some Home values are display fallbacks when there is no live data. The top-bar connection label is currently presentation text; do not use it as proof that the device is online.

### Test

- Open SpO2 or ECG to see readings and history.
- Use the date filters to narrow the list.
- The simulation buttons create sample readings in the website; they do not measure the user.
- Delete controls remove history records from Firebase and the local browser cache when the write is allowed.

### Report

- Choose SpO2, ECG, or both, and optionally choose a date range.
- Generate a preview, then download a PDF.
- The report contains the readings returned by Firebase/local fallback and profile details available to the app. Its text guidance is general information, not a diagnosis.

### Medicine

- Enter a medicine name, dose, date, time, duration, patient name, and gender, then add the schedule.
- The web app saves the medicine in `/medicines` and writes the current active reminder to `/active_reminder`.
- Change the status on the web page or use the device reminder buttons. The list listens for Firebase changes.

The firmware uses one `/active_reminder` record. Adding another medicine replaces that single active reminder entry on the device side, even though the web list can contain several medicines.

### Help

- Open FAQ answers and drag FAQ cards to reorder them for the current page session.
- The feedback form currently shows a confirmation in the page only. It does not send an email or support request to a server.

### Profile and Settings

- Save profile details and view the generated Medicare ID and QR code.
- Choose the light or dark theme.
- The account deletion control saves a scheduled-deletion marker and allows cancelling it. The current code does **not** automatically delete the Firebase Auth account, profile, or records after seven days.

## Build and flash the ESP32

The firmware uses ESP32 Wi-Fi and FreeRTOS APIs. Use Arduino IDE with an ESP32 board package and select the board that matches your actual module. Install the **SparkFun MAX3010x Pulse and Proximity Sensor Library** (it supplies `MAX30105.h`). The display driver is implemented directly in the sketch; the code expects an ILI9341-compatible 320×240 TFT.

1. Open `SmartPocketMedicalKit/SmartPocketMedicalKit.ino` in Arduino IDE.
2. Install the ESP32 board support and the MAX3010x library if they are not already installed.
3. Select the correct ESP32 board and serial port.
4. Check the pin table below against your wiring.
5. Build and upload the sketch.
6. Open Serial Monitor at **115200 baud** to see startup and connection messages.

The sketch can load Wi-Fi credentials from `SmartPocketMedicalKit/MediCareWiFiSecrets.h`. Define `WIFI_SSID` and `WIFI_PASSWORD` there. That file is ignored by Git. If it is absent, the sketch has fallback credentials in the source; replace those before sharing or uploading the project.

The `.cpp` and `.h` files in the sketch folder are part of the firmware and must remain beside the `.ino` file when building.

## Connect the device to Wi-Fi

At startup, the Wi-Fi manager reads networks saved in NVS. If the list is empty, it seeds the list once from `WIFI_SSID` and `WIFI_PASSWORD`. It scans for saved networks, tries the strongest matching saved network, and retries in the background if it cannot connect. Connection attempts time out and cycle through the saved list.

If the configured network is not available, the device stays in offline mode and keeps retrying. It does not automatically start the setup hotspot just because a saved network failed to connect. Hold BACK while powering on to open the setup portal.

When Wi-Fi connects, the device requests network time and uses India time (UTC+05:30) for reminder scheduling and reading dates.

## Set up the device hotspot

The device hotspot is a local setup network. It does not provide internet access.

### Open the portal

1. Turn the device off.
2. Press and hold the **BACK** button.
3. Turn the device on while still holding BACK. Keep holding it for about 1.5 seconds.
4. Release BACK when the setup screen appears. The code treats the buttons as active-high: a pressed button connects its GPIO to 3.3 V.
5. On a phone or computer, open Wi-Fi settings and join:

   - Network: `MediCare+`
   - Password: `medic@re`

6. If the phone says “No internet”, choose the option to stay connected to this network.
7. Open a browser and go to `http://192.168.4.1`.

If the firmware has no saved Wi-Fi network, it also starts this portal automatically. With the source fallback credentials present, first boot normally seeds those credentials, so use the BACK-button method when you need to configure the device manually.

### Save a router or phone-hotspot network

1. In the portal, press **Scan** to find nearby Wi-Fi networks.
2. Press **Add** next to a network, or add a blank row and type its name (SSID).
3. Enter its password. For a new open network, leave the password blank. When editing an existing saved network, a blank password means “keep the existing password”.
4. Save the list. Up to 10 networks can be stored.
5. Press **Reboot MEDICARE+** or restart the device.
6. Keep the selected router or phone hotspot available while the ESP32 reconnects.

The ESP32 supports 2.4 GHz Wi-Fi. If you want to use a phone hotspot, configure that hotspot for 2.4 GHz/compatibility mode if the phone offers that setting. Usually, one phone cannot stay connected to the device setup hotspot while also broadcasting its own hotspot. Use a second phone or computer to open the setup portal while the first phone provides the hotspot.

The portal also has a nearby network list, saved-network table, and a reboot button. Its firmware routes are `GET /api/networks`, `GET /api/scan`, `POST /api/save`, `POST /api/delete`, and `GET /api/reboot`.

## Firebase data and setup

The firmware’s REST bridge uses these Realtime Database paths:

| Path | Purpose |
| --- | --- |
| `/readings/spo2` | SpO2 and heart-rate history uploaded by the ESP32 |
| `/live/spo2` | Latest SpO2 reading for the web dashboard |
| `/readings/ecg` and `/live/ecg` | ECG-shaped records supported by the bridge; current firmware does not acquire live ECG sensor data |
| `/medicines` | Medicine list and status shared with the web app |
| `/active_reminder` | The single medicine reminder currently polled by the ESP32 |
| `/users/{uid}` | Web profile data |

The web app uses Firebase Authentication and the Firebase web SDK. The ESP32 currently makes unauthenticated Realtime Database REST requests. It also calls `setInsecure()`, which skips TLS certificate verification. Because of this, the firmware needs database access rules that permit its requests, but globally allowing every read and write is unsafe. Do not publish a database with open `.read` and `.write` rules or put real patient data in an unprotected project. Add device authentication and certificate validation before using this system with real users or sensitive data.

The browser permission banner may display an example of open rules for troubleshooting. That example is not a safe production configuration.

## Hardware pins and controls

| Part | ESP32 GPIO / setting |
| --- | --- |
| TFT SPI clock | GPIO 4 |
| TFT MOSI | GPIO 5 |
| TFT MISO | GPIO 6 |
| TFT chip select | GPIO 7 |
| TFT data/command | GPIO 10 |
| TFT reset | GPIO 11 |
| TFT backlight | GPIO 3 |
| MAX30102 I2C SDA | GPIO 8 |
| MAX30102 I2C SCL | GPIO 9 |
| BACK button | GPIO 1 |
| NEXT button | GPIO 12 |
| ENTER button | GPIO 13 |

Button presses are active-high and the code configures the inputs with pull-downs. On the main device menu, BACK and NEXT move between four tiles, and ENTER opens the selected tile. On the SpO2 page, ENTER starts a measurement. While a medicine reminder is on screen, BACK dismisses it, NEXT marks it skipped, and ENTER marks it taken.

**GPIO 1 is also commonly the ESP32 UART transmit pin.** The sketch uses `Serial.begin(115200)` for logs and also assigns GPIO 1 to BACK. Check your exact board and wiring; these two uses may conflict.

## Troubleshooting

### I cannot see the `MediCare+` Wi-Fi network

- Make sure you held BACK while powering on, for at least 1.5 seconds.
- Confirm the BACK button is wired active-high to GPIO 1 as the sketch expects.
- Look for AP startup messages in Serial Monitor.
- If necessary, try another board-specific GPIO; update the code and wiring together.

### The portal page does not open

- Stay connected to the `MediCare+` network even if it has no internet.
- Type `http://192.168.4.1` in the browser address bar. The portal does not rely on automatic captive-portal popups.
- Turn off mobile data temporarily if the phone keeps switching away from the local network.

### The device does not connect to my router or hotspot

- Check the exact SSID and password; Wi-Fi names and passwords are case-sensitive.
- Use a 2.4 GHz network.
- Keep the router/hotspot on while the device scans and retries.
- Use BACK at startup to reopen the setup portal and save the correct network.

### The website shows no readings or medicine changes

- Confirm `.env.local` points to the same Firebase project/database as the firmware URL.
- Check Firebase Authentication, Realtime Database availability, and database permissions.
- Open Serial Monitor and look for HTTP error codes from the Firebase bridge.
- Confirm the MAX30102 is connected to the configured I2C pins and detected at boot.

### Reminder times seem wrong

- The ESP32 sets India time after Wi-Fi connects. NTP needs internet access and can take time to synchronize.
- The reminder is shown only after its selected date and time have arrived.
- The device currently polls one active reminder, not every medicine entry independently.

## Important limits and security notes

- The firmware has MAX30102 SpO2 acquisition. It has an ECG upload format, but the current sketch does not read an AD8232 or another ECG input; ECG examples can be simulated from the website.
- The buzzer functions currently change a software flag and print messages. No physical buzzer GPIO is configured in this firmware.
- Medicine actions pressed while the device is offline are not queued for later upload.
- Offline sensor readings are stored locally only when an upload is attempted while Wi-Fi is disconnected. The persistent queue holds up to 20 records.
- The web app uses shared top-level sensor and medicine paths. The current ESP32 code does not authenticate as a user or separate device data by patient.
- The seven-day account deletion control records a date/status only; it does not perform automatic deletion.
- Avoid entering real medical data until Firebase access is secured and the device security limitations above are addressed.

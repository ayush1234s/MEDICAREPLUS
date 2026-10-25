# MEDICARE+ | Your Smart Medical Kit & Patient Monitoring Portal

A comprehensive IoT healthcare ecosystem connecting an **ESP32 Smart Pocket Medical Kit** with a modern **React + Tailwind CSS** cloud portal powered by **Firebase Realtime Database**.

---

## 🌟 System Architecture & Features

### 1. 🖥️ Web Application (`medicare-web`)
- **Splash Screen**: 5-second animated loading screen featuring glowing logo, pulse animation, and real-time initialization progress bar.
- **Authentication**:
  - Split-screen layout:
    - **Left**: Deep-dive presentation into **"YOUR SMART MEDICAL KIT"**, showcasing hardware features and sensor capabilities.
    - **Right**: Sign In & Sign Up with Google Auth (`signInWithPopup`), Email/Password authentication, and "Forgot Password" modal.
- **Collapsible Sidebar**:
  - Expand/collapse to show icons or full labels.
  - Navigation: `HOME`, `TEST`, `REPORT`, `MEDICINE`, `HELP`.
  - Profile card at the bottom with dropdown to view profile, change theme, and sign out.
- **User Profile Page**:
  - Fetches profile photo, Name, and Email Address from Google / Firebase.
  - Inputs for **Date of Birth**, **Gender**, Blood Group, and Emergency Contact.
  - Clicking **Submit** auto-generates a unique system **Medicare ID** (`MED-XXX-XXXX`) displayed on a digital holographic medical pass with QR code.
  - **Theme Toggle**: Dark medical aesthetic vs clean light mode.
  - **Account Deletion**: **7-Day Temporary Deletion** grace period with cancellation option before permanent erasure.
- **Home Page**:
  - Hero section with live ESP32 cloud connection status pill and quick vital metrics ticker.
  - **About MEDICARE+ - YOUR SMART MEDICAL KIT**: Sensor specs (MAX30102, AD8232 ECG).
  - **How To Use**: 4-step interactive guide.
  - Modern footer with quick navigation links.
- **Test Page (Sensors & Vitals)**:
  - 2 primary cards/tiles: **SPO2** and **ECG + HEART**.
  - Sub-views with **Date From** and **Date To** range filter.
  - Real-time animated ECG rhythm canvas drawing P-Q-R-S-T complexes.
  - Live pulse, SpO2 percentage, and diagnosis status badges (Optimal vs Mild Hypoxia).
  - Integrated "Simulate Hardware Push" for testing without hardware connected.
- **Report Page (PDF Export)**:
  - Form to choose test: **SPO2**, **ECG+HEART**, or **BOTH**.
  - Date range selector and clinical doctor's remarks input.
  - Formatted on-screen report preview.
  - **Download PDF Report**: Generates an official medical diagnostic report using `jspdf` and `jspdf-autotable`.
- **Medicine Page (Two-Way Device Link)**:
  - Medication scheduling form: Medicine Name, Dose, Date, Alarm Time, Duration (Days), Patient Name, and Gender.
  - Real-time table: **SrNo., Full Name, Gender, Medicine, Dose, Date, Time, Days, Action (Taken, Pending, Skip)**.
  - **Interactive Hardware Sync Hub**: Live preview of the 320x240 screen and buttons.
- **Help Section**:
  - Drag-and-drop interactive FAQ cards (drag to reorder, click to expand answers).
  - ESP32 hardware buttons guide (BACK, NEXT, ENTER GPIO pins).
  - Support & technical query form.

---

## 🔌 Hardware Integration (`SmartPocketMedicalKit`)

The ESP32 firmware connects directly to Firebase Realtime Database via REST HTTPS:

```
ESP32 Hardware (MAX30102 / AD8232)
   │
   ├── [SpO2 / BPM Reading Complete]  ──►  POST /readings/spo2.json  &  PUT /live/spo2.json
   │
   ├── [Background Loop]             ◄───  GET /active_reminder.json (Polls every 4s)
   │                                        (Triggers TFT Alarm + Buzzer on schedule)
   │
   └── [Physical Button Press]       ──►  PATCH /medicines/<id>.json & /active_reminder.json
          ENTER Button: TAKEN               Status: "Taken"
          NEXT Button:  SKIPPED              Status: "Skip"
```

### Files:
- [SmartPocketMedicalKit.ino](file:///c:/Users/ayush/OneDrive/Documents/MEDICARE+/SmartPocketMedicalKit/SmartPocketMedicalKit.ino): Main sketch with TFT display routines, sensor loops, button debouncing, and Firebase calls.
- [MediCareFirebase.h](file:///c:/Users/ayush/OneDrive/Documents/MEDICARE+/SmartPocketMedicalKit/MediCareFirebase.h): Cloud bridge with REST API calls for SpO2 upload, ECG upload, reminder polling, and hardware button status synchronization.

---

## 🚀 Running the Web Portal

The web server is running locally on:
```
http://127.0.0.1:5173/
```

To run again in future:
```powershell
cd medicare-web
npm run dev
```

To create a production build:
```powershell
cd medicare-web
npm run build
```

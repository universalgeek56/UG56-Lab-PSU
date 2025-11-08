---
# UG Lab PSU — User Guide

## 1. Getting Started

1. **Connect hardware**

   - PSU input (5–36 V recommended)

   - Output to your load (resistor, LED strip, motor, etc.)

   - ESP32 powered via onboard 5 V regulator or USB

2. **Power on**

   - OLED screen lights up

   - WS2812B ring shows startup animation

   - ESP32 enters **AP mode** by default

3. **Connect to Web UI**

   - Open the PSU front-end in your browser

   - All controls and navigation are accessible **without typing URLs**

---

## 2. Local UI Guide

### **Home Screen**

- Displays the **selected main parameter** (Voltage or Current) in large digits

- **Short press (Encoder):** Cycle through editable setpoints:
  
  - Vset (Voltage)
  
  - Iset (CC current limit)
  
  - Icut (Overcurrent/user fuse limit)

- **Rotate Encoder:** Enter edit mode and apply changes immediately
  
  - **Default step:** 0.01
  
  - **Short press while editing:** Cycle step size 0.01 → 0.1 → 1.0
  
  - Step sizes for Voltage and Currents are remembered separately across sessions

- **Long press Encoder:** Enter **Settings Menu**


![Home Screen](/Screenshots/ui_home.JPG)

---

### **Settings Menu Pages**

Pages switch by **rotating the encoder**, confirm selection by pressing:

1. **Errors / NTC Temperature**
   
   - Displays current errors
   
   - If multiple errors → auto-rotate through them
   
   - Shows NTC temperature (e.g., converter chip)

 
   ![Errors / NTC](/Screenshots/ui_errors.JPG)

1. **Wi-Fi: STA/AP Mode**
   
   - Shows current network & password in AP mode
   
   - **Short press Encoder:** enable selection mode
   
   - **Rotate Encoder:** select desired mode (STA or AP)
   
   - **Press Encoder:** confirm → triggers reboot in 5 s
   
     
   ![Wi-Fi STA/AP](/Screenshots/ui_wifi_mode.JPG)

2. **Wi-Fi: ON/OFF**
   
   - Shows **SSID / IP** in STA mode
   
   - Network & password changes only via **Web UI**
   
   - Changing mode triggers **reboot in 5 s**
   
    
   ![Wi-Fi On/Off](/Screenshots/ui_wifi_onoff.JPG)

3. **OTA: ON/OFF**
   
   - Short press → select ON/OFF
   
   - Rotate → choose option
   
   - Press → confirm
   
     
   ![OTA Screen](/Screenshots/ui_ota.JPG)

4. **Main Parameter on Home Screen**
   
   - Choose which main parameter is displayed on the home screen (Voltage or Current)
   
   - **Rotate Encoder:** select V or I
   
   - **Press Encoder:** confirm selection
   
     
   ![Main Parameter](/Screenshots/ui_main_parameter.JPG)

---

## 3. Web Interface (Front-End)

The UG Lab PSU Web UI is fully interactive. Open it in your browser and **use the menu** to navigate—no URL typing required.

### **Main Dashboard**

- Live readings: voltage, current, power, temperature

- Output toggle, CV/CC mode selection, quick presets

- Clickable controls and touch-friendly

**Demo placeholder:** [Open Live UI](https://[your_username].github.io/SmartPSU/)

### **Charts**

- Real-time graphs for V/I/P/T

- Auto-scaling and scrollable history

- Navigation via front-end menu

### **Settings**

- Adjust PID, limits, Wi-Fi, save/restore presets

- Menu-driven; no URL input needed

### **System Info**

- Firmware version, OTA updates, project info

### **Wi-Fi Setup**

- AP/STA mode selection, network changes, and resets

- All interactive in the front-end

💡 **Tip:** All pages are accessible **through the UI navigation**. No URL typing required; everything is interactive from the main interface.

---

## 4. Safety & Protections

- Output cannot exceed **upper shoulder limit** (hardware potentiometer)

- MOSFET cuts off output on **overcurrent / overtemperature**

- INA226 disconnection → error display

- Errors are indicated on **OLED + LED** and in **Web UI**

Reset errors by pressing encoder or cycling power.

---

## 5. Quick Tests

| Step | Action                                       | Expected Result                        |
| ---- | -------------------------------------------- | -------------------------------------- |
| 1    | Power on ESP32 only                          | OLED + LED pulse, no output            |
| 2    | Apply PSU input                              | Display shows 0 V / 0 A                |
| 3    | Adjust voltage via encoder or web            | Output changes smoothly                |
| 4    | Connect load                                 | Current reading appears, graphs update |
| 5    | Trigger overcurrent (test resistor overload) | Output cuts off, error shown           |

---

## 6. Advanced Features

- OTA firmware updates

- Modular code for **extensions**: new displays, sensors, loads

- Preset management via Web UI

- PID auto-tuning (future)
  

---

## 7. Debug Variables (Web)

The UG Lab PSU exposes key internal parameters for real-time monitoring and debugging.

💡 **Note:** These six `debugVars` are shown on the Web UI graphs. You can repurpose them anywhere in the code to monitor **any variable** in real time, not just PID or touch sensors.

### **Selecting Debug Mode**

* `dbgMode = 0` → All debug outputs disabled
* `dbgMode = 1` → Touch UI debug variables
* `dbgMode = 2` → Voltage PID debug
* `dbgMode = 3` → Current PID debug

You can choose the debug mode on Config page in the WEB UI. The graphs will update according to the selected mode.

#### **Touch UI (dbgMode = 1)**

| Variable       | Description                   |
| -------------- | ----------------------------- |
| `debugVars[0]` | TOUCH1_PAD raw value          |
| `debugVars[1]` | TOUCH1_PAD diff from baseline |
| `debugVars[2]` | TOUCH1_PAD threshold          |
| `debugVars[3]` | TOUCH2_PAD raw value          |
| `debugVars[4]` | TOUCH2_PAD diff from baseline |
| `debugVars[5]` | TOUCH2_PAD threshold          |

#### **Voltage PID (dbgMode = 2)**

| Variable       | Description                      |
| -------------- | -------------------------------- |
| `debugVars[0]` | PID PWM output (%)               |
| `debugVars[1]` | Voltage integral term            |
| `debugVars[2]` | Previous voltage error           |
| `debugVars[3]` | RMS deviation of voltage         |
| `debugVars[4]` | Peak voltage deviation           |
| `debugVars[5]` | Voltage derivative (rate change) |

#### **Current PID (dbgMode = 3)**

| Variable       | Description                      |
| -------------- | -------------------------------- |
| `debugVars[0]` | PID PWM output (%)               |
| `debugVars[1]` | Current integral term            |
| `debugVars[2]` | Previous current error           |
| `debugVars[3]` | RMS deviation of current         |
| `debugVars[4]` | Peak current deviation           |
| `debugVars[5]` | Current derivative (rate change) |

> 💡 Updated periodically. You can connect these variables to any calculation or sensor in your code and monitor them live on the graphs. Perfect for PID tuning, sensor checks, or custom experiments.

---

  
  

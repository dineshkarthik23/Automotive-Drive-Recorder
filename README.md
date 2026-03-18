# Automotive Drive Recorder (Black Box)

An IoT-enabled Event Data Recorder system for low-end vehicles designed to monitor vehicle behavior, detect accidents, and provide real-time insights through cloud logging and mobile application.

## Project Overview

- The proposed system uses an **ESP32** as the central controller for continuous vehicular data telemetry acquisition, event detection, and resilient local storage, with conditional cloud synchronization after critical incidents.

- The system logs key parameters every 3 seconds, including speed, acceleration during turns, vehicle angle, indicator and brake status, and GPS location with timestamp.

- Upon detecting an accident or near-crash, it automatically uploads an **incident package** to the cloud containing pre- and post-event sensor data, enabling accurate reconstruction and analysis.
  
### Key Features

- **Real-Time Crash Detection**: Detects abnormal acceleration and vehicle impacts using MPU6050 accelerometer
- **GPS Tracking**: Captures precise location coordinates (latitude/longitude) with timestamp during incidents
- **Speed Monitoring**: Calculates vehicle speed using Hall-effect sensor on wheel rotation
- **Driving Behavior Analysis**: Logs acceleration, braking, and indicator status continuously
- **Cloud Synchronization**: Automatic data upload to Adafruit IO cloud dashboard
- **Offline Storage**: CSV data logging to SD card for forensic analysis and legal evidence
- **Mobile Dashboard**: Flutter app for real-time monitoring and incident review
- **Wireless Communication**: ESP-NOW protocol for brake and indicator status from secondary node

## System Architecture

### Hardware Components

| Component | Model | Purpose |
|-----------|-------|---------|
| **Microcontroller** | ESP32 DevKit v4 | Central processing unit with WiFi/BLE |
| **Accelerometer** | MPU6050 | 3-axis acceleration and temperature sensing |
| **GPS Module** | NEO-6M | Real-time location acquisition |
| **Storage** | SD Card Module | Local CSV data logging |
| **Speed Sensor** | Hall Effect Sensor | Vehicle speed calculation via wheel rotation |

### Sensor Connections (ESP32 Pinout)

```
I2C Interface (for MPU6050):
  - GPIO 21 (SDA) → MPU6050 SDA
  - GPIO 22 (SCL) → MPU6050 SCL

SPI Interface (for SD Card):
  - GPIO 23 (MOSI) → SD Module
  - GPIO 19 (MISO) → SD Module
  - GPIO 18 (SCK) → SD Module
  - GPIO 5 (CS) → SD Module

Input Pins:
  - GPIO 34 (Hall Pin) → Speed sensor
  
Serial Communication:
  - TX/RX for debugging and serial monitor
```

## Software Implementation

### Arduino Firmware (sensor_esp_1.ino)

The primary firmware handles:

1. **Sensor Data Acquisition**
   - MPU6050: 16-bit acceleration data (±16g range)
   - Temperature compensation from accelerometer
   - Low-pass filtering (α = 0.2) for noise reduction

2. **Crash Detection Algorithm**
   - Monitors calculated G-force threshold (>1.2g)
   - Analyzes sudden deceleration patterns
   - Triggers incident package upload on detection

3. **Speed Calculation**
   - Interrupt-driven pulse counting from Hall sensor
   - Calibrated wheel circumference: 0.03m (wheel radius)
   - Speed updated every 1000ms interval

4. **GPS & Location Services**
   - WiFi-based geolocation using Google Geolocation API
   - Periodic updates every 5 seconds
   - Stores coordinates (latitude/longitude) with timestamp

5. **Cloud Integration**
   - Adafruit IO feeds for real-time dashboard:
     - `crash-acc-x`, `crash-acc-y`, `crash-acc-z` (acceleration)
     - `g-force` (calculated magnitude)
     - `crash-lat`, `crash-lon` (GPS coordinates)
     - `crash-speed` (vehicle velocity)
     - `crash-time` (timestamp)
     - `brake-status`, `indicator-status` (vehicle events)

6. **SD Card Data Logging**
   - CSV format: `timestamp, ax, ay, az, temp, speed, lat, lon, gforce, brake, indicator, crash`
   - Continuous parameter logging every 3 seconds
   - Incident packages include full context data

### Secondary Arduino (car_control.ino)

Handles brake and indicator status monitoring:
- ESP-NOW wireless protocol for low-latency communication
- Transmits brake and indicator status to primary ESP32
- Integrates with incident detection package

## Simulation Environment

### Wokwi Simulation

A Wokwi-based simulation projects (`Crashdetection-Wokwi-Simulation/`) provides:
- Virtual ESP32 DevKit C v4 environment
- Simulated MPU6050 sensor with realistic acceleration data
- Serial monitor output for debugging

**Simulated Components:**
- ESP32 microcontroller
- MPU6050 accelerometer with I2C interface
- Serial communication for data visualization

## Data Flow & Workflow

```
Sensors (MPU6050, GPS, Hall) 
    ↓
ESP32 Processing & Filtering
    ↓
Crash Detection Algorithm
    ├→ Normal Operation: Log to SD (CSV)
    └→ Incident Detected: Package data → Cloud Upload
    ↓
Adafruit IO Cloud Dashboard
    ↓
Flutter Mobile App (Real-time Monitoring)
    ↓
Forensic Analysis & Legal Evidence
```

## Performance Metrics

- **Sampling Rate**: Acceleration data every 10ms, aggregate every 3 seconds
- **GPS Update Interval**: Every 5 seconds
- **Speed Calculation**: Updated every 1000ms
- **Crash Detection Latency**: Real-time with G-force threshold monitoring
- **Cloud Sync**: Automatic upon incident detection
- **Storage**: SD card continuous CSV logging capacity ~1GB for ~30 days of normal operation

## Getting Started

### Hardware Setup

1. Assemble the circuit according to the pinout diagram:
   - Connect MPU6050 to I2C (GPIO 21/22)
   - Connect SD card module to SPI (GPIO 18/19/23/5)
   - Connect Hall sensor to GPIO 34
   - Connect power and ground appropriately

2. Install PlatformIO or Arduino IDE with ESP32 board support

3. Upload `sensor_esp_1.ino` to the primary ESP32

4. Upload `car_control.ino` to the secondary ESP32 (optional, for brake/indicator monitoring)

### Software Setup

1. Install Flutter and dependencies
2. Configure Adafruit IO credentials in the Arduino firmware
3. Configure WiFi SSID and password in the code
4. Build and deploy the Flutter app to your mobile device

### Cloud Configuration

1. Create an Adafruit IO account and feeds for each sensor reading
2. Update `IO_USERNAME` and `IO_KEY` in the Arduino code
3. Configure Google Geolocation API key for location services

## File Structure

```
black_box/
├── lib/
│   ├── main.dart
│   └── screens/
│       └── settings_page.dart
├── Arduino/
│   ├── sensor_esp_1.ino (Primary sensor node)
│   └── car_control.ino (Secondary control node)
├── Crashdetection-Wokwi-Simulation/
│   ├── diagram.json (Circuit schematic)
│   ├── wokwi-project.txt (Project metadata)
│   └── wifi-scan.ino (WiFi simulation code)
├── android/, ios/, web/, windows/, macos/, linux/
└── pubspec.yaml
```

## References

[1] Khare, Sameer, and K. P. Peeyush. "Automotive drive recorder as black box for low end vehicles." In 2017 International Conference on Advances in Computing, Informatics and Informatics (ICACCI), pp. 1855-1861. IEEE, 2017.

[2] Ponnalar, A., B. Chandra, S. Aarthi, G. K. R. Bhavana, Arund A. Jose, and S. Gomathi. "IoT Based Automotive Drive Recorder As Black Box." In 2022 International Conference on Computer, Power and Communications (ICCPC), pp. 557-561. IEEE, 2022.

[3] Jegan, J., M. Raja Suguna, M. Shobana, H. Azath, S. Murugan, and M. Rajmohan. "IoT-Enabled Black Box For Driver Behavior Analysis Using Cloud Computing." In 2024 International Conference on Data Engineering and Intelligent Computing Systems (ADICS), pp. 1-6. IEEE, 2024.

## License

This project is part of academic research on automotive safety and IoT-based vehicle monitoring systems.


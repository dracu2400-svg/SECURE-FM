# Project 2: Advanced ML Fitness Tracker with GPS & Cloud

**Difficulty:** Advanced ⭐⭐⭐⭐⭐

**Duration:** 12-16 hours

**Hardware:** NRF52840-DK + X-Nucleo-IQS4A1 + SimCom A7672SA

---

## Project Overview

Build a **professional-grade fitness tracker** combining:
- ✅ **Machine Learning** (TensorFlow Lite Micro) - Activity classification
- ✅ **GPS Tracking** (SimCom A7672SA) - Location with 2.5m accuracy
- ✅ **4G Connectivity** (SimCom A7672SA) - Real-time cloud sync
- ✅ **Multi-Sensor Fusion** (X-Nucleo-IQS4A1) - IMU, magnetometer, pressure
- ✅ **BLE** (NRF52840) - Mobile app integration
- ✅ **TF-M Security** - End-to-end encryption

### What This Device Does

**Real-Time Activity & Location Tracking:**
1. Detects activity (walking, running, cycling, climbing stairs)
2. Tracks GPS location during activities
3. Counts steps with ML-enhanced accuracy
4. Calculates calories burned
5. Measures altitude changes (barometer)
6. Detects direction of travel (magnetometer)
7. Uploads to cloud via 4G LTE
8. Syncs with phone via BLE

**Example Output:**
```
🏃 Running detected (95% confident)
📍 Location: 37.7749°N, 122.4194°W
🔼 Altitude: 152m (climbing +5m)
🧭 Direction: NE (45°)
👟 Steps: 5,432 (+18 in last minute)
🔥 Calories: 342 kcal
📡 Uploaded to cloud ✓
```

---

## Why This Enhanced Version?

### vs Original Project 2

| Feature | Original | Enhanced |
|---------|----------|----------|
| **Activity Detection** | ✓ ML-based | ✓ ML-based |
| **GPS Tracking** | ✗ None | ✓ Real-time location |
| **Cloud Sync** | ✗ BLE only | ✓ 4G LTE direct |
| **Sensors** | LSM6DSO only | Full sensor suite |
| **Use Cases** | Activity tracker | Complete fitness tracker |
| **Battery Life** | 7 days | 3-5 days (with GPS) |

### Real-World Applications

✓ **Outdoor Fitness Tracking:** Running routes with GPS
✓ **Hiking Companion:** Altitude, direction, location
✓ **Cycling Computer:** Speed, distance, elevation
✓ **Emergency Beacon:** Send location if fallen (fall detection)
✓ **Asset Tracking:** Track valuable equipment
✓ **Field Worker Safety:** Monitor worker location & activity

---

## Hardware Bill of Materials

### Main Components

| Component | Description | Quantity | Price |
|-----------|-------------|----------|-------|
| **NRF52840-DK** | Nordic development kit | 1 | $40 |
| **X-Nucleo-IQS4A1** | Multi-sensor expansion | 1 | $30 |
| **SimCom A7672SA** | 4G LTE + GPS module | 1 | $25 |
| **GPS Antenna** | External active antenna | 1 | $8 |
| **4G LTE Antenna** | External antenna | 1 | $6 |
| **LiPo Battery** | 3.7V 2000mAh | 1 | $12 |
| **Breakout Board** | For A7672SA | 1 | $5 |
| **Case** | 3D-printed enclosure | 1 | Free |

**Total:** ~$126

### Sensors Included

**X-Nucleo-IQS4A1 Multi-Sensor Board:**
- ✅ **LSM6DSO:** 6-axis IMU (accelerometer + gyroscope)
- ✅ **LIS2MDL:** 3-axis magnetometer (compass)
- ✅ **LPS22HH:** Barometric pressure sensor (altitude)
- ✅ **STTS751:** Temperature sensor

**SimCom A7672SA Module:**
- ✅ **GPS/GLONASS/BeiDou/Galileo:** Multi-constellation positioning
- ✅ **4G LTE Cat-1:** 10 Mbps down, 5 Mbps up
- ✅ **Fallback:** 3G/2G support

---

## Hardware Connections

### Pin Mapping

```
NRF52840-DK Connections:
┌──────────────────────────────────────────────────────────┐
│                                                          │
│  I2C Bus (X-Nucleo-IQS4A1):                            │
│  ├─ P0.26 (SCL) ──────→ IQS4A1 SCL                     │
│  ├─ P0.27 (SDA) ──────→ IQS4A1 SDA                     │
│  ├─ P0.28 ────────────→ IQS4A1 INT1 (IMU interrupt)    │
│  └─ P0.29 ────────────→ IQS4A1 INT2 (Pressure ready)   │
│                                                          │
│  UART (SimCom A7672SA):                                 │
│  ├─ P0.05 (TX) ────────→ A7672SA RXD                   │
│  ├─ P0.06 (RX) ←────────  A7672SA TXD                  │
│  ├─ P0.07 ────────────→ A7672SA PWRKEY                 │
│  └─ P0.08 ←────────────  A7672SA STATUS                │
│                                                          │
│  Power:                                                  │
│  ├─ VDD (3.3V) ────────→ IQS4A1 VDD                    │
│  ├─ 5V (VBUS) ─────────→ A7672SA VCC (via regulator)   │
│  └─ GND ───────────────→ Common ground                  │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

### Wiring Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                      NRF52840-DK                                │
│  ┌──────────────────────────────────────────────────┐          │
│  │  Cortex-M4F @ 64MHz                              │          │
│  │  BLE 5.0, 1MB Flash, 256KB RAM                   │          │
│  └──────────────────────────────────────────────────┘          │
│         │                    │                    │             │
│         │ I2C                │ UART               │ BLE 5.0     │
│         ↓                    ↓                    ↓             │
│  ┌──────────────┐   ┌──────────────┐   ┌──────────────┐       │
│  │ X-Nucleo     │   │ SimCom       │   │ Phone App    │       │
│  │ IQS4A1       │   │ A7672SA      │   │ (iOS/Android)│       │
│  ├──────────────┤   ├──────────────┤   └──────────────┘       │
│  │ LSM6DSO IMU  │   │ 4G LTE Cat-1 │                          │
│  │ LIS2MDL Mag  │   │ GPS/GLONASS  │                          │
│  │ LPS22HH Baro │   │ HTTP/HTTPS   │                          │
│  │ STTS751 Temp │   │ TCP/IP       │                          │
│  └──────────────┘   └──────────────┘                          │
│                              │                                  │
│                              │ 4G LTE                          │
│                              ↓                                  │
│                     ┌──────────────┐                           │
│                     │ Cloud Server │                           │
│                     │ (AWS/Azure)  │                           │
│                     └──────────────┘                           │
└─────────────────────────────────────────────────────────────────┘
```

---

## System Architecture

### Data Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                   Sensor Data Collection                         │
└─────────────────────────────────────────────────────────────────┘
         │
         ├─→ LSM6DSO: Accel + Gyro (100 Hz)
         ├─→ LIS2MDL: Magnetometer (50 Hz)
         ├─→ LPS22HH: Pressure (10 Hz)
         └─→ GPS: Location (1 Hz)
         │
         ↓
┌─────────────────────────────────────────────────────────────────┐
│              ML Activity Classification (TFLite)                 │
│  Input: 2-second window (200 samples × 6 axes)                  │
│  Output: Activity probabilities [Walk, Run, Cycle, Stairs, ...]│
└─────────────────────────────────────────────────────────────────┘
         │
         ↓
┌─────────────────────────────────────────────────────────────────┐
│                  Activity + Location Fusion                      │
│  • Activity: Running (95%)                                      │
│  • Location: 37.7749°N, 122.4194°W                             │
│  • Altitude: 152m                                                │
│  • Direction: NE (45°)                                          │
│  • Speed: 12 km/h                                               │
└─────────────────────────────────────────────────────────────────┘
         │
         ├─→ Local Storage (TF-M PS): Historical data
         ├─→ BLE: Real-time to phone app
         └─→ 4G Cloud: Upload every 5 minutes
```

### TF-M Security Integration

```
┌──────────────────────────────────────────────────────────────┐
│                    Secure World (TF-M)                        │
│  ┌────────────┬────────────┬────────────┬─────────────┐     │
│  │ PSA Crypto │ PSA ITS    │ PSA PS     │ Attestation │     │
│  ├────────────┼────────────┼────────────┼─────────────┤     │
│  │ Encrypt    │ Store:     │ Store:     │ Prove       │     │
│  │ location   │ • API keys │ • Activity │ device      │     │
│  │ data       │ • APN      │   history  │ identity    │     │
│  │            │ • Certs    │ • Location │             │     │
│  │            │ • ML model │   history  │             │     │
│  └────────────┴────────────┴────────────┴─────────────┘     │
└──────────────────────────────────────────────────────────────┘
         │
         │ Secure API Calls
         ↓
┌──────────────────────────────────────────────────────────────┐
│                   Non-Secure World (App)                      │
│  ┌──────────────────────────────────────────────────────┐   │
│  │         Application Logic                             │   │
│  ├──────────┬──────────┬──────────┬──────────┬─────────┤   │
│  │ ML       │ GPS      │ Sensor   │ BLE      │ 4G      │   │
│  │ Inference│ Parser   │ Fusion   │ Service  │ Upload  │   │
│  └──────────┴──────────┴──────────┴──────────┴─────────┘   │
└──────────────────────────────────────────────────────────────┘
```

---

## Software Components

### 1. Sensor Drivers (Reused from Project 1)

**LSM6DSO Driver** (`drivers/lsm6dso_driver.c`):
- Already implemented in Project 1
- Provides: Accel, gyro, step counter, activity hints
- TF-M integration: Calibration in ITS

**New: LIS2MDL Magnetometer Driver** (`drivers/lis2mdl_driver.c`):
```c
/**
 * @brief Read magnetometer data
 * @param data Output: Magnetic field in mGauss
 */
LIS2MDL_Status_t LIS2MDL_ReadMag(LIS2MDL_MagData_t *data)
{
    uint8_t buffer[6];

    // Read X, Y, Z magnetic field (signed 16-bit)
    i2c_read(LIS2MDL_ADDR, LIS2MDL_OUTX_L_REG, buffer, 6);

    int16_t mag_x = (buffer[1] << 8) | buffer[0];
    int16_t mag_y = (buffer[3] << 8) | buffer[2];
    int16_t mag_z = (buffer[5] << 8) | buffer[4];

    // Convert to mGauss (sensitivity: 1.5 mGauss/LSB)
    data->mag_x = mag_x * 1.5f;
    data->mag_y = mag_y * 1.5f;
    data->mag_z = mag_z * 1.5f;

    // Calculate heading (0-360°)
    data->heading = atan2f(data->mag_y, data->mag_x) * 180.0f / M_PI;
    if (data->heading < 0) data->heading += 360.0f;

    return LIS2MDL_OK;
}
```

**New: LPS22HH Barometer Driver** (`drivers/lps22hh_driver.c`):
```c
/**
 * @brief Read pressure and calculate altitude
 */
LPS22HH_Status_t LPS22HH_ReadAltitude(float *altitude_m)
{
    uint32_t pressure_raw;
    i2c_read(LPS22HH_ADDR, LPS22HH_PRESS_OUT_XL, &pressure_raw, 3);

    // Convert to hPa
    float pressure_hpa = pressure_raw / 4096.0f;

    // Calculate altitude (standard atmosphere)
    *altitude_m = 44330.0f * (1.0f - powf(pressure_hpa / 1013.25f, 0.1903f));

    return LPS22HH_OK;
}
```

**SimCom A7672SA Driver** (`drivers/simcom_a7672sa_driver.c`):
- Already implemented in Project 1
- Provides: GPS, 4G LTE, HTTP/HTTPS
- TF-M integration: APN credentials in ITS

### 2. ML Activity Classification

**Enhanced Model with GPS Context:**

```python
# ml_training/enhanced_activity_classifier.ipynb

import tensorflow as tf
import numpy as np

# Model inputs:
# - 6-axis IMU (accel + gyro): 200 samples
# - GPS speed: 1 value
# - Altitude change: 1 value
# - Heading: 1 value

model = tf.keras.Sequential([
    # IMU feature extraction
    tf.keras.layers.Input(shape=(200, 6)),
    tf.keras.layers.Conv1D(32, 3, activation='relu'),
    tf.keras.layers.MaxPooling1D(2),
    tf.keras.layers.Conv1D(64, 3, activation='relu'),
    tf.keras.layers.GlobalAveragePooling1D(),

    # Concatenate with GPS features
    tf.keras.layers.Concatenate(),  # Add GPS speed, alt, heading

    # Classification
    tf.keras.layers.Dense(128, activation='relu'),
    tf.keras.layers.Dropout(0.3),
    tf.keras.layers.Dense(7, activation='softmax')  # 7 activities
])

# Activities:
# 0: Standing
# 1: Walking
# 2: Running
# 3: Cycling
# 4: Climbing stairs
# 5: Descending stairs
# 6: Driving
```

**GPS-Enhanced Classification:**
- **Walking:** GPS speed < 8 km/h, IMU shows walking pattern
- **Running:** GPS speed 8-20 km/h, high IMU activity
- **Cycling:** GPS speed > 15 km/h, smooth IMU (less steps)
- **Driving:** GPS speed > 30 km/h, minimal IMU movement
- **Climbing stairs:** Altitude increasing, step pattern
- **Descending stairs:** Altitude decreasing, step pattern

### 3. Sensor Fusion Engine

```c
/**
 * @brief Fuse IMU, GPS, magnetometer, and barometer data
 */
typedef struct {
    // Activity from ML
    ActivityType_t activity;
    float activity_confidence;

    // Location from GPS
    float latitude;
    float longitude;
    float gps_speed_kmh;
    float gps_course;

    // Altitude from barometer
    float altitude_m;
    float altitude_change_m;  // vs 1 minute ago

    // Heading from magnetometer
    float heading_degrees;

    // Derived metrics
    uint32_t steps;
    uint16_t calories_kcal;
    float distance_km;

    // Quality indicators
    uint8_t gps_satellites;
    bool gps_fix_valid;

} FusedData_t;

void sensor_fusion_update(FusedData_t *fused)
{
    // 1. Read all sensors
    LSM6DSO_Data_t imu;
    LSM6DSO_ReadData(&imu);

    LIS2MDL_MagData_t mag;
    LIS2MDL_ReadMag(&mag);

    float altitude;
    LPS22HH_ReadAltitude(&altitude);

    A7672_GPSData_t gps;
    A7672_GPSReadData(&gps);

    // 2. Run ML inference on IMU data
    fused->activity = ml_classify_activity(&imu);

    // 3. Validate with GPS
    if (gps.speed_kmh > 30 && fused->activity != ACTIVITY_DRIVING) {
        // Override: Likely driving
        fused->activity = ACTIVITY_DRIVING;
    }

    // 4. Update altitude change
    fused->altitude_change_m = altitude - fused->altitude_m;
    fused->altitude_m = altitude;

    // 5. Update heading
    fused->heading_degrees = mag.heading;

    // 6. Calculate calories
    fused->calories_kcal = calculate_calories(
        fused->activity,
        HAL_GetTick() - last_update_time
    );
}
```

---

## Cloud Integration

### Data Upload Format

```json
{
  "device_id": "FITNESS-TRACKER-001",
  "timestamp": 1700000000,
  "session": {
    "activity": "running",
    "confidence": 0.95,
    "duration_s": 1800,
    "location": {
      "start": {"lat": 37.7749, "lon": -122.4194},
      "end": {"lat": 37.7850, "lon": -122.4100},
      "path": [
        {"lat": 37.7749, "lon": -122.4194, "alt": 10, "time": 0},
        {"lat": 37.7760, "lon": -122.4180, "alt": 12, "time": 60},
        ...
      ]
    },
    "metrics": {
      "steps": 2400,
      "distance_km": 3.2,
      "calories_kcal": 245,
      "avg_speed_kmh": 10.2,
      "max_altitude_m": 25,
      "elevation_gain_m": 15
    }
  },
  "encrypted": true,
  "hmac": "3F8D2C7E1A5F9B4D8C3E7A1F..."
}
```

### Upload Strategy

**Real-Time (BLE to Phone):**
- Update every 1 second
- Low-latency for live tracking
- Battery efficient (BLE)

**Batch Upload (4G to Cloud):**
- Upload every 5 minutes
- Or after activity session ends
- Includes full GPS track
- Encrypted payload

---

## Power Management

### Power Budget

| Component | Active | Sleep | Duty Cycle | Avg Power |
|-----------|--------|-------|------------|-----------|
| NRF52840 (ML) | 15 mA | 5 µA | 20% | 3.0 mA |
| LSM6DSO IMU | 0.5 mA | 3 µA | 100% | 0.5 mA |
| LIS2MDL Mag | 0.2 mA | 1 µA | 10% | 0.02 mA |
| LPS22HH Baro | 0.1 mA | 1 µA | 10% | 0.01 mA |
| GPS (acquiring) | 30 mA | - | 10% | 3.0 mA |
| 4G (idle) | 3 mA | - | 100% | 3.0 mA |
| 4G (transmit) | 500 mA | - | 1% | 5.0 mA |
| **TOTAL** | | | | **~14.5 mA** |

**Battery Life Calculation:**
```
2000 mAh / 14.5 mA = 138 hours ≈ 5.7 days

With optimizations:
- GPS only during activity: 3-5 days
- GPS + 4G aggressive: 2-3 days
```

### Power Optimization Strategies

**1. Motion-Based Wake:**
```c
// Sleep until motion detected
LSM6DSO_ConfigureMotionDetect(50, 0);  // 50mg threshold
__WFI();  // Wait for interrupt

// Motion detected → Start GPS & ML
```

**2. Adaptive GPS:**
```c
// GPS update rate based on activity
if (activity == ACTIVITY_RUNNING) {
    gps_update_rate = 1;  // 1 Hz (every second)
} else if (activity == ACTIVITY_WALKING) {
    gps_update_rate = 5;  // 0.2 Hz (every 5 seconds)
} else {
    gps_power_off();  // Stationary
}
```

**3. Cloud Upload Batching:**
```c
// Store locally, upload in batches
if (activity_ended || (time_since_upload > 300)) {
    upload_to_cloud();  // Every 5 min or end of activity
}
```

---

## Use Case Examples

### Use Case 1: Trail Running

**Scenario:** User goes for a 5km mountain trail run

**Device Behavior:**
1. Detects motion (ML: Running detected)
2. Starts GPS tracking
3. Records elevation changes (barometer)
4. Tracks heading changes (magnetometer)
5. Counts steps, calculates calories
6. Stores track locally (TF-M PS)
7. Uploads to cloud when finished

**Output:**
```
🏃 Trail Running Session
📅 2025-11-22 14:30 - 15:15 (45 min)
📍 Start: 37.8651°N, 119.5383°W (Yosemite Valley)
📍 End: 37.8712°N, 119.5402°W (Mirror Lake)
📏 Distance: 5.2 km
👟 Steps: 6,840
🔼 Elevation Gain: 180m (Max: 1,320m)
🔥 Calories: 425 kcal
⚡ Avg Pace: 8:40 /km
💨 Avg Speed: 6.9 km/h
```

### Use Case 2: Cycling Commute

**Scenario:** Daily bike commute with route tracking

**Device Behavior:**
1. ML detects cycling (GPS speed + low step count)
2. Tracks route with 1Hz GPS
3. Monitors altitude (hills)
4. Detects direction changes (turns)
5. Uploads commute data to cloud
6. Phone app shows live location

### Use Case 3: Hiking with Navigation

**Scenario:** Multi-hour hike with real-time tracking

**Device Behavior:**
1. GPS tracks position every 5 seconds
2. Barometer shows altitude profile
3. Magnetometer shows heading
4. Fall detection (sudden deceleration)
5. Emergency beacon if fallen

---

## Mobile App Integration

### BLE Services

**Fitness Service (Custom UUID: 0x181D)**

| Characteristic | UUID | Properties | Description |
|----------------|------|------------|-------------|
| Activity | 0x2A53 | Notify | Current activity (0-6) |
| Location | 0x2A54 | Notify | Lat/Lon (8 bytes) |
| Altitude | 0x2A55 | Notify | Altitude (2 bytes) |
| Heading | 0x2A56 | Notify | Compass heading (2 bytes) |
| Steps | 0x2A57 | Read, Notify | Step count (4 bytes) |
| Calories | 0x2A58 | Read, Notify | Calories (2 bytes) |
| Control | 0x2A59 | Write | Start/Stop tracking |

### React Native App Example

```javascript
// mobile_app/src/FitnessTracker.js

import { BleManager } from 'react-native-ble-plx';

const FITNESS_SERVICE = '181D';
const LOCATION_CHAR = '2A54';

// Subscribe to location updates
device.monitorCharacteristicForService(
    FITNESS_SERVICE,
    LOCATION_CHAR,
    (error, characteristic) => {
        if (characteristic) {
            // Parse lat/lon (4 bytes each, float)
            const buffer = base64.decode(characteristic.value);
            const latitude = buffer.readFloatLE(0);
            const longitude = buffer.readFloatLE(4);

            console.log(`Location: ${latitude}, ${longitude}`);
            updateMapMarker(latitude, longitude);
        }
    }
);
```

---

## Building the Project

### Step 1: Hardware Assembly

**Materials Needed:**
- NRF52840-DK board
- X-Nucleo-IQS4A1 sensor board
- SimCom A7672SA module breakout
- Jumper wires
- Breadboard (for prototyping)

**Assembly:**
1. Connect X-Nucleo-IQS4A1 to NRF52840 I2C pins
2. Connect SimCom A7672SA to NRF52840 UART pins
3. Wire power (5V for A7672SA, 3.3V for sensors)
4. Attach GPS and 4G antennas to A7672SA
5. Connect LiPo battery

**Test Points:**
- Verify I2C communication: `i2cdetect`
- Test UART: Send AT commands
- Check GPS fix: `AT+CGPSINFO`

### Step 2: Flash Firmware

```bash
cd project_02_nrf52840_tracker/firmware/
make clean
make -j8
make flash
```

### Step 3: Collect Training Data

```bash
# Start data collection mode
make flash_datacollect

# Perform activities:
# - Stand still for 5 minutes
# - Walk for 5 minutes
# - Run for 5 minutes
# - Cycle for 5 minutes
# - Climb stairs for 5 minutes

# Download data
python3 scripts/download_training_data.py \
    --port /dev/ttyACM0 \
    --output training_data.csv
```

### Step 4: Train ML Model

```bash
cd ml_training/
jupyter notebook enhanced_activity_classifier.ipynb

# Train model, convert to TFLite
# Output: activity_model.tflite (~20KB)
```

### Step 5: Deploy Model

```bash
# Convert TFLite to C array
xxd -i activity_model.tflite > ../firmware/src/ml/model_data.h

# Rebuild firmware with model
cd ../firmware/
make clean && make && make flash
```

### Step 6: Test Complete System

```bash
# Open serial monitor
screen /dev/ttyACM0 115200

# Expected output:
[BOOT] Fitness Tracker v2.0
[INIT] ✓ Sensors initialized
[INIT] ✓ GPS ready
[INIT] ✓ 4G connected
[INIT] ✓ ML model loaded
[BLE] Advertising...
[READY] Waiting for activity...

# Start moving:
[ML] Activity: Walking (92%)
[GPS] Location: 37.7749, -122.4194
[GPS] Altitude: 10m
[MAG] Heading: 45° (NE)
[UPLOAD] ✓ Data sent to cloud
```

---

## Advanced Features

### Feature 1: Fall Detection

```c
/**
 * @brief Detect falls using accelerometer
 */
bool detect_fall(LSM6DSO_Data_t *imu)
{
    // Calculate total acceleration magnitude
    float accel_mag = sqrtf(
        imu->accel_x * imu->accel_x +
        imu->accel_y * imu->accel_y +
        imu->accel_z * imu->accel_z
    );

    // Fall: sudden drop below 0.5g (free-fall)
    // Then impact > 3g
    static bool freefall_detected = false;

    if (accel_mag < 4.9f) {  // < 0.5g
        freefall_detected = true;
    } else if (freefall_detected && accel_mag > 29.4f) {  // > 3g impact
        freefall_detected = false;

        // FALL DETECTED!
        send_emergency_alert();
        return true;
    }

    return false;
}
```

### Feature 2: Route Recommendation

```c
/**
 * @brief Recommend routes based on past activities
 */
void recommend_route(void)
{
    // Load historical routes from TF-M PS
    RouteHistory_t history[100];
    psa_ps_get(ROUTE_HISTORY_UID, 0, sizeof(history), &history, &len);

    // Find routes with similar:
    // - Distance
    // - Elevation gain
    // - Activity type

    // ML model predicts user preference
    float scores[100];
    ml_predict_route_preference(history, scores);

    // Display top 3 recommendations on phone app
}
```

### Feature 3: Weather Integration

```c
/**
 * @brief Get current weather from cloud
 */
void get_weather_forecast(float lat, float lon)
{
    char url[256];
    snprintf(url, sizeof(url),
             "https://api.weather.com/forecast?lat=%.6f&lon=%.6f",
             lat, lon);

    char response[1024];
    A7672_HTTPGet(url, response, sizeof(response));

    // Parse weather
    // Adjust activity recommendations
}
```

---

## Success Metrics

### Performance Targets

| Metric | Target | Achieved |
|--------|--------|----------|
| Activity Accuracy | >90% | TBD |
| GPS Accuracy | <5m | 2.5m (A7672SA) |
| Battery Life | >3 days | TBD |
| ML Inference Time | <20ms | TBD |
| Cloud Upload Success | >95% | TBD |
| BLE Range | >10m | TBD |

---

## Troubleshooting

### GPS Not Getting Fix

**Problem:** GPS shows "No fix" after 5 minutes

**Solutions:**
1. Move outdoors (needs clear sky view)
2. Attach external GPS antenna
3. Wait longer (cold start: 30-60s)
4. Check antenna connection

```c
// Debug GPS status
A7672_GPSData_t gps;
if (A7672_GPSReadData(&gps) == A7672_ERROR_NO_FIX) {
    printf("GPS: No fix (Satellites: %d)\n", gps.satellites);
    // Need 4+ satellites for fix
}
```

### High Power Consumption

**Problem:** Battery drains in <1 day

**Check:**
- GPS constantly on?
- 4G transmitting continuously?
- ML inference too frequent?

**Debug:**
```c
#define POWER_PROFILE_ENABLE  1
// Logs: "GPS: 30mA for 5s = 150mJ"
```

### 4G Connection Fails

**Problem:** Can't connect to cellular network

**Solutions:**
1. Check SIM card inserted
2. Verify APN settings
3. Check signal strength (RSSI)
4. Try different network mode (4G → 3G)

---

## Project Deliverables

### Week 1: Hardware Integration ✓
- [x] Assemble all hardware
- [x] Test I2C sensors
- [x] Test GPS module
- [x] Test 4G connectivity
- [x] Collect initial sensor data

### Week 2: ML Development
- [ ] Collect training data (25+ min)
- [ ] Train TensorFlow model
- [ ] Convert to TFLite
- [ ] Deploy to NRF52840
- [ ] Validate accuracy >90%

### Week 3: Application Development
- [ ] Implement sensor fusion
- [ ] GPS + ML integration
- [ ] BLE service
- [ ] Cloud upload
- [ ] Mobile app

### Week 4: TF-M Security
- [ ] Encrypt location data
- [ ] Secure credential storage
- [ ] Attestation
- [ ] Final testing
- [ ] Documentation

---

## Next Steps

1. ✅ Assemble hardware
2. ✅ Test each sensor individually
3. ✅ Collect training data
4. ✅ Train ML model
5. ✅ Deploy firmware
6. ✅ Build mobile app
7. ✅ Test end-to-end

---

## Resources

- [NRF52840 Documentation](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fstruct_nrf52%2Fstruct%2Fnrf52840.html)
- [X-Nucleo-IQS4A1 User Manual](https://www.st.com/resource/en/user_manual/um2470-getting-started-with-the-xnucleoiqs4a1-multichannel-environmental-sensor-expansion-board-based-on-piezoelectric-sensing-technology-for-stm32-nucleo-stmicroelectronics.pdf)
- [SimCom A7672SA Datasheet](https://www.simcom.com/product/A7672SA.html)
- [TensorFlow Lite Micro](https://www.tensorflow.org/lite/microcontrollers)

---

**Project Status:** Enhanced with GPS + Multi-Sensor
**Estimated Time:** 12-16 hours
**Difficulty:** Advanced (ML + GPS + Multi-sensor)
**Fun Factor:** ⭐⭐⭐⭐⭐ (Maximum!)

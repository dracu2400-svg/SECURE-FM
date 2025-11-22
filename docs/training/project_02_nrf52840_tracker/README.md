# Project 2: NRF52840 ML Activity Tracker

**Difficulty:** Advanced ⭐⭐⭐⭐☆

**Duration:** 10-12 hours

**Hardware:** NRF52840-DK + LSM6DSO IMU

---

## Project Overview

Build an intelligent activity tracker using **Machine Learning** (TensorFlow Lite Micro) to classify user activities in real-time, secured by TF-M (ARM TrustZone-M).

### What It Does

1. **Collects Motion Data:** IMU sensor (accelerometer + gyroscope)
2. **Classifies Activity:** On-device ML model identifies:
   - Walking
   - Running
   - Cycling
   - Standing still
   - Climbing stairs
3. **Counts Steps:** Pedometer algorithm
4. **Tracks Calories:** Based on activity and duration
5. **Sends to Phone:** BLE connection to mobile app
6. **Secure Storage:** TF-M protects user data

---

## Why This Project?

### Educational Value

✓ **TensorFlow Lite Micro:** On-device ML for embedded systems
✓ **Real-Time Inference:** <10ms latency on Cortex-M4
✓ **BLE Security:** Encrypted communication with phone
✓ **TF-M Integration:** Secure ML model storage & execution
✓ **Power Optimization:** Run for days on coin cell battery
✓ **Complete Product:** From sensor to mobile app

### Real-World Skills

- Train ML models for embedded devices
- Optimize models for MCU constraints (RAM, Flash, CPU)
- Implement secure BLE protocols
- Design low-power systems
- Integrate multiple subsystems

---

## Hardware Requirements

### Main Components

| Component | Description | Price |
|-----------|-------------|-------|
| **NRF52840-DK** | Nordic Development Kit | $40 |
| **LSM6DSO Breakout** | 6-axis IMU sensor board | $10 |
| **LiPo Battery** | 3.7V 500mAh (optional) | $8 |
| **Case** | 3D-printed enclosure | Free |

**Total:** ~$58 (without battery/case)

### Pin Connections

```
NRF52840-DK                LSM6DSO Breakout
┌─────────────┐            ┌──────────────┐
│ P0.26 (SCL) ├────────────┤ SCL          │
│ P0.27 (SDA) ├────────────┤ SDA          │
│ P0.28       ├────────────┤ INT1 (Motion)│
│ P0.29       ├────────────┤ INT2 (FIFO)  │
│ VDD (3.3V)  ├────────────┤ VDD          │
│ GND         ├────────────┤ GND          │
└─────────────┘            └──────────────┘
```

---

## Software Architecture

### System Diagram

```
┌─────────────────────────────────────────────────────┐
│                  NRF52840-DK                        │
│  ┌───────────────────────────────────────────────┐  │
│  │         Non-Secure World (Application)        │  │
│  │  ┌──────────┬──────────┬──────────┬─────────┐ │  │
│  │  │ BLE      │ ML       │ Activity │ UI      │ │  │
│  │  │ Service  │ Inference│ Tracker  │ (LEDs)  │ │  │
│  │  └──────────┴──────────┴──────────┴─────────┘ │  │
│  ├───────────────────────────────────────────────┤  │
│  │         TF-M Secure Services                  │  │
│  │  ┌──────────┬──────────┬──────────┬─────────┐ │  │
│  │  │ PSA      │ PSA      │ PSA ITS  │ PSA PS  │ │  │
│  │  │ Crypto   │ Attest   │ (Creds)  │ (Data)  │ │  │
│  │  └──────────┴──────────┴──────────┴─────────┘ │  │
│  └───────────────────────────────────────────────┘  │
│                                                      │
│  ┌───────────────────────────────────────────────┐  │
│  │         LSM6DSO IMU Sensor                    │  │
│  │  (Accelerometer + Gyroscope)                  │  │
│  └───────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
         │                              │
         │ BLE 5.0                      │ I2C
         ↓                              ↓
   ┌──────────┐                   ┌──────────┐
   │ Phone    │                   │ IMU      │
   │ App      │                   │ Sensor   │
   └──────────┘                   └──────────┘
```

---

## TF-M Integration

### Secure Services Used

#### 1. PSA Crypto
**Purpose:** Encrypt activity data before BLE transmission

```c
// Encrypt activity record
psa_aead_encrypt(
    key_id,
    PSA_ALG_GCM,
    nonce, sizeof(nonce),
    NULL, 0,
    (uint8_t*)&activity_data, sizeof(activity_data),
    encrypted, sizeof(encrypted),
    &encrypted_len
);
```

#### 2. Internal Trusted Storage (ITS)
**Purpose:** Store ML model weights securely

```c
// Store trained model
psa_its_set(
    ML_MODEL_UID,
    model_size,
    model_data,
    PSA_STORAGE_FLAG_WRITE_ONCE  // Prevent model tampering
);
```

#### 3. Protected Storage (PS)
**Purpose:** Store historical activity data

```c
// Save daily activity summary
psa_ps_set(
    ACTIVITY_HISTORY_UID,
    sizeof(daily_summary),
    &daily_summary,
    PSA_STORAGE_FLAG_NONE
);
```

#### 4. Initial Attestation
**Purpose:** Prove device authenticity to phone app

```c
// Generate attestation token
psa_initial_attest_get_token(
    challenge, challenge_size,
    token_buf, token_buf_size,
    &token_size
);

// Send over BLE to phone
ble_send_attestation_token(token_buf, token_size);
```

---

## Machine Learning Pipeline

### Phase 1: Data Collection (Week 1)

**Goal:** Collect training data

**Steps:**
1. Flash data collection firmware
2. Wear device and perform activities:
   - Walk for 5 minutes
   - Run for 5 minutes
   - Cycle for 5 minutes
   - Stand still for 5 minutes
   - Climb stairs for 5 minutes
3. Label data manually
4. Export to CSV

**Output:** 25 minutes of labeled sensor data

### Phase 2: Model Training (Week 2)

**Goal:** Train TensorFlow model

**Notebook:** `ml_training/activity_classifier.ipynb`

```python
import tensorflow as tf
import numpy as np
import pandas as pd

# Load data
data = pd.read_csv('sensor_data.csv')

# Preprocess
X = data[['accel_x', 'accel_y', 'accel_z', 'gyro_x', 'gyro_y', 'gyro_z']].values
y = data['activity'].values

# Create model
model = tf.keras.Sequential([
    tf.keras.layers.Input(shape=(100, 6)),  # 100 samples × 6 features
    tf.keras.layers.Conv1D(16, 3, activation='relu'),
    tf.keras.layers.MaxPooling1D(2),
    tf.keras.layers.Conv1D(32, 3, activation='relu'),
    tf.keras.layers.GlobalAveragePooling1D(),
    tf.keras.layers.Dense(64, activation='relu'),
    tf.keras.layers.Dropout(0.3),
    tf.keras.layers.Dense(5, activation='softmax')  # 5 activities
])

# Train
model.compile(optimizer='adam',
              loss='sparse_categorical_crossentropy',
              metrics=['accuracy'])

model.fit(X, y, epochs=50, validation_split=0.2)

# Convert to TFLite
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
tflite_model = converter.convert()

# Save
with open('activity_model.tflite', 'wb') as f:
    f.write(tflite_model)

print(f"Model size: {len(tflite_model)} bytes")
```

**Output:** `activity_model.tflite` (~15KB)

### Phase 3: Model Deployment (Week 2-3)

**Goal:** Deploy model to NRF52840

**Steps:**
1. Convert TFLite model to C array
2. Include in firmware
3. Load model into TFLite Micro interpreter
4. Run inference on new sensor data

**Code:**
```c
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "activity_model.h"  // Generated from .tflite

// Tensor arena (working memory for inference)
constexpr int kTensorArenaSize = 8 * 1024;  // 8KB
uint8_t tensor_arena[kTensorArenaSize];

// Setup interpreter
static tflite::MicroMutableOpResolver<5> resolver;
resolver.AddConv2D();
resolver.AddMaxPool2D();
resolver.AddFullyConnected();
resolver.AddSoftmax();
resolver.AddReshape();

static tflite::MicroInterpreter interpreter(
    model, resolver, tensor_arena, kTensorArenaSize);

interpreter.AllocateTensors();

// Get input tensor
TfLiteTensor* input = interpreter.input(0);

// Copy sensor data to input
for (int i = 0; i < 100; i++) {
    input->data.f[i * 6 + 0] = accel_x[i];
    input->data.f[i * 6 + 1] = accel_y[i];
    input->data.f[i * 6 + 2] = accel_z[i];
    input->data.f[i * 6 + 3] = gyro_x[i];
    input->data.f[i * 6 + 4] = gyro_y[i];
    input->data.f[i * 6 + 5] = gyro_z[i];
}

// Run inference
TfLiteStatus invoke_status = interpreter.Invoke();

// Get output
TfLiteTensor* output = interpreter.output(0);

// Find most likely activity
int max_idx = 0;
float max_prob = output->data.f[0];
for (int i = 1; i < 5; i++) {
    if (output->data.f[i] > max_prob) {
        max_prob = output->data.f[i];
        max_idx = i;
    }
}

// Print result
const char* activities[] = {"Walking", "Running", "Cycling", "Standing", "Stairs"};
printf("Activity: %s (%.1f%% confident)\n", activities[max_idx], max_prob * 100);
```

---

## BLE Protocol

### Service UUID: `0x180D` (Heart Rate Service - reused)

### Characteristics

| UUID | Name | Properties | Description |
|------|------|------------|-------------|
| `0x2A37` | Activity | Notify | Current activity (0-4) |
| `0x2A38` | Steps | Read, Notify | Step count (uint32_t) |
| `0x2A39` | Calories | Read, Notify | Calories burned (uint16_t) |
| `0x2A3A` | Control | Write | Commands (start/stop/reset) |

### Example: Phone Receives Activity Update

```javascript
// JavaScript (React Native app)
const device = await BleManager.connect(deviceId);

// Subscribe to activity notifications
await device.monitorCharacteristicForService(
    '180D',  // Service UUID
    '2A37',  // Activity characteristic
    (error, characteristic) => {
        if (characteristic) {
            const activity = characteristic.value;
            console.log('Activity:', ['Walking', 'Running', 'Cycling', 'Standing', 'Stairs'][activity]);
        }
    }
);
```

---

## Power Optimization

### Target: 7 days on 500mAh battery

**Power Budget:**
- Active (inference): 5mA @ 10% duty cycle = 0.5mA avg
- BLE advertising: 3mA @ 5% duty cycle = 0.15mA avg
- IMU: 0.5mA continuous
- Sleep: 5µA
- **Total Average:** ~1.2mA

**Battery Life Calculation:**
```
500mAh / 1.2mA = 416 hours = 17 days
```

**Optimizations:**
1. Use IMU wake-up interrupt (sleep until motion)
2. Run inference every 2 seconds (not continuously)
3. BLE connection interval: 1 second (energy efficient)
4. Power down IMU gyroscope when not needed

---

## Project Milestones

### Week 1: Hardware Bring-Up
- [ ] Assemble hardware
- [ ] Flash blinking LED test
- [ ] Verify IMU communication (I2C)
- [ ] Test BLE advertising
- [ ] Collect training data

### Week 2: ML Development
- [ ] Train TensorFlow model
- [ ] Convert to TFLite
- [ ] Integrate TFLite Micro
- [ ] Test inference on device
- [ ] Validate accuracy >90%

### Week 3: Application Development
- [ ] Implement step counter
- [ ] Add calorie calculation
- [ ] Create BLE service
- [ ] Test with phone app
- [ ] Add LED activity indicators

### Week 4: TF-M Integration
- [ ] Configure TrustZone-M
- [ ] Move sensitive data to secure world
- [ ] Encrypt BLE communication
- [ ] Implement attestation
- [ ] Final testing

---

## Success Metrics

### Model Performance
- **Accuracy:** >90% on test set
- **Inference Time:** <10ms
- **Model Size:** <20KB
- **RAM Usage:** <10KB

### System Performance
- **Step Accuracy:** ±3% error
- **Battery Life:** >7 days
- **BLE Range:** >10 meters
- **Latency:** <1 second update rate

---

## Deliverables

### 1. Firmware (`firmware/`)
- Main application
- TFLite Micro integration
- BLE service
- TF-M configuration

### 2. ML Model (`ml_training/`)
- Jupyter notebook
- Training data
- Trained model (.tflite)
- Accuracy metrics

### 3. Mobile App (`mobile_app/`)
- React Native app
- Real-time activity display
- Historical graphs
- Settings

### 4. Documentation
- This README
- Hardware assembly guide
- Software setup guide
- ML training guide

---

## Getting Started

### 1. Clone Repository

```bash
git clone https://github.com/your-repo/SECURE-FM.git
cd SECURE-FM/docs/training/project_02_nrf52840_tracker/
```

### 2. Install Dependencies

```bash
# NRF SDK
cd ~/
wget https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v17.x.x/nRF5_SDK_17.1.0_ddde560.zip
unzip nRF5_SDK_17.1.0_ddde560.zip

# TensorFlow Lite Micro
git clone https://github.com/tensorflow/tflite-micro.git

# ARM GCC Toolchain
sudo apt-get install gcc-arm-none-eabi
```

### 3. Build Firmware

```bash
cd firmware/
make
```

### 4. Flash

```bash
make flash
```

### 5. Test

```bash
# Open serial monitor
screen /dev/ttyACM0 115200

# Expected output:
[BOOT] Activity Tracker v1.0
[INIT] Initializing IMU...
[INIT] ✓ LSM6DSO found
[INIT] Loading ML model...
[INIT] ✓ Model loaded (15 KB)
[BLE] Advertising as "Activity-001"
[ML] Waiting for motion...
```

---

## Troubleshooting

### Model too large

**Error:** `Model size exceeds flash capacity`

**Solution:** Quantize model more aggressively
```python
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.target_spec.supported_types = [tf.int8]  # Use int8
```

### Low inference accuracy

**Causes:**
- Insufficient training data
- Model too simple
- Poor preprocessing

**Solution:**
- Collect more data (>30 min per activity)
- Try deeper model
- Normalize sensor values

### High power consumption

**Check:**
- IMU not sleeping
- BLE connection interval too short
- CPU not entering sleep mode

**Debug:**
```c
#define POWER_DEBUG  1  // Enable power profiling
```

---

## Next Steps

1. ✅ Complete hardware assembly
2. ✅ Run "Hello World" BLE example
3. ✅ Test IMU data collection
4. ✅ Train initial ML model
5. ✅ Deploy and test inference
6. ✅ Integrate TF-M security
7. ✅ Build mobile app
8. ✅ Final system integration

---

## Additional Resources

- [TensorFlow Lite Micro Guide](https://www.tensorflow.org/lite/microcontrollers)
- [NRF52840 Documentation](https://infocenter.nordicsemi.com/)
- [TF-M for NRF](https://docs.zephyrproject.org/latest/security/tfm.html)
- [LSM6DSO Datasheet](https://www.st.com/resource/en/datasheet/lsm6dso.pdf)

---

**Project Status:** In Development
**Estimated Completion:** 4 weeks
**Difficulty:** Advanced (requires ML knowledge)
**Fun Factor:** ⭐⭐⭐⭐⭐ (Very High!)

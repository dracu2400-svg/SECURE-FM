# Project 2: NRF52840 Activity Tracker with TinyML

**Hardware:** NRF52840-DK + SimCom A7672SA + IMU Sensor

**Features:**
- Activity recognition (walking, running, cycling, etc.)
- User identification via motion patterns
- BLE connectivity for smartphone app
- 4G fallback for cloud sync
- Secure ML model storage
- OTA model updates

## Quick Start

See `docs/BUILD_GUIDE.md` for complete build instructions.
See `docs/ML_PIPELINE.md` for ML model training.

## Documentation

- `docs/ARCHITECTURE.md` - System architecture
- `docs/BUILD_GUIDE.md` - Build instructions
- `docs/ML_PIPELINE.md` - ML training and deployment
- `docs/TESTING_GUIDE.md` - Testing procedures

## Source Code

- `src/secure/` - Secure firmware (TF-M)
- `src/non_secure/` - Application code
- `src/ml_models/` - TensorFlow Lite models
- `src/drivers/` - Hardware drivers
- `src/services/` - Application services

## ML Training

- `ml_training/train_activity_model.py` - Activity classifier
- `ml_training/train_user_model.py` - User identifier
- `ml_training/convert_to_tflite.py` - Model quantization

---

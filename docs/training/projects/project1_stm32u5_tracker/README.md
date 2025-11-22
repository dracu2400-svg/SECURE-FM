# Project 1: STM32U5 Secure GPS Tracker

**Hardware:** STM32U585 Nucleo-64 + SimCom A7672SA + X-Nucleo-IQS4A1

**Features:**
- Secure boot with MCUboot
- GPS tracking and location reporting
- 4G LTE connectivity
- Motion detection and geofencing
- Secure cloud communication (TLS 1.3)
- OTA firmware updates
- Device attestation

## Quick Start

See `docs/BUILD_GUIDE.md` for complete build instructions.

## Documentation

- `docs/ARCHITECTURE.md` - System architecture
- `docs/BUILD_GUIDE.md` - Build and flash instructions
- `docs/HARDWARE_SETUP.md` - Hardware connections
- `docs/TESTING_GUIDE.md` - Testing procedures
- `docs/DEPLOYMENT.md` - Production deployment

## Source Code

- `src/secure/` - Secure firmware (TF-M)
- `src/non_secure/` - Application code
- `src/drivers/` - Hardware drivers
- `src/services/` - Application services

---

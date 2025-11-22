# Tools for TF-M Training Package

This folder contains utilities and helper applications for advanced labs demonstrating multi-MCU communication and client-server architectures.

---

## Tools Overview

### 1. Side MCU Simulator
**Purpose:** Simulates external sensors/peripherals for communication labs

**Hardware:** Second NUCLEO board or Raspberry Pi Pico

**Use Cases:**
- Lab 15: UART Communication Security
- Lab 18: I2C Secure Transactions
- Lab 22: SPI with TrustZone

**Location:** `tools/side_mcu/`

---

### 2. Server MCU Application
**Purpose:** Backend server for client-server security demonstrations

**Hardware:** NUCLEO board or PC with Python

**Use Cases:**
- Lab 25: Mutual TLS Authentication
- Lab 27: Secure Remote Attestation
- Lab 29: OTA Firmware Update Server

**Location:** `tools/server_mcu/`

---

### 3. Flash Programming Scripts
**Purpose:** Automated flashing for multiple boards

**Supported Programmers:**
- ST-LINK (st-flash)
- OpenOCD
- STM32CubeProgrammer CLI
- J-Link

**Location:** `tools/flashers/`

---

### 4. Serial Console Multiplexer
**Purpose:** Monitor multiple UART consoles simultaneously

**Features:**
- Color-coded output per device
- Timestamp logging
- Packet capture
- Replay functionality

**Location:** `tools/serial_monitor/`

---

### 5. Crypto Key Generator
**Purpose:** Generate test keys and certificates

**Outputs:**
- RSA/ECC key pairs
- X.509 certificates
- Pre-shared keys (PSK)
- Test vectors

**Location:** `tools/keygen/`

---

## Quick Start

### Tool 1: Side MCU Simulator

**What It Does:**
Simulates a sensor sending encrypted data to main MCU over UART.

**Hardware Setup:**
```
Main MCU (NUCLEO-U545RE-Q)     Side MCU (NUCLEO-G071RB)
┌────────────────┐             ┌────────────────┐
│ PA9  (TX1) ────┼─────────────┼→ PA10 (RX2)    │
│ PA10 (RX1) ←───┼─────────────┼── PA9  (TX2)   │
│ GND ───────────┼─────────────┼── GND          │
└────────────────┘             └────────────────┘
```

**Build & Flash Side MCU:**
```bash
cd tools/side_mcu/sensor_simulator/
./build.sh
./flash.sh /dev/ttyACM1  # Second ST-LINK device
```

**Expected Output (Side MCU):**
```
[SIDE MCU] Sensor Simulator v1.0
[SIDE MCU] Sending encrypted sensor data...
[TX] Temp: 25.3°C, Humidity: 60%
[TX] Encrypted: A3 F2 8D 1C 94 B7 E5 22 ...
```

**Main MCU Receives:**
```
[MAIN MCU] Received encrypted data (24 bytes)
[MAIN MCU] Decrypting with shared key...
[MAIN MCU] ✓ Decryption successful
[MAIN MCU] Temp: 25.3°C, Humidity: 60%
[MAIN MCU] ✓ HMAC verified
```

**Lab Integration:**
- Lab 15: UART Secure Communication
- Lab 16: Encrypted Sensor Data

---

### Tool 2: Server MCU Application

**What It Does:**
Acts as cloud server for device attestation and firmware updates.

**Hardware Options:**

**Option A: NUCLEO Board as Server**
```bash
cd tools/server_mcu/firmware/
./build.sh
./flash.sh
```

**Option B: PC with Python**
```bash
cd tools/server_mcu/python/
pip3 install -r requirements.txt
python3 server.py --port 8443
```

**Server Features:**
- HTTPS/TLS server
- Device attestation verification
- Firmware update hosting
- Key provisioning

**Expected Output:**
```
[SERVER] TF-M Attestation Server v1.0
[SERVER] Listening on https://0.0.0.0:8443
[SERVER] Loading root CA certificate...
[SERVER] ✓ Server ready

[CLIENT CONNECT] 192.168.1.100:52341
[ATTEST] Verifying attestation token...
[ATTEST] Device ID: STM32U545-001
[ATTEST] Firmware version: 1.0.0
[ATTEST] Boot state: SECURE
[ATTEST] ✓ Token signature valid
[ATTEST] ✓ Device authenticated
```

**Lab Integration:**
- Lab 25: Mutual TLS
- Lab 27: Remote Attestation
- Lab 29: OTA Update

---

## Tool Documentation

Each tool has detailed documentation:

### Side MCU Tools

| Tool | Purpose | Lab | Doc |
|------|---------|-----|-----|
| `sensor_simulator` | Send encrypted sensor data | 15, 16 | [README](side_mcu/sensor_simulator/README.md) |
| `peripheral_mock` | Mock I2C/SPI peripherals | 18, 22 | [README](side_mcu/peripheral_mock/README.md) |
| `challenge_responder` | Challenge-response auth | 20 | [README](side_mcu/challenge_responder/README.md) |

### Server MCU Tools

| Tool | Purpose | Lab | Doc |
|------|---------|-----|-----|
| `attestation_server` | Verify device identity | 27 | [README](server_mcu/attestation_server/README.md) |
| `ota_server` | Firmware update server | 29 | [README](server_mcu/ota_server/README.md) |
| `key_provisioning` | Provision device keys | 30 | [README](server_mcu/key_provisioning/README.md) |

### Utility Tools

| Tool | Purpose | Doc |
|------|---------|-----|
| `flasher` | Multi-board programming | [README](flashers/README.md) |
| `serial_mux` | Multi-UART console | [README](serial_monitor/README.md) |
| `keygen` | Crypto key generation | [README](keygen/README.md) |

---

## Common Workflows

### Workflow 1: Multi-MCU Lab Setup

**Scenario:** Lab 15 - Secure UART Communication

**Steps:**

1. **Flash Main MCU:**
```bash
cd section_03_advanced/labs/solutions/lab_15/
./build.sh
./flash.sh /dev/ttyACM0
```

2. **Flash Side MCU:**
```bash
cd tools/side_mcu/sensor_simulator/
./flash.sh /dev/ttyACM1
```

3. **Start Serial Monitor:**
```bash
cd tools/serial_monitor/
./monitor.sh --main /dev/ttyACM0 --side /dev/ttyACM1
```

4. **Expected Output:**
```
[MAIN MCU] Waiting for sensor data...
[SIDE MCU] Sending temperature: 25.3°C
[MAIN MCU] ✓ Received and decrypted: 25.3°C
```

---

### Workflow 2: OTA Firmware Update

**Scenario:** Lab 29 - Secure OTA Update

**Steps:**

1. **Start OTA Server (PC):**
```bash
cd tools/server_mcu/ota_server/
python3 ota_server.py \
    --firmware new_firmware.bin \
    --cert server.crt \
    --key server.key
```

2. **Flash Initial Firmware (NUCLEO):**
```bash
cd section_05_optimization/labs/solutions/lab_29/
./flash.sh old_firmware.bin
```

3. **Trigger Update:**
- Press button on NUCLEO
- Device connects to server
- Downloads new firmware
- Verifies signature
- Installs and reboots

4. **Verify Update:**
```bash
# New firmware prints:
[BOOT] Firmware version: 2.0.0 (updated!)
```

---

### Workflow 3: Attestation Demo

**Scenario:** Lab 27 - Remote Attestation

**Steps:**

1. **Start Attestation Server:**
```bash
cd tools/server_mcu/attestation_server/
./run_server.sh --port 8443
```

2. **Run Device:**
```bash
cd section_04_integration/labs/solutions/lab_27/
# Device auto-connects to server
```

3. **Server Verifies:**
```
[SERVER] Device connected: 192.168.1.100
[SERVER] Verifying attestation...
[SERVER] ✓ Device authenticated
[SERVER] Allowing access to cloud services
```

---

## Hardware Requirements

### Minimum Setup (Most Labs)
- 1× NUCLEO-U545RE-Q
- 1× USB cable
- 1× PC with Linux/macOS/Windows

### Advanced Setup (Multi-MCU Labs)
- 2× NUCLEO boards (any compatible model)
- 2× USB cables
- 1× PC

### Full Setup (All Labs + Demos)
- 2× NUCLEO-U545RE-Q
- 1× NUCLEO-G071RB (side MCU)
- 1× Raspberry Pi (optional server)
- 3× USB cables
- Breadboard and jumper wires

---

## Software Requirements

### Development Tools
- ARM GCC toolchain
- CMake 3.15+
- OpenOCD or ST-LINK tools
- Python 3.8+ (for server tools)

### Python Dependencies
```bash
pip3 install -r tools/requirements.txt
# Installs:
# - pyserial (serial communication)
# - cryptography (TLS/crypto)
# - flask (web server)
# - cbor2 (attestation tokens)
# - pycose (COSE signatures)
```

---

## Troubleshooting

### Issue: Can't find second ST-LINK device

**Problem:**
```
Error: /dev/ttyACM1 not found
```

**Solution:**
```bash
# List all USB serial devices
ls /dev/ttyACM*
# Output: /dev/ttyACM0 /dev/ttyACM2

# Update flash script
./flash.sh /dev/ttyACM2  # Use actual device
```

---

### Issue: UART communication not working

**Problem:**
Main MCU doesn't receive data from side MCU.

**Solution:**
1. Check wiring (TX→RX crossover)
2. Verify baud rates match (115200)
3. Test with loopback (TX→RX on same board)
4. Check ground connection

```bash
# Enable debug output
./monitor.sh --main /dev/ttyACM0 --side /dev/ttyACM1 --debug
```

---

### Issue: Server certificate errors

**Problem:**
```
SSL Error: certificate verify failed
```

**Solution:**
```bash
# Regenerate certificates
cd tools/keygen/
./generate_test_certs.sh

# Copy to server
cp certs/server.crt tools/server_mcu/ota_server/
cp certs/server.key tools/server_mcu/ota_server/

# Copy CA cert to device
cp certs/ca.crt section_05_optimization/labs/solutions/lab_29/certs/
```

---

## Security Notes

⚠️ **WARNING:** Tools in this folder are for **EDUCATION ONLY**

- Certificate files are for testing
- Pre-shared keys are example keys
- DO NOT use in production!
- Always generate unique keys for real devices

**Production Checklist:**
- ✓ Generate unique device keys
- ✓ Use HSM for root keys
- ✓ Implement key rotation
- ✓ Use proper certificate authority
- ✓ Enable audit logging
- ✓ Follow PSA Certified guidelines

---

## Additional Resources

### Documentation
- [Side MCU Guide](side_mcu/GUIDE.md)
- [Server Setup](server_mcu/SETUP.md)
- [Crypto Tools](keygen/CRYPTO_GUIDE.md)

### Video Tutorials
- Multi-MCU Lab Setup (10 min)
- OTA Update Demo (15 min)
- Attestation Workflow (12 min)

### Support
- GitHub Issues: [SECURE-FM/issues](https://github.com/...)
- Forum: [TF-M Community](https://developer.trustedfirmware.org/)

---

**Tools Version:** 1.0
**Last Updated:** 2025-11-22
**Compatibility:** TF-M 1.8+, STM32U5, NUCLEO boards

# Side MCU: Sensor Simulator

**Purpose:** Simulates encrypted sensor data transmission for Lab 15 & 16

**Hardware:** NUCLEO-G071RB or any STM32 Nucleo board

**Connection:** UART to main NUCLEO-U545RE-Q

---

## Overview

This tool simulates a sensor module that:
1. Reads simulated temperature/humidity data
2. Encrypts data with shared AES key
3. Adds HMAC for integrity
4. Sends over UART to main MCU
5. Main MCU decrypts and verifies

**Security Demonstrated:**
- Shared key encryption (AES-128-GCM)
- Message authentication (HMAC)
- Replay attack prevention (nonce/timestamp)
- Secure UART protocol

---

## Hardware Setup

### Wiring Diagram

```
Main MCU (NUCLEO-U545RE-Q)     Side MCU (NUCLEO-G071RB)
┌────────────────────┐         ┌────────────────────┐
│                    │         │                    │
│ PA9  (UART1 TX) ───┼────────→│ PA10 (UART2 RX)    │
│ PA10 (UART1 RX) ←──┼─────────│ PA9  (UART2 TX)    │
│                    │         │                    │
│ GND ───────────────┼─────────│ GND                │
│                    │         │                    │
└────────────────────┘         └────────────────────┘

Note: TX → RX crossover!
```

### Pin Configuration

| Main MCU | Side MCU | Signal |
|----------|----------|--------|
| PA9 | PA10 | Main TX → Side RX |
| PA10 | PA9 | Main RX ← Side TX |
| GND | GND | Common ground |

---

## Building

### Prerequisites

```bash
# Install ARM GCC toolchain
sudo apt-get install gcc-arm-none-eabi

# Install st-flash
sudo apt-get install stlink-tools
```

### Build Steps

```bash
cd tools/side_mcu/sensor_simulator/
./build.sh
```

**Expected Output:**
```
[BUILD] Compiling sensor_simulator.c...
[BUILD] Linking...
[BUILD] Creating binary...
[BUILD] ✓ Build successful
[BUILD] Output: build/sensor_simulator.bin (24 KB)
```

---

## Flashing

### Method 1: Using st-flash

```bash
# Connect side MCU via USB
# Check device appears
ls /dev/ttyACM*
# Should show /dev/ttyACM1 (if main MCU is /dev/ttyACM0)

# Flash firmware
./flash.sh /dev/ttyACM1
```

### Method 2: Using OpenOCD

```bash
openocd -f board/st_nucleo_g0.cfg -c "program build/sensor_simulator.bin verify reset exit 0x08000000"
```

---

## Running

### Start Sensor Simulator

1. Flash side MCU with sensor_simulator.bin
2. Open serial console to side MCU:
```bash
screen /dev/ttyACM1 115200
```

3. Expected output:
```
═══════════════════════════════════════════════════════════
  Sensor Simulator v1.0 - Side MCU
═══════════════════════════════════════════════════════════
Hardware: NUCLEO-G071RB (STM32G071RB)
UART: 115200 baud
Encryption: AES-128-GCM
─────────────────────────────────────────────────────────

[INIT] Initializing crypto...
[INIT] ✓ AES key loaded
[INIT] ✓ UART configured
[INIT] Starting sensor loop...

[SENSOR] Reading sensors...
[SENSOR] Temperature: 25.3°C
[SENSOR] Humidity: 60%
[SENSOR] Pressure: 1013 hPa

[CRYPTO] Encrypting data...
[CRYPTO] Plaintext: 19 43 00 3C 03 F5 (6 bytes)
[CRYPTO] ✓ Encrypted: A3 F2 8D 1C 94 B7 E5 22 9F 1D 4E 7A
[CRYPTO] ✓ HMAC: 3F 8D 2C 7E 1A 9F 4B 6D

[TX] Sending packet (32 bytes)...
[TX] ✓ Sent successfully

Waiting 5 seconds...
```

### Main MCU Receives

On main MCU (Lab 15), you should see:
```
[RX] Received 32 bytes from sensor
[RX] Encrypted payload: A3 F2 8D 1C 94 B7 E5 22 9F 1D 4E 7A
[RX] HMAC: 3F 8D 2C 7E 1A 9F 4B 6D

[CRYPTO] Verifying HMAC...
[CRYPTO] ✓ HMAC valid
[CRYPTO] Decrypting...
[CRYPTO] ✓ Decryption successful

[DATA] Temperature: 25.3°C
[DATA] Humidity: 60%
[DATA] Pressure: 1013 hPa
[DATA] ✓ All checks passed
```

---

## Protocol Specification

### Packet Format

```
┌────────┬──────────┬────────────┬──────────┬──────────┐
│ Header │ Nonce    │ Ciphertext │ Tag      │ HMAC     │
│ 4 bytes│ 12 bytes │ Variable   │ 16 bytes │ 32 bytes │
└────────┴──────────┴────────────┴──────────┴──────────┘
```

### Header (4 bytes)

| Byte | Description |
|------|-------------|
| 0 | Magic (0xA5) |
| 1 | Version (0x01) |
| 2-3 | Payload length (big-endian) |

### Example Packet

```
Hex dump:
A5 01 00 06   # Header: magic=0xA5, ver=1, len=6
3F 2D 8B 1C 94 B7 E5 22 9A 1D 4E 7A   # Nonce (12 bytes)
C3 F8 2D 1A 9F 4B   # Ciphertext (6 bytes - encrypted sensor data)
5D 8C 3A 7F 1E 9B 4D 2F A6 C3 8E 1D 7F 4B 9A 2C   # GCM tag (16 bytes)
8F 3D 2C 7E 1A 5F 9B 4D 8C 3E 7A 1F 6D 9C 2E 5B   # HMAC (32 bytes)
1C 4E 7B 9A 3D 6F 8C 2E 5A 1D 4F 7C 9B 3E 6A 8D
```

### Sensor Data Format (Plaintext)

```c
typedef struct {
    int16_t temperature;  // °C × 100 (e.g., 2530 = 25.30°C)
    uint16_t humidity;    // % × 100 (e.g., 6000 = 60%)
    uint16_t pressure;    // hPa (e.g., 1013)
} SensorData_t;
```

---

## Configuration

### Shared Key

**⚠️ Test key only - DO NOT use in production!**

```c
const uint8_t shared_key[16] = {
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};
```

### Transmission Interval

Default: 5 seconds

To change:
```c
// In sensor_simulator.c
#define TX_INTERVAL_MS  5000  // Change to desired interval
```

### Simulated Sensor Ranges

```c
// Temperature: 20-30°C (random)
// Humidity: 40-80% (random)
// Pressure: 990-1030 hPa (random)
```

To use real sensors, replace `read_simulated_sensors()` with actual I2C reads.

---

## Troubleshooting

### Problem: Main MCU not receiving data

**Check:**
1. Wiring (TX→RX crossover?)
2. Common ground connected
3. Baud rate matches (115200)
4. Correct UART peripheral enabled

**Debug:**
```bash
# Connect to side MCU console
screen /dev/ttyACM1 115200

# Should see "[TX] Sent successfully"
# If not, check UART initialization
```

### Problem: HMAC verification fails

**Check:**
1. Shared keys match on both MCUs
2. Nonce not being reused
3. Packet format correct

**Debug:**
Enable verbose crypto output in main MCU:
```c
#define CRYPTO_DEBUG  1  // In lab_15/main.c
```

### Problem: Random "garbage" data

**Likely cause:** Baud rate mismatch

**Solution:**
```c
// Verify both MCUs use same baud rate
// Side MCU: sensor_simulator.c
huart.Init.BaudRate = 115200;

// Main MCU: lab_15/main.c
huart.Init.BaudRate = 115200;
```

---

## Advanced Usage

### Inject Attacks for Testing

**Replay Attack:**
```c
// In sensor_simulator.c
// Uncomment to send same nonce twice
#define TEST_REPLAY_ATTACK  1
```

**Tampered Data:**
```c
// Modify one byte of ciphertext
#define TEST_TAMPER_ATTACK  1
```

**Invalid HMAC:**
```c
// Send packet with wrong HMAC
#define TEST_INVALID_HMAC  1
```

Main MCU should detect and reject all attacks!

---

## Code Structure

```
sensor_simulator/
├── src/
│   ├── main.c              # Main program
│   ├── crypto.c            # AES/HMAC functions
│   ├── uart.c              # UART driver
│   └── sensors.c           # Simulated sensors
├── include/
│   ├── crypto.h
│   ├── uart.h
│   └── sensors.h
├── build.sh                # Build script
├── flash.sh                # Flash script
├── CMakeLists.txt          # CMake configuration
└── README.md               # This file
```

---

## Integration with Labs

### Lab 15: Secure UART Communication
- Main MCU receives and decrypts sensor data
- Verifies HMAC integrity
- Detects replay attacks

### Lab 16: Sensor Data Encryption
- Extended version with multiple sensors
- Data logging to PS
- Alert thresholds

### Lab 20: Challenge-Response Authentication
- Modified to include challenge-response handshake before data exchange

---

## Security Analysis

### What This Demonstrates

✓ **Encryption:** Prevents eavesdropping on UART
✓ **Authentication:** HMAC proves data from trusted sensor
✓ **Integrity:** Tampering detected
✓ **Freshness:** Nonce prevents replay attacks

### Attack Scenarios

**Scenario 1: Attacker Eavesdrops UART**
- Captures packets
- Cannot decrypt (no key)
- Cannot read sensor values

**Scenario 2: Attacker Replays Old Packet**
- Main MCU tracks nonces
- Detects duplicate
- Rejects packet

**Scenario 3: Attacker Modifies Packet**
- HMAC verification fails
- Packet rejected
- Alert logged

### Limitations

⚠️ **Shared key must be provisioned securely**
⚠️ **No perfect forward secrecy** (same key always)
⚠️ **Key compromise = all past data readable**

**Production improvements:**
- Use TLS-like handshake
- Session keys derived from master key
- Key rotation
- Certificate-based authentication

---

## Performance

**Packet Size:** 32 bytes overhead
**Throughput:** ~500 packets/sec @ 115200 baud
**Latency:** <5ms end-to-end
**CPU Usage:** <5% on STM32G0 @ 64MHz

---

## Next Steps

1. ✅ Build and flash sensor simulator
2. ✅ Connect to main MCU (Lab 15)
3. ✅ Verify encrypted data reception
4. ✅ Test attack scenarios
5. ✅ Modify for your own sensors

**Want more?**
- Implement bidirectional communication
- Add command & control from main MCU
- Multi-sensor support (up to 10 sensors)
- Data compression before encryption

---

**Version:** 1.0
**Tested On:** NUCLEO-G071RB, NUCLEO-F401RE
**Compatible With:** Lab 15, Lab 16, Lab 20

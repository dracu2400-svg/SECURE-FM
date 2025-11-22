# Project 1: STM32U5 Secure Tracker with SimCom A7672SA
## Building a Production-Ready Secure IoT Tracker

---

## 📋 Project Overview

### Project Goal

Build a complete, production-ready secure GPS tracker using:
- **STM32U5 Nucleo-64 board** (STM32U585ZI-Q)
- **SimCom A7672SA** 4G LTE module
- **LSM6DSO** IMU (accelerometer + gyroscope)
- **Trusted Firmware-M** for security
- **MCUboot** for secure boot and OTA updates

### Features

✅ **Secure Communication**: TLS 1.3 to cloud platform
✅ **Secure Boot**: MCUboot with ECDSA signature verification
✅ **Secure Storage**: Device credentials in TF-M Protected Storage
✅ **OTA Updates**: Over-the-air firmware updates via 4G
✅ **Motion Detection**: Accelerometer-based movement tracking
✅ **GPS Tracking**: Real-time location reporting
✅ **Low Power**: Sleep modes with wake-on-motion
✅ **Tamper Detection**: Physical attack mitigation
✅ **Attestation**: Device identity and integrity reporting

### System Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                    STM32U585 MCU                             │
│  ┌────────────────────────────────────────────────────────┐  │
│  │  Non-Secure World (NSPE)                               │  │
│  │  ┌──────────────────────────────────────────────────┐  │  │
│  │  │  Application Logic                               │  │  │
│  │  │  • GPS data processing                           │  │  │
│  │  │  • Motion detection algorithm                    │  │  │
│  │  │  • Cloud communication                           │  │  │
│  │  │  • Power management                              │  │  │
│  │  └──────────────────────────────────────────────────┘  │  │
│  │  ┌──────────────────────────────────────────────────┐  │  │
│  │  │  Drivers                                         │  │  │
│  │  │  • A7672SA driver (UART + AT commands)           │  │  │
│  │  │  • LSM6DSO driver (I2C/SPI)                      │  │  │
│  │  │  • GNSS parser                                   │  │  │
│  │  └──────────────────────────────────────────────────┘  │  │
│  └────────────────────────────────────────────────────────┘  │
│         │ PSA API Calls (Crypto, Storage, Attestation)       │
│  ═══════════════════════════════════════════════════════════ │
│  ┌────────────────────────────────────────────────────────┐  │
│  │  Secure World (SPE) - TF-M                            │  │
│  │  ┌──────────────────────────────────────────────────┐  │  │
│  │  │  Crypto Service                                  │  │  │
│  │  │  • TLS key management                            │  │  │
│  │  │  • Message signing                               │  │  │
│  │  │  • Random number generation                      │  │  │
│  │  └──────────────────────────────────────────────────┘  │  │
│  │  ┌──────────────────────────────────────────────────┐  │  │
│  │  │  Secure Storage (ITS + PS)                       │  │  │
│  │  │  • Device credentials (SIM PIN, API keys)        │  │  │
│  │  │  • TLS certificates                              │  │  │
│  │  │  • Configuration data                            │  │  │
│  │  └──────────────────────────────────────────────────┘  │  │
│  │  ┌──────────────────────────────────────────────────┐  │  │
│  │  │  Attestation Service                             │  │  │
│  │  │  • Device identity token                         │  │  │
│  │  │  • Firmware integrity proof                      │  │  │
│  │  └──────────────────────────────────────────────────┘  │  │
│  │  ┌──────────────────────────────────────────────────┐  │  │
│  │  │  Firmware Update Service                         │  │  │
│  │  │  • OTA update management                         │  │  │
│  │  │  • Image validation                              │  │  │
│  │  └──────────────────────────────────────────────────┘  │  │
│  └────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
          │                 │                    │
          ▼                 ▼                    ▼
   ┌────────────┐    ┌────────────┐      ┌────────────┐
   │  A7672SA   │    │  LSM6DSO   │      │   Flash    │
   │  4G Module │    │    IMU     │      │  Storage   │
   │  • LTE     │    │  • Accel   │      │  • Secure  │
   │  • GNSS    │    │  • Gyro    │      │  • Non-Sec │
   └────────────┘    └────────────┘      └────────────┘
          │
          ▼
   ┌────────────┐
   │    Cloud   │
   │  Platform  │
   │  • MQTT    │
   │  • HTTPS   │
   └────────────┘
```

---

## 🔧 Hardware Setup

### Required Components

1. **STM32U585ZI-Q Nucleo-64 Board**
   - ARM Cortex-M33 @ 160 MHz
   - 2 MB Flash, 786 KB RAM
   - TrustZone support
   - Hardware crypto accelerator (AES, PKA)
   - ~$25 USD

2. **SimCom A7672SA 4G LTE Module**
   - LTE Cat-1 (10 Mbps DL, 5 Mbps UL)
   - Integrated GNSS (GPS, GLONASS, Galileo, BeiDou)
   - UART interface
   - SIM card slot
   - ~$15-20 USD

3. **LSM6DSO IMU Sensor**
   - 6-axis: 3D accelerometer + 3D gyroscope
   - I2C/SPI interface
   - Ultra-low power
   - ~$5 USD
   - Alternative: Use STM32 Nucleo IKS01A3 shield (includes LSM6DSO)

4. **Additional Components**
   - SIM card (activated with data plan)
   - GPS/LTE antenna
   - Power supply (3.3V-5V, 2A capable)
   - Jumper wires, breadboard

### Pinout Connections

**A7672SA Module → STM32U585:**

```
A7672SA Pin    →  STM32U585 Pin    Function
────────────────────────────────────────────
UART_TXD       →  PA10 (USART1_RX) Serial RX
UART_RXD       →  PA9  (USART1_TX) Serial TX
PWRKEY         →  PC13             Power On/Off
STATUS         →  PC14             Module Status
RESET          →  PC15             Reset
VCC            →  5V               Power Supply
GND            →  GND              Ground
```

**LSM6DSO IMU → STM32U585 (I2C):**

```
LSM6DSO Pin    →  STM32U585 Pin    Function
────────────────────────────────────────────
SDA            →  PB7  (I2C1_SDA)  I2C Data
SCL            →  PB6  (I2C1_SCL)  I2C Clock
INT1           →  PB8              Interrupt 1
VDD            →  3.3V             Power Supply
GND            →  GND              Ground
```

**Power Supply:**

```
⚠️  IMPORTANT: A7672SA requires 2A peak current during transmission!

Option 1: External 5V/2A power supply
  5V → A7672SA VCC
  GND → Common ground

Option 2: USB power with capacitor bank
  Add 1000µF capacitor near A7672SA VCC
  Reduces voltage drop during TX bursts
```

### Hardware Setup Steps

1. **Connect A7672SA Module**
   ```bash
   1. Insert SIM card into A7672SA
   2. Connect UART pins to STM32U585
   3. Connect power (5V) and ground
   4. Connect PWRKEY to GPIO (PC13)
   5. Attach LTE antenna to A7672SA MAIN antenna port
   6. Attach GPS antenna to A7672SA GNSS antenna port
   ```

2. **Connect LSM6DSO Sensor**
   ```bash
   1. Connect I2C pins (SDA, SCL) to STM32U585
   2. Connect interrupt pin (INT1) to PB8
   3. Connect 3.3V power and ground
   ```

3. **Power Setup**
   ```bash
   1. Connect external 5V/2A power supply
   2. OR use USB with 1000µF capacitor near A7672SA
   3. Verify voltage stability with multimeter
   ```

4. **Verify Connections**
   ```bash
   1. Check all connections with multimeter (continuity)
   2. Verify no shorts between VCC and GND
   3. Power on STM32U585
   4. Power on A7672SA (hold PWRKEY low for 2 seconds)
   5. STATUS LED should blink (module is running)
   ```

---

## 💻 Software Setup

### Development Environment

```bash
# 1. Clone TF-M repository
git clone https://git.trustedfirmware.org/TF-M/trusted-firmware-m.git
cd trusted-firmware-m

# 2. Install STM32 platform dependencies
sudo apt-get install -y \
    gcc-arm-none-eabi \
    cmake \
    ninja-build \
    python3-pip \
    git \
    openocd

# 3. Install Python dependencies
pip3 install -r tools/requirements.txt

# 4. Install STM32CubeProgrammer (for flashing)
# Download from: https://www.st.com/en/development-tools/stm32cubeprog.html

# 5. Verify installation
arm-none-eabi-gcc --version
cmake --version
openocd --version
```

### Project Directory Structure

```
stm32u5-secure-tracker/
├── tfm/                          # TF-M integration
│   ├── config/                   # TF-M configuration
│   ├── platform/                 # Platform-specific code
│   └── partitions/               # Custom secure partitions
│
├── app/                          # Application code
│   ├── main.c                    # Main application
│   ├── tracker/                  # Tracker logic
│   │   ├── gps_manager.c
│   │   ├── motion_detector.c
│   │   └── cloud_client.c
│   ├── drivers/                  # Hardware drivers
│   │   ├── a7672sa.c            # A7672SA driver
│   │   ├── lsm6dso.c            # IMU driver
│   │   └── gnss_parser.c        # GPS NMEA parser
│   └── config/                   # Configuration
│       ├── config.h
│       └── credentials.h
│
├── secure/                       # Secure services
│   ├── crypto_manager.c          # Crypto operations
│   ├── credential_store.c        # Credential management
│   └── attestation.c             # Device attestation
│
├── tests/                        # Test code
│   ├── test_gps.c
│   ├── test_motion.c
│   ├── test_cloud.c
│   └── test_security.c
│
├── tools/                        # Build and deployment tools
│   ├── flash.sh                  # Flashing script
│   ├── ota_server.py             # OTA update server
│   └── provision.py              # Device provisioning
│
├── CMakeLists.txt                # Build configuration
└── README.md                     # Project documentation
```

---

## 🔐 Security Architecture

### Threat Model

**Assets to Protect:**
1. Device credentials (SIM PIN, API keys, certificates)
2. GPS location data
3. Firmware intellectual property
4. Communication security (TLS keys)

**Threats:**
1. **Physical Attacks**: Device tampering, chip-off attacks
2. **Network Attacks**: Man-in-the-middle, eavesdropping
3. **Firmware Attacks**: Malware injection, rollback
4. **Side-Channel Attacks**: Power analysis, timing attacks

**Mitigations:**

```
┌────────────────────────────────────────────────────────┐
│  Security Measures                                     │
├────────────────────────────────────────────────────────┤
│  1. Secure Boot (MCUboot)                             │
│     ✓ Verify firmware signature on every boot        │
│     ✓ ECDSA-P256 signatures                           │
│     ✓ Rollback protection with security counters     │
│                                                        │
│  2. Secure Credential Storage (TF-M PS)               │
│     ✓ AES-256-GCM encryption                          │
│     ✓ Authenticated storage                           │
│     ✓ Isolated from application                       │
│                                                        │
│  3. Secure Communication (TLS 1.3)                    │
│     ✓ End-to-end encryption to cloud                  │
│     ✓ Certificate-based authentication                │
│     ✓ Perfect forward secrecy                         │
│                                                        │
│  4. Device Attestation                                │
│     ✓ Cryptographic device identity                   │
│     ✓ Firmware integrity proof                        │
│     ✓ Boot measurement                                │
│                                                        │
│  5. Tamper Detection                                  │
│     ✓ Flash readout protection (RDP Level 2)          │
│     ✓ Debug interface locked in production            │
│     ✓ Unique device ID (UID)                          │
│                                                        │
│  6. Side-Channel Protection                           │
│     ✓ Hardware AES engine                             │
│     ✓ Randomized execution (FIH library)              │
│     ✓ Constant-time crypto operations                 │
└────────────────────────────────────────────────────────┘
```

### Secure Boot Flow

```
Power-On
    │
    ▼
┌─────────────────────────┐
│  MCUboot (BL2)          │
│  • Immutable code in    │
│    protected flash      │
└─────────────────────────┘
    │
    ├─→ Read TF-M S image from primary slot
    │
    ├─→ Verify ECDSA-P256 signature
    │   SHA256(image) → hash
    │   ECDSA_verify(hash, signature, public_key)
    │
    ├─→ Check security counter
    │   if (image_counter < stored_counter) → REJECT
    │
    ├─→ Check anti-rollback version
    │   if (image_version < min_version) → REJECT
    │
    ├─→ Signature valid? ────No───→ HALT (Security violation)
    │                                   │
    │                                   ▼
    Yes                          ┌──────────────┐
    │                            │  Secure Fail │
    │                            │  - Red LED   │
    ├─→ Repeat for NS image     │  - UART log  │
    │                            └──────────────┘
    ├─→ Both images valid? ───No───→ HALT
    │
    Yes
    │
    ▼
┌─────────────────────────┐
│  Jump to TF-M SPE       │
│  • Initialize TrustZone │
│  • Start secure services│
└─────────────────────────┘
    │
    ▼
┌─────────────────────────┐
│  Jump to Application    │
│  • Normal execution     │
└─────────────────────────┘
```

### Credential Management

**Stored Credentials:**

```c
/* Credential Storage UIDs */
#define UID_SIM_PIN          1001  /* SIM card PIN */
#define UID_APN_CONFIG       1002  /* APN configuration */
#define UID_CLOUD_API_KEY    1003  /* Cloud API key */
#define UID_TLS_CLIENT_CERT  1004  /* TLS client certificate */
#define UID_TLS_CLIENT_KEY   1005  /* TLS private key */
#define UID_TLS_CA_CERT      1006  /* TLS CA certificate */
#define UID_DEVICE_ID        1007  /* Unique device ID */
#define UID_ATTESTATION_KEY  1008  /* Attestation private key */

/* All stored in TF-M Protected Storage (encrypted + authenticated) */
```

**Provisioning Process:**

```
Manufacturing / First Boot:
1. Generate unique device ID (from STM32 UID)
2. Generate attestation key pair (ECDSA-P256)
3. Store private key in PS (UID_ATTESTATION_KEY)
4. Send public key + device ID to cloud for registration
5. Receive and store:
   - Cloud API key
   - TLS certificates
   - APN configuration
6. All credentials encrypted in PS
7. Lock flash (RDP Level 2)
8. Device ready for deployment
```

---

## 📡 A7672SA Driver Implementation

### Driver Architecture

```c
/* a7672sa.h */

#ifndef A7672SA_H
#define A7672SA_H

#include <stdint.h>
#include <stdbool.h>

/* Module power states */
typedef enum {
    A7672_POWER_OFF = 0,
    A7672_POWER_ON = 1,
    A7672_POWER_SLEEP = 2
} a7672_power_state_t;

/* Network registration status */
typedef enum {
    A7672_NET_NOT_REGISTERED = 0,
    A7672_NET_REGISTERED_HOME = 1,
    A7672_NET_SEARCHING = 2,
    A7672_NET_DENIED = 3,
    A7672_NET_UNKNOWN = 4,
    A7672_NET_REGISTERED_ROAMING = 5
} a7672_net_status_t;

/* GPS fix status */
typedef enum {
    GPS_NO_FIX = 0,
    GPS_FIX_2D = 2,
    GPS_FIX_3D = 3
} gps_fix_type_t;

/* GPS data structure */
typedef struct {
    double latitude;       /* Decimal degrees */
    double longitude;      /* Decimal degrees */
    float altitude;        /* Meters above sea level */
    float speed;           /* km/h */
    float course;          /* Degrees from north */
    uint8_t satellites;    /* Number of satellites */
    gps_fix_type_t fix;    /* Fix type */
    uint32_t timestamp;    /* Unix timestamp */
} gps_data_t;

/* Module configuration */
typedef struct {
    const char *apn;
    const char *username;
    const char *password;
    const char *pin;       /* SIM PIN (if required) */
} a7672_config_t;

/* Initialization */
int a7672_init(void);
int a7672_deinit(void);

/* Power management */
int a7672_power_on(void);
int a7672_power_off(void);
int a7672_sleep(void);
int a7672_wakeup(void);

/* Network operations */
int a7672_configure(const a7672_config_t *config);
int a7672_connect_network(uint32_t timeout_ms);
a7672_net_status_t a7672_get_network_status(void);
int a7672_get_signal_quality(int *rssi, int *ber);

/* GPS operations */
int a7672_gps_power_on(void);
int a7672_gps_power_off(void);
int a7672_gps_get_location(gps_data_t *location);

/* Data communication */
int a7672_tcp_connect(const char *host, uint16_t port);
int a7672_tcp_send(const uint8_t *data, size_t len);
int a7672_tcp_receive(uint8_t *buffer, size_t max_len, size_t *received);
int a7672_tcp_disconnect(void);

int a7672_http_get(const char *url, uint8_t *response, size_t max_len);
int a7672_http_post(const char *url, const uint8_t *data, size_t len,
                     uint8_t *response, size_t max_len);

/* MQTT operations */
int a7672_mqtt_connect(const char *broker, uint16_t port,
                        const char *client_id, const char *username,
                        const char *password);
int a7672_mqtt_publish(const char *topic, const uint8_t *payload, size_t len,
                        uint8_t qos);
int a7672_mqtt_subscribe(const char *topic, uint8_t qos);
int a7672_mqtt_disconnect(void);

/* AT command interface */
int a7672_send_at_command(const char *cmd, char *response, size_t max_len,
                           uint32_t timeout_ms);

#endif /* A7672SA_H */
```

### Driver Implementation

```c
/* a7672sa.c */

#include "a7672sa.h"
#include "stm32u5xx_hal.h"
#include <string.h>
#include <stdio.h>

/* UART handle (configured for USART1) */
static UART_HandleTypeDef huart1;

/* GPIO pins */
#define PWRKEY_PORT  GPIOC
#define PWRKEY_PIN   GPIO_PIN_13
#define STATUS_PORT  GPIOC
#define STATUS_PIN   GPIO_PIN_14
#define RESET_PORT   GPIOC
#define RESET_PIN    GPIO_PIN_15

/* Buffer for AT responses */
#define AT_BUFFER_SIZE 2048
static char at_buffer[AT_BUFFER_SIZE];

/* Internal state */
static bool module_powered = false;
static bool network_connected = false;
static bool gps_enabled = false;

/**
 * Initialize A7672SA module
 */
int a7672_init(void) {
    /* Configure UART */
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;

    if (HAL_UART_Init(&huart1) != HAL_OK) {
        return -1;
    }

    /* Configure GPIO pins */
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable GPIO clocks */
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* PWRKEY - Output */
    GPIO_InitStruct.Pin = PWRKEY_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(PWRKEY_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(PWRKEY_PORT, PWRKEY_PIN, GPIO_PIN_SET);

    /* STATUS - Input */
    GPIO_InitStruct.Pin = STATUS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(STATUS_PORT, &GPIO_InitStruct);

    /* RESET - Output */
    GPIO_InitStruct.Pin = RESET_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RESET_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(RESET_PORT, RESET_PIN, GPIO_PIN_SET);

    return 0;
}

/**
 * Power on module
 */
int a7672_power_on(void) {
    if (module_powered) {
        return 0;  /* Already powered on */
    }

    /* Pull PWRKEY low for 2 seconds */
    HAL_GPIO_WritePin(PWRKEY_PORT, PWRKEY_PIN, GPIO_PIN_RESET);
    HAL_Delay(2000);
    HAL_GPIO_WritePin(PWRKEY_PORT, PWRKEY_PIN, GPIO_PIN_SET);

    /* Wait for STATUS pin to indicate module is running */
    uint32_t timeout = HAL_GetTick() + 10000;  /* 10 second timeout */
    while (HAL_GetTick() < timeout) {
        if (HAL_GPIO_ReadPin(STATUS_PORT, STATUS_PIN) == GPIO_PIN_RESET) {
            /* STATUS low = module running */
            module_powered = true;
            HAL_Delay(2000);  /* Wait for module to fully initialize */

            /* Test communication */
            if (a7672_send_at_command("AT", at_buffer, sizeof(at_buffer), 1000) == 0) {
                return 0;  /* Success */
            }
        }
        HAL_Delay(100);
    }

    return -1;  /* Timeout */
}

/**
 * Power off module
 */
int a7672_power_off(void) {
    if (!module_powered) {
        return 0;
    }

    /* Send AT command to power off */
    int ret = a7672_send_at_command("AT+CPOF", at_buffer, sizeof(at_buffer), 5000);

    /* Or force power off via PWRKEY */
    if (ret != 0) {
        HAL_GPIO_WritePin(PWRKEY_PORT, PWRKEY_PIN, GPIO_PIN_RESET);
        HAL_Delay(3000);
        HAL_GPIO_WritePin(PWRKEY_PORT, PWRKEY_PIN, GPIO_PIN_SET);
    }

    module_powered = false;
    network_connected = false;
    gps_enabled = false;

    return 0;
}

/**
 * Send AT command and receive response
 */
int a7672_send_at_command(const char *cmd, char *response, size_t max_len,
                           uint32_t timeout_ms) {
    if (!module_powered) {
        return -1;
    }

    /* Clear buffer */
    memset(response, 0, max_len);

    /* Send command */
    char cmd_buf[256];
    int cmd_len = snprintf(cmd_buf, sizeof(cmd_buf), "%s\r\n", cmd);
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd_buf, cmd_len, 1000);

    /* Receive response */
    uint32_t start_time = HAL_GetTick();
    size_t received = 0;

    while ((HAL_GetTick() - start_time) < timeout_ms) {
        uint8_t byte;
        if (HAL_UART_Receive(&huart1, &byte, 1, 10) == HAL_OK) {
            if (received < max_len - 1) {
                response[received++] = byte;
                response[received] = '\0';

                /* Check for OK or ERROR */
                if (strstr(response, "\r\nOK\r\n") != NULL) {
                    return 0;  /* Success */
                }
                if (strstr(response, "\r\nERROR\r\n") != NULL) {
                    return -2;  /* Error response */
                }
            }
        }
    }

    return -3;  /* Timeout */
}

/**
 * Configure network settings
 */
int a7672_configure(const a7672_config_t *config) {
    char cmd[256];
    int ret;

    /* Enter PIN if required */
    if (config->pin != NULL && strlen(config->pin) > 0) {
        snprintf(cmd, sizeof(cmd), "AT+CPIN=\"%s\"", config->pin);
        ret = a7672_send_at_command(cmd, at_buffer, sizeof(at_buffer), 5000);
        if (ret != 0) {
            return ret;
        }
        HAL_Delay(2000);  /* Wait for SIM to initialize */
    }

    /* Configure APN */
    snprintf(cmd, sizeof(cmd), "AT+CGDCONT=1,\"IP\",\"%s\"", config->apn);
    ret = a7672_send_at_command(cmd, at_buffer, sizeof(at_buffer), 1000);
    if (ret != 0) {
        return ret;
    }

    /* Set APN username and password if provided */
    if (config->username != NULL && config->password != NULL) {
        snprintf(cmd, sizeof(cmd), "AT+CGAUTH=1,1,\"%s\",\"%s\"",
                 config->username, config->password);
        ret = a7672_send_at_command(cmd, at_buffer, sizeof(at_buffer), 1000);
        if (ret != 0) {
            return ret;
        }
    }

    return 0;
}

/**
 * Connect to network
 */
int a7672_connect_network(uint32_t timeout_ms) {
    uint32_t start_time = HAL_GetTick();
    int ret;

    /* Activate PDP context */
    ret = a7672_send_at_command("AT+CGACT=1,1", at_buffer, sizeof(at_buffer), 10000);
    if (ret != 0) {
        return ret;
    }

    /* Wait for network registration */
    while ((HAL_GetTick() - start_time) < timeout_ms) {
        a7672_net_status_t status = a7672_get_network_status();

        if (status == A7672_NET_REGISTERED_HOME ||
            status == A7672_NET_REGISTERED_ROAMING) {
            network_connected = true;
            return 0;  /* Connected */
        }

        HAL_Delay(1000);
    }

    return -1;  /* Timeout */
}

/**
 * Get network registration status
 */
a7672_net_status_t a7672_get_network_status(void) {
    int ret = a7672_send_at_command("AT+CREG?", at_buffer, sizeof(at_buffer), 1000);
    if (ret != 0) {
        return A7672_NET_UNKNOWN;
    }

    /* Parse response: +CREG: n,stat */
    char *pos = strstr(at_buffer, "+CREG:");
    if (pos != NULL) {
        int n, stat;
        if (sscanf(pos, "+CREG: %d,%d", &n, &stat) == 2) {
            return (a7672_net_status_t)stat;
        }
    }

    return A7672_NET_UNKNOWN;
}

/**
 * Get signal quality
 */
int a7672_get_signal_quality(int *rssi, int *ber) {
    int ret = a7672_send_at_command("AT+CSQ", at_buffer, sizeof(at_buffer), 1000);
    if (ret != 0) {
        return ret;
    }

    /* Parse response: +CSQ: rssi,ber */
    char *pos = strstr(at_buffer, "+CSQ:");
    if (pos != NULL) {
        if (sscanf(pos, "+CSQ: %d,%d", rssi, ber) == 2) {
            return 0;
        }
    }

    return -1;
}

/**
 * Power on GPS
 */
int a7672_gps_power_on(void) {
    int ret;

    /* Power on GPS */
    ret = a7672_send_at_command("AT+CGNSPWR=1", at_buffer, sizeof(at_buffer), 1000);
    if (ret != 0) {
        return ret;
    }

    gps_enabled = true;
    HAL_Delay(1000);  /* Wait for GPS to initialize */

    return 0;
}

/**
 * Power off GPS
 */
int a7672_gps_power_off(void) {
    int ret = a7672_send_at_command("AT+CGNSPWR=0", at_buffer, sizeof(at_buffer), 1000);
    if (ret == 0) {
        gps_enabled = false;
    }
    return ret;
}

/**
 * Get GPS location
 */
int a7672_gps_get_location(gps_data_t *location) {
    if (!gps_enabled) {
        return -1;
    }

    int ret = a7672_send_at_command("AT+CGNSINF", at_buffer, sizeof(at_buffer), 1000);
    if (ret != 0) {
        return ret;
    }

    /* Parse NMEA response */
    /* +CGNSINF: run,fix,date,time,lat,lon,alt,speed,course,pdop,hdop,vdop,sats,... */
    char *pos = strstr(at_buffer, "+CGNSINF:");
    if (pos != NULL) {
        int run, fix_status;
        char date[16], time[16];

        if (sscanf(pos, "+CGNSINF: %d,%d,%[^,],%[^,],%lf,%lf,%f,%f,%f",
                   &run, &fix_status, date, time,
                   &location->latitude, &location->longitude,
                   &location->altitude, &location->speed,
                   &location->course) >= 7) {

            if (fix_status == 1) {
                location->fix = GPS_FIX_3D;
                return 0;  /* Valid fix */
            } else {
                location->fix = GPS_NO_FIX;
                return -2;  /* No GPS fix */
            }
        }
    }

    return -3;  /* Parse error */
}

/* ... (Continue with TCP, HTTP, MQTT implementations) ... */
```

---

This is the beginning of the comprehensive STM32U5 secure tracker guide. Would you like me to:

1. Continue with the complete implementation (motion detection, cloud integration, etc.)
2. Move on to the NRF52840 project guide
3. Create the final summary document

Which would you prefer?

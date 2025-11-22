# Section 6: Security & Attack Resistance

## Module 04: Physical Attack Mitigation

**Learning Objectives:**
- Understand physical security threats to embedded devices
- Learn tamper detection and response mechanisms
- Configure debug port security and lockdown
- Implement flash readout protection
- Design secure key storage and provisioning
- Apply physical security best practices

---

## 📋 Table of Contents

1. [Introduction to Physical Security](#1-introduction)
2. [Debug Port Security](#2-debug-port-security)
3. [Flash Readout Protection](#3-flash-readout-protection)
4. [Tamper Detection](#4-tamper-detection)
5. [Secure Key Storage](#5-secure-key-storage)
6. [Secure Enclosures](#6-secure-enclosures)
7. [Supply Chain Security](#7-supply-chain-security)
8. [Implementation on STM32U5](#8-implementation-on-stm32u5)

---

## 1. Introduction to Physical Security

### 1.1 Physical Threat Model

**Assumption:** Attacker has physical access to the device

**Attack Scenarios:**
1. **Laboratory Analysis**
   - Reverse engineering
   - Decapsulation and die inspection
   - Probing internal signals

2. **Field Deployment**
   - Device theft
   - Temporary access (minutes to hours)
   - Installation of malicious hardware

3. **Supply Chain**
   - Interception during shipping
   - Malicious insider at manufacturing
   - Counterfeit components

### 1.2 Physical Attack Hierarchy

```
Increasing Difficulty & Cost →

Level 1: External Access
├─ Debug port (JTAG/SWD)
├─ Serial console
└─ USB/Network interfaces

Level 2: PCB Access
├─ Desolder flash chips
├─ Probe bus signals
└─ Power/clock manipulation

Level 3: Package Removal
├─ Decapsulation (acid/milling)
├─ Die imaging
└─ Microprobing

Level 4: Die Modification
├─ Focused Ion Beam (FIB)
├─ Laser ablation
└─ Circuit editing

Level 5: Advanced Analysis
├─ Scanning Electron Microscope (SEM)
├─ Transmission Electron Microscope (TEM)
└─ X-ray tomography
```

### 1.3 Defense Strategy

**Layered Defense:**
```
┌───────────────────────────────────────────┐
│ Layer 5: Active Tamper Response          │ ← Erase keys on detection
├───────────────────────────────────────────┤
│ Layer 4: Passive Tamper Detection        │ ← Detect case opening
├───────────────────────────────────────────┤
│ Layer 3: Chip-Level Protection           │ ← RDP, debug lockdown
├───────────────────────────────────────────┤
│ Layer 2: Cryptographic Protection        │ ← Encrypted storage
├───────────────────────────────────────────┤
│ Layer 1: Access Control                  │ ← Authentication
└───────────────────────────────────────────┘
```

**Key Principle:** Make attack cost exceed asset value

---

## 2. Debug Port Security

### 2.1 Debug Interfaces on ARM Cortex-M

**Standard Interfaces:**
- **SWD (Serial Wire Debug):** 2-wire interface (SWDIO, SWCLK)
- **JTAG:** 4-wire interface (TDI, TDO, TMS, TCK)

**Capabilities with Debug Access:**
```
Debug Port Unlocked
    ↓
┌─────────────────────────────────────┐
│ Attacker Can:                       │
│ ✓ Halt CPU execution                │
│ ✓ Read all memory (Flash, RAM)      │
│ ✓ Write to memory                   │
│ ✓ Modify registers                  │
│ ✓ Single-step code execution        │
│ ✓ Set breakpoints                   │
│ ✓ Extract encryption keys            │
│ ✓ Bypass security checks            │
│ ✓ Install backdoors                 │
└─────────────────────────────────────┘
```

### 2.2 Debug Port Lockdown Strategies

#### Strategy 1: Permanent Disable (Production)

**STM32 Example:**
```c
/**
 * Permanently disable debug ports (IRREVERSIBLE!)
 *
 * WARNING: This is a one-way operation. Once set, the chip
 * cannot be debugged or reprogrammed via SWD/JTAG.
 */
void permanently_lock_debug(void)
{
    FLASH_OBProgramInitTypeDef ob_config;

    // Unlock option bytes
    HAL_FLASH_OB_Unlock();

    // Read current option bytes
    HAL_FLASHEx_OBGetConfig(&ob_config);

    // Set RDP (Read Protection) Level 2
    // Level 2 = Permanent protection, no debug, no downgrade
    ob_config.RDPLevel = OB_RDP_LEVEL_2;

    // Write option bytes
    HAL_FLASHEx_OBProgram(&ob_config);

    // Launch option byte loading (triggers system reset)
    HAL_FLASH_OB_Launch();

    // Lock option bytes
    HAL_FLASH_OB_Lock();

    // Device will reset and debug will be permanently disabled
}
```

**RDP Levels on STM32:**

| Level | Protection | Debug | Flash R/W | Reversible |
|-------|------------|-------|-----------|------------|
| **0** | None | ✅ Full access | ✅ Full | N/A |
| **1** | Medium | ❌ Disabled | ⚠️ Limited | ✅ Yes (erases flash) |
| **2** | Maximum | ❌ Permanently disabled | ❌ Protected | ❌ **NEVER** |

**⚠️ WARNING:** RDP Level 2 is IRREVERSIBLE! Use only for production devices.

#### Strategy 2: Conditional Access (Development)

**Secure Debug with Authentication:**
```c
/**
 * Enable secure debug with certificate-based authentication
 * (ARM CoreSight SDC-600 Secure Debug Channel)
 */

#define DEBUG_CERT_SIZE 256

typedef struct {
    uint8_t public_key[64];        // ECDSA P-256 public key
    uint8_t signature[64];         // Signature over challenge
    uint32_t permissions;          // Debug permissions
    uint32_t timestamp;            // Certificate validity
} debug_certificate_t;

bool authenticate_debug_access(debug_certificate_t *cert)
{
    // Generate random challenge
    uint8_t challenge[32];
    psa_generate_random(challenge, sizeof(challenge));

    // Verify certificate signature
    psa_key_id_t debug_root_key;  // Stored in OTP/fuses
    psa_status_t status;

    status = psa_verify_hash(
        debug_root_key,
        PSA_ALG_ECDSA(PSA_ALG_SHA_256),
        challenge,
        sizeof(challenge),
        cert->signature,
        sizeof(cert->signature)
    );

    if (status != PSA_SUCCESS) {
        return false;  // Invalid certificate
    }

    // Check timestamp (prevent replay attacks)
    if (cert->timestamp < get_current_time() - DEBUG_CERT_VALIDITY) {
        return false;  // Expired certificate
    }

    // Grant debug access
    enable_debug_port(cert->permissions);

    return true;
}
```

#### Strategy 3: Password Protection

```c
/**
 * Simple password-based debug unlock
 * (Less secure than certificate-based, but simpler)
 */

#define DEBUG_PASSWORD_LEN 16

static const uint8_t debug_password_hash[32] = {
    // SHA-256 hash of debug password (stored in protected flash)
    0xAB, 0xCD, 0xEF, /* ... */
};

bool unlock_debug_with_password(const uint8_t *password, size_t len)
{
    if (len != DEBUG_PASSWORD_LEN) {
        return false;
    }

    // Hash entered password
    uint8_t hash[32];
    psa_hash_compute(
        PSA_ALG_SHA_256,
        password,
        len,
        hash,
        sizeof(hash),
        NULL
    );

    // Constant-time compare
    volatile uint8_t diff = 0;
    for (int i = 0; i < 32; i++) {
        diff |= (hash[i] ^ debug_password_hash[i]);
    }

    if (diff == 0) {
        // Unlock debug for limited time (e.g., 1 hour)
        enable_debug_port_timed(3600);
        return true;
    }

    return false;
}
```

### 2.3 Debug Port Best Practices

**Production Devices:**
1. ✅ Set RDP Level 2 (if truly never need debug access)
2. ✅ OR use secure debug with strong authentication
3. ✅ Disable JTAG if only SWD needed (smaller attack surface)
4. ✅ Remove debug connector from PCB
5. ✅ Cover debug pads with epoxy

**Development/Field Service:**
1. ✅ Use RDP Level 1 (can unlock if needed, but erases flash)
2. ✅ Implement debug authentication
3. ✅ Log all debug access attempts
4. ✅ Time-limited debug access
5. ✅ Require physical button press + password

---

## 3. Flash Readout Protection

### 3.1 Threat: Flash Memory Extraction

**Attack Vectors:**

1. **Via Debug Port:**
   ```
   Attacker → SWD/JTAG → Halt CPU → Dump Flash → Extract Firmware & Keys
   ```

2. **Via Bootloader:**
   ```
   Attacker → Trigger bootloader → Use read command → Dump Flash
   ```

3. **External Flash Chip:**
   ```
   Attacker → Desolder SPI Flash → Read with programmer → Extract Data
   ```

4. **Chip Decapsulation:**
   ```
   Attacker → Remove package → Probe flash array → Read bits optically
   ```

### 3.2 Read Protection Levels (STM32)

**Configuration:**
```c
/**
 * Configure flash read protection
 */
void configure_flash_protection(uint8_t rdp_level)
{
    FLASH_OBProgramInitTypeDef ob;

    HAL_FLASH_OB_Unlock();

    ob.OptionType = OPTIONBYTE_RDP;

    switch (rdp_level) {
    case 0:
        // No protection (development only)
        ob.RDPLevel = OB_RDP_LEVEL_0;
        break;

    case 1:
        // Protection enabled
        // - Debug disabled
        // - Flash read via debug blocked
        // - Can downgrade to Level 0 (erases flash)
        ob.RDPLevel = OB_RDP_LEVEL_1;
        break;

    case 2:
        // Maximum protection (IRREVERSIBLE)
        // - Debug permanently disabled
        // - Flash permanently protected
        // - Cannot downgrade
        ob.RDPLevel = OB_RDP_LEVEL_2;
        break;

    default:
        // Invalid level
        HAL_FLASH_OB_Lock();
        return;
    }

    // Program option bytes
    if (HAL_FLASHEx_OBProgram(&ob) != HAL_OK) {
        // Handle error
    }

    // Reload option bytes (causes reset)
    HAL_FLASH_OB_Launch();

    HAL_FLASH_OB_Lock();
}
```

### 3.3 Flash Encryption

**Encrypt Sensitive Data at Rest:**

```c
/**
 * Encrypted flash storage using device-unique key
 */

// Device-unique key (derived from hardware UID)
static psa_key_id_t device_key_id;

/**
 * Initialize device-unique encryption key
 */
psa_status_t init_device_encryption_key(void)
{
    // Get STM32 unique device ID (96 bits)
    uint32_t uid[3];
    uid[0] = HAL_GetUIDw0();
    uid[1] = HAL_GetUIDw1();
    uid[2] = HAL_GetUIDw2();

    // Derive 256-bit key from UID using HKDF
    psa_key_derivation_operation_t op = PSA_KEY_DERIVATION_OPERATION_INIT;

    psa_key_derivation_setup(&op, PSA_ALG_HKDF(PSA_ALG_SHA_256));

    // Use UID as input key material
    psa_key_derivation_input_bytes(
        &op,
        PSA_KEY_DERIVATION_INPUT_SECRET,
        (uint8_t *)uid,
        sizeof(uid)
    );

    // Salt (could be stored in OTP or flash)
    const uint8_t salt[] = "STM32U5-FlashEnc-v1";
    psa_key_derivation_input_bytes(
        &op,
        PSA_KEY_DERIVATION_INPUT_SALT,
        salt,
        sizeof(salt) - 1
    );

    // Info string
    const uint8_t info[] = "device-encryption-key";
    psa_key_derivation_input_bytes(
        &op,
        PSA_KEY_DERIVATION_INPUT_INFO,
        info,
        sizeof(info) - 1
    );

    // Generate key
    psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
    psa_set_key_algorithm(&attr, PSA_ALG_GCM);
    psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attr, 256);

    psa_status_t status = psa_key_derivation_output_key(
        &attr,
        &op,
        &device_key_id
    );

    psa_key_derivation_abort(&op);

    return status;
}

/**
 * Write encrypted data to flash
 */
psa_status_t write_encrypted_flash(uint32_t address,
                                    const uint8_t *data,
                                    size_t data_len)
{
    // Generate random IV (nonce)
    uint8_t iv[12];
    psa_generate_random(iv, sizeof(iv));

    // Encrypt data
    uint8_t ciphertext[data_len + 16];  // +16 for GCM tag
    size_t ciphertext_len;

    psa_status_t status = psa_aead_encrypt(
        device_key_id,
        PSA_ALG_GCM,
        iv,
        sizeof(iv),
        NULL, 0,  // No AAD
        data,
        data_len,
        ciphertext,
        sizeof(ciphertext),
        &ciphertext_len
    );

    if (status != PSA_SUCCESS) {
        return status;
    }

    // Write IV + ciphertext to flash
    HAL_FLASH_Unlock();

    // Write IV
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, (uint64_t *)iv);

    // Write ciphertext
    HAL_FLASH_Program(
        FLASH_TYPEPROGRAM_WORD,
        address + sizeof(iv),
        (uint64_t *)ciphertext
    );

    HAL_FLASH_Lock();

    return PSA_SUCCESS;
}

/**
 * Read and decrypt data from flash
 */
psa_status_t read_encrypted_flash(uint32_t address,
                                   uint8_t *data,
                                   size_t data_len)
{
    // Read IV
    uint8_t iv[12];
    memcpy(iv, (void *)address, sizeof(iv));

    // Read ciphertext
    uint8_t ciphertext[data_len + 16];
    memcpy(ciphertext, (void *)(address + sizeof(iv)), data_len + 16);

    // Decrypt
    size_t plaintext_len;
    psa_status_t status = psa_aead_decrypt(
        device_key_id,
        PSA_ALG_GCM,
        iv,
        sizeof(iv),
        NULL, 0,
        ciphertext,
        data_len + 16,
        data,
        data_len,
        &plaintext_len
    );

    return status;
}
```

**Benefits:**
- Even if flash is dumped, data is encrypted
- Key is device-unique (cannot use on different chip)
- Key never stored in flash (derived from UID)

---

## 4. Tamper Detection

### 4.1 Types of Tamper Events

**Physical Tamper:**
1. Case opening
2. PCB access
3. Chip removal
4. Probing attempts

**Environmental Tamper:**
1. Temperature extremes
2. Voltage manipulation
3. Clock glitching
4. Radiation/EM interference

**Logical Tamper:**
1. Unauthorized firmware
2. Invalid boot sequence
3. Memory corruption
4. Unexpected reset

### 4.2 Tamper Detection Methods

#### External Tamper Pins

**STM32 RTC Tamper:**
```c
/**
 * Configure external tamper detection
 */
void configure_tamper_detection(void)
{
    RTC_TamperTypeDef sTamper;

    // Tamper 1: Detect case opening
    // Connect tamper pin to case switch (normally closed)
    sTamper.Tamper = RTC_TAMPER_1;
    sTamper.Trigger = RTC_TAMPERTRIGGER_RISINGEDGE;  // Trigger when case opens
    sTamper.Filter = RTC_TAMPERFILTER_2SAMPLE;       // Debounce
    sTamper.SamplingFrequency = RTC_TAMPERSAMPLINGFREQ_RTCCLK_DIV256;
    sTamper.PrechargeDuration = RTC_TAMPERPRECHARGEDURATION_1RTCCLK;
    sTamper.TamperPullUp = RTC_TAMPER_PULLUP_ENABLE;
    sTamper.TimeStampOnTamperDetection = RTC_TIMESTAMPONTAMPERDETECTION_ENABLE;

    // On tamper: Erase backup registers (contains keys)
    sTamper.NoErase = RTC_TAMPER_ERASE_BACKUP_ENABLE;

    HAL_RTCEx_SetTamper_IT(&hrtc, &sTamper);

    // Tamper 2: Detect voltage attack
    sTamper.Tamper = RTC_TAMPER_2;
    sTamper.Trigger = RTC_TAMPERTRIGGER_FALLINGEDGE;  // VDD drop
    HAL_RTCEx_SetTamper_IT(&hrtc, &sTamper);

    // Enable tamper interrupt
    HAL_NVIC_SetPriority(TAMP_STAMP_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TAMP_STAMP_IRQn);
}

/**
 * Tamper interrupt handler
 */
void TAMP_STAMP_IRQHandler(void)
{
    HAL_RTCEx_TamperIRQHandler(&hrtc);
}

/**
 * Tamper detection callback
 */
void HAL_RTCEx_Tamper1EventCallback(RTC_HandleTypeDef *hrtc)
{
    // Tamper 1 detected (case opened)

    // Log event
    log_security_event(SECURITY_EVENT_TAMPER_CASE);

    // Erase all keys in RAM
    erase_all_keys();

    // Set permanent tamper flag in flash
    set_tamper_flag();

    // Disable all functionality
    enter_lockdown_mode();
}

void HAL_RTCEx_Tamper2EventCallback(RTC_HandleTypeDef *hrtc)
{
    // Tamper 2 detected (voltage glitch)

    log_security_event(SECURITY_EVENT_TAMPER_VOLTAGE);
    erase_all_keys();
    NVIC_SystemReset();
}
```

#### Mesh Network on PCB

**Conductive Mesh:**
```
┌─────────────────────────────────────┐
│  ┌───────────────────────────────┐  │
│  │  Conductive Mesh              │  │ ← Serpentine traces
│  │  ╔═══╗  ╔═══╗  ╔═══╗         │  │   covering sensitive areas
│  │  ║MCU║  ║RAM║  ║Key║         │  │
│  │  ╚═══╝  ╚═══╝  ╚═══╝         │  │
│  │                                │  │
│  │  Monitor continuity            │  │
│  └────────┬──────────────────────┘  │
│           ↓                          │
│      Tamper Pin                      │
└─────────────────────────────────────┘
```

**Implementation:**
```c
/**
 * Monitor mesh integrity
 */
void monitor_mesh_integrity(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    // Configure mesh driver pin (output)
    GPIO_InitStruct.Pin = MESH_DRIVE_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(MESH_DRIVE_PORT, &GPIO_InitStruct);

    // Configure mesh sense pin (input)
    GPIO_InitStruct.Pin = MESH_SENSE_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(MESH_SENSE_PORT, &GPIO_InitStruct);

    // Periodically check mesh
    HAL_GPIO_WritePin(MESH_DRIVE_PORT, MESH_DRIVE_PIN, GPIO_PIN_SET);
    HAL_Delay(1);  // Propagation delay

    if (HAL_GPIO_ReadPin(MESH_SENSE_PORT, MESH_SENSE_PIN) != GPIO_PIN_SET) {
        // Mesh broken (PCB tampered)
        handle_tamper_event(TAMPER_EVENT_MESH_BROKEN);
    }

    HAL_GPIO_WritePin(MESH_DRIVE_PORT, MESH_DRIVE_PIN, GPIO_PIN_RESET);
}
```

#### Active Shields

**Concept:** Shield layer that actively responds to probing

```c
/**
 * Active shield on top and bottom of PCB
 * Detects if attacker tries to probe internal signals
 */
void configure_active_shield(void)
{
    // Configure shield as alternating pattern generator
    TIM_HandleTypeDef htim_shield;

    htim_shield.Instance = TIM2;
    htim_shield.Init.Prescaler = 0;
    htim_shield.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim_shield.Init.Period = 100;  // ~1 MHz square wave
    htim_shield.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

    HAL_TIM_PWM_Init(&htim_shield);

    // Start PWM on shield layer
    HAL_TIM_PWM_Start(&htim_shield, TIM_CHANNEL_1);

    // Monitor shield signal for unexpected changes
    // (probing will load the signal and change frequency/duty cycle)
}
```

### 4.3 Tamper Response Actions

**Response Severity Levels:**

```c
typedef enum {
    TAMPER_RESPONSE_LOG = 0,        // Just log the event
    TAMPER_RESPONSE_ALERT,          // Log + notify admin
    TAMPER_RESPONSE_LOCKDOWN,       // Disable functionality
    TAMPER_RESPONSE_ERASE_KEYS,     // Erase crypto keys
    TAMPER_RESPONSE_ERASE_ALL,      // Erase all data
    TAMPER_RESPONSE_BRICK            // Permanent disable
} tamper_response_t;

void handle_tamper_event(tamper_event_t event)
{
    tamper_response_t response;

    switch (event) {
    case TAMPER_EVENT_CASE_OPEN:
        response = TAMPER_RESPONSE_ERASE_KEYS;
        break;

    case TAMPER_EVENT_VOLTAGE_GLITCH:
        response = TAMPER_RESPONSE_LOCKDOWN;
        break;

    case TAMPER_EVENT_TEMPERATURE:
        response = TAMPER_RESPONSE_ALERT;
        break;

    case TAMPER_EVENT_MESH_BROKEN:
        response = TAMPER_RESPONSE_ERASE_ALL;
        break;

    case TAMPER_EVENT_DEBUG_ACCESS:
        response = TAMPER_RESPONSE_LOG;
        break;

    default:
        response = TAMPER_RESPONSE_LOCKDOWN;
    }

    execute_tamper_response(response, event);
}

void execute_tamper_response(tamper_response_t response,
                              tamper_event_t event)
{
    // Always log
    log_tamper_event(event);

    switch (response) {
    case TAMPER_RESPONSE_LOG:
        // Already logged above
        break;

    case TAMPER_RESPONSE_ALERT:
        send_alert_to_server(event);
        break;

    case TAMPER_RESPONSE_LOCKDOWN:
        enter_lockdown_mode();
        break;

    case TAMPER_RESPONSE_ERASE_KEYS:
        erase_all_cryptographic_keys();
        enter_lockdown_mode();
        break;

    case TAMPER_RESPONSE_ERASE_ALL:
        erase_all_cryptographic_keys();
        erase_application_data();
        enter_lockdown_mode();
        break;

    case TAMPER_RESPONSE_BRICK:
        // Permanent disable (set OTP fuses)
        set_permanent_disable_flag();
        while (1) __WFI();  // Never recover
        break;
    }
}
```

---

## 5. Secure Key Storage

### 5.1 Key Storage Hierarchy

```
Security Level: Highest
    ↓
┌─────────────────────────────────────┐
│ Hardware Security Module (HSM)      │ ← Dedicated crypto chip
│ - Tamper-resistant enclosure        │   (e.g., ATECC608, SE050)
│ - Keys never leave chip             │
│ - ~$1-5 per unit                    │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ On-Chip OTP/Fuses                   │ ← One-Time Programmable
│ - Cannot be read externally         │   (STM32 has 1KB OTP)
│ - Survives flash erase              │
│ - Programmed once at factory        │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ Protected Flash (ITS/PS)            │ ← TF-M Internal Trusted Storage
│ - Encrypted with device-unique key │
│ - Rollback protection               │
│ - Read protection enabled           │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ Regular Flash                        │ ← Least secure
│ - Visible to all code               │   (avoid for secrets!)
│ - Can be dumped if RDP disabled     │
└─────────────────────────────────────┘
Security Level: Lowest
```

### 5.2 Using OTP for Root Keys

**STM32U5 OTP Region:**
```c
/**
 * STM32U5 has 1KB of One-Time Programmable (OTP) memory
 * Address: 0x0BFA_0000 - 0x0BFA_03FF
 */

#define OTP_BASE_ADDR   0x0BFA0000
#define OTP_SIZE        1024

#define OTP_ROOT_KEY_ADDR   (OTP_BASE_ADDR + 0x000)  // 32 bytes
#define OTP_DEVICE_ID_ADDR  (OTP_BASE_ADDR + 0x020)  // 16 bytes
#define OTP_MFG_DATA_ADDR   (OTP_BASE_ADDR + 0x030)  // 32 bytes

/**
 * Program root key to OTP (one-time only!)
 */
bool program_root_key_to_otp(const uint8_t *root_key, size_t key_len)
{
    if (key_len != 32) {
        return false;  // Must be 256-bit key
    }

    // Check if already programmed
    uint32_t *otp_ptr = (uint32_t *)OTP_ROOT_KEY_ADDR;
    bool already_programmed = false;

    for (int i = 0; i < 8; i++) {
        if (otp_ptr[i] != 0xFFFFFFFF) {
            already_programmed = true;
            break;
        }
    }

    if (already_programmed) {
        return false;  // OTP can only be written once!
    }

    // Unlock flash
    HAL_FLASH_Unlock();

    // Program key (word by word)
    for (int i = 0; i < 8; i++) {
        uint32_t word;
        memcpy(&word, &root_key[i * 4], 4);

        if (HAL_FLASH_Program(
                FLASH_TYPEPROGRAM_WORD,
                OTP_ROOT_KEY_ADDR + (i * 4),
                word
            ) != HAL_OK) {
            HAL_FLASH_Lock();
            return false;
        }
    }

    HAL_FLASH_Lock();

    return true;
}

/**
 * Read root key from OTP
 */
void read_root_key_from_otp(uint8_t *root_key, size_t key_len)
{
    if (key_len != 32) {
        return;
    }

    memcpy(root_key, (void *)OTP_ROOT_KEY_ADDR, 32);
}
```

### 5.3 Key Derivation from Hardware UID

**Device-Unique Keys:**
```c
/**
 * Derive cryptographic keys from hardware UID
 * This makes keys unique per device without storing them
 */

psa_status_t derive_device_unique_key(const char *purpose,
                                      psa_key_id_t *key_id)
{
    // Get 96-bit hardware UID
    uint32_t uid[3];
    uid[0] = HAL_GetUIDw0();
    uid[1] = HAL_GetUIDw1();
    uid[2] = HAL_GetUIDw2();

    // Setup HKDF derivation
    psa_key_derivation_operation_t op = PSA_KEY_DERIVATION_OPERATION_INIT;

    psa_key_derivation_setup(&op, PSA_ALG_HKDF(PSA_ALG_SHA_256));

    // Input: Hardware UID
    psa_key_derivation_input_bytes(
        &op,
        PSA_KEY_DERIVATION_INPUT_SECRET,
        (uint8_t *)uid,
        sizeof(uid)
    );

    // Salt: From OTP or fixed
    uint8_t salt[32];
    read_root_key_from_otp(salt, sizeof(salt));

    psa_key_derivation_input_bytes(
        &op,
        PSA_KEY_DERIVATION_INPUT_SALT,
        salt,
        sizeof(salt)
    );

    // Info: Purpose of this key
    psa_key_derivation_input_bytes(
        &op,
        PSA_KEY_DERIVATION_INPUT_INFO,
        (const uint8_t *)purpose,
        strlen(purpose)
    );

    // Output key
    psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
    psa_set_key_algorithm(&attr, PSA_ALG_GCM);
    psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attr, 256);
    psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);  // RAM only

    psa_status_t status = psa_key_derivation_output_key(&attr, &op, key_id);

    psa_key_derivation_abort(&op);

    return status;
}
```

**Usage:**
```c
// Derive key for storage encryption
psa_key_id_t storage_key;
derive_device_unique_key("storage-encryption-v1", &storage_key);

// Derive key for attestation
psa_key_id_t attestation_key;
derive_device_unique_key("attestation-signing-v1", &attestation_key);
```

**Benefits:**
- Keys are unique per device (different UID)
- Keys never stored in flash
- Different keys for different purposes
- Cannot be extracted even with flash dump

---

## 6. Secure Enclosures

### 6.1 Enclosure Design Principles

**Tamper-Evident vs. Tamper-Resistant:**

| Type | Goal | Cost | Examples |
|------|------|------|----------|
| **Tamper-Evident** | Detect tampering | Low | Seals, labels, special screws |
| **Tamper-Resistant** | Prevent tampering | High | Hardened enclosures, active meshes |

### 6.2 Physical Hardening Techniques

**Level 1: Basic (Consumer Devices)**
- Security screws (Torx, tri-wing)
- Tamper-evident labels
- Ultrasonic welding (no screws)

**Level 2: Enhanced (Industrial)**
- Metal enclosure
- Potting (epoxy encapsulation)
- Mesh sensors
- Case switches

**Level 3: High Security (Payment, Military)**
- Hardened steel enclosure
- Active shields
- Multiple sensor layers
- Self-destruct mechanisms
- Environmental monitoring

### 6.3 Epoxy Potting

**Application:**
```
Benefits:
✓ Prevents chip removal
✓ Hides PCB traces
✓ Makes probing very difficult
✓ Relatively cheap (~$5-20/unit)

Drawbacks:
✗ Adds weight and size
✗ Heat dissipation issues
✗ Cannot repair/debug
✗ Can be removed with chemicals (slow)
```

**Best Practices:**
- Use opaque epoxy (prevents visual inspection)
- Fill completely (no air gaps)
- Multiple layers with different hardness
- Add dummy components to confuse attacker

---

## 7. Supply Chain Security

### 7.1 Threats

**Manufacturing:**
- Malicious insider adds backdoors
- Counterfeit components substituted
- Extra units manufactured and sold (overproduction)

**Distribution:**
- Interception during shipping
- Malware injection before delivery
- Replacement with tampered units

**Deployment:**
- Installation by untrusted personnel
- Temporary access during maintenance

### 7.2 Countermeasures

**Secure Provisioning:**
```c
/**
 * Factory provisioning workflow
 */

typedef struct {
    uint8_t device_id[16];          // Unique device ID
    uint8_t provisioning_cert[512]; // Signed by factory CA
    uint8_t device_pubkey[64];      // ECDSA P-256 public key
    uint8_t factory_signature[64];  // Signature over above fields
} device_certificate_t;

/**
 * Provision device at factory with unique credentials
 */
bool factory_provision_device(device_certificate_t *cert)
{
    // 1. Generate unique keypair
    psa_key_id_t private_key;
    uint8_t public_key[64];

    psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_HASH);
    psa_set_key_algorithm(&attr, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
    psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&attr, 256);

    psa_generate_key(&attr, &private_key);

    // Export public key
    size_t pubkey_len;
    psa_export_public_key(private_key, public_key, sizeof(public_key), &pubkey_len);

    // 2. Get device unique ID
    uint32_t uid[3];
    uid[0] = HAL_GetUIDw0();
    uid[1] = HAL_GetUIDw1();
    uid[2] = HAL_GetUIDw2();

    memcpy(cert->device_id, uid, 12);

    // 3. Sign certificate with factory private key
    // (factory key is on HSM, not in device)
    uint8_t cert_hash[32];
    psa_hash_compute(
        PSA_ALG_SHA_256,
        (uint8_t *)cert,
        sizeof(*cert) - sizeof(cert->factory_signature),
        cert_hash,
        sizeof(cert_hash),
        NULL
    );

    // Sign with factory key (would be done on separate HSM)
    // psa_sign_hash(factory_key, ..., cert_hash, ..., cert->factory_signature);

    // 4. Store certificate in protected flash
    store_device_certificate(cert);

    // 5. Lock down device
    configure_flash_protection(1);  // RDP Level 1

    return true;
}
```

**Anti-Cloning:**
```c
/**
 * Device attestation to prove authenticity
 */
bool attest_device_authenticity(uint8_t *attestation_token,
                                 size_t *token_len)
{
    device_certificate_t cert;
    load_device_certificate(&cert);

    // Create attestation token
    // Contains: device_id, measurements, timestamp, signature

    uint8_t challenge[32];
    psa_generate_random(challenge, sizeof(challenge));

    // Attestation token format (simplified)
    struct {
        uint8_t device_id[16];
        uint8_t challenge[32];
        uint8_t firmware_hash[32];
        uint64_t timestamp;
        uint8_t signature[64];
    } token;

    memcpy(token.device_id, cert.device_id, 16);
    memcpy(token.challenge, challenge, 32);

    // Compute firmware hash
    compute_firmware_hash(token.firmware_hash);

    // Get timestamp
    token.timestamp = get_secure_time();

    // Sign token with device private key
    psa_key_id_t device_key;
    load_device_private_key(&device_key);

    uint8_t token_hash[32];
    psa_hash_compute(
        PSA_ALG_SHA_256,
        (uint8_t *)&token,
        sizeof(token) - sizeof(token.signature),
        token_hash,
        sizeof(token_hash),
        NULL
    );

    psa_sign_hash(
        device_key,
        PSA_ALG_ECDSA(PSA_ALG_SHA_256),
        token_hash,
        sizeof(token_hash),
        token.signature,
        sizeof(token.signature),
        NULL
    );

    // Return attestation token
    memcpy(attestation_token, &token, sizeof(token));
    *token_len = sizeof(token);

    return true;
}
```

---

## 8. Implementation on STM32U5

### 8.1 Complete Security Configuration

```c
/**
 * Configure all physical security features on STM32U5
 */
void configure_physical_security(void)
{
    // 1. Flash Read Protection Level 1
    //    (can be upgraded to Level 2 after field testing)
    configure_flash_protection(1);

    // 2. Disable JTAG (keep SWD for emergencies)
    __HAL_AFIO_REMAP_SWJ_NOJTAG();

    // 3. Configure tamper detection
    configure_tamper_detection();

    // 4. Enable voltage monitoring (PVD)
    configure_pvd_protection();

    // 5. Enable clock security system
    HAL_RCC_EnableCSS();

    // 6. Configure watchdog
    configure_watchdog();

    // 7. Setup secure boot chain
    configure_secure_boot();

    // 8. Initialize encrypted storage
    init_device_encryption_key();

    // 9. Erase any debug/provisioning data
    erase_provisioning_data();

    // 10. Set security flags
    set_production_mode_flag();
}
```

### 8.2 Production vs. Development Modes

```c
typedef enum {
    DEVICE_MODE_DEVELOPMENT = 0,
    DEVICE_MODE_PRODUCTION = 1
} device_mode_t;

device_mode_t get_device_mode(void)
{
    // Read from OTP or option bytes
    uint32_t mode = *(volatile uint32_t *)(OTP_BASE_ADDR + 0x100);

    if (mode == 0xFFFFFFFF) {
        return DEVICE_MODE_DEVELOPMENT;
    }

    return DEVICE_MODE_PRODUCTION;
}

void device_main(void)
{
    device_mode_t mode = get_device_mode();

    if (mode == DEVICE_MODE_PRODUCTION) {
        // Production: Full security
        configure_physical_security();

        // Disable all debug output
        disable_debug_uart();

        // Strict error handling
        set_strict_mode(true);

    } else {
        // Development: Relaxed security
        configure_flash_protection(0);  // No RDP

        // Enable debug UART
        enable_debug_uart();

        // Verbose error messages
        set_verbose_errors(true);
    }

    // Continue with normal application
    main_application();
}
```

---

## 📊 Summary

### Key Takeaways

1. **Physical Security is Essential**
   - Software security alone is insufficient
   - Physical access = game over (without proper mitigations)

2. **Debug Port is Major Attack Vector**
   - Always disable or protect in production
   - Use RDP Level 1 (reversible) or Level 2 (permanent)
   - Consider secure debug for field service

3. **Flash Protection is Critical**
   - Enable Read Protection (RDP)
   - Encrypt sensitive data at rest
   - Use device-unique keys

4. **Tamper Detection Adds Security**
   - External tamper pins
   - Voltage/clock monitoring
   - Mesh networks
   - Active shields (for high-security)

5. **Secure Key Storage**
   - Use OTP for root keys
   - Derive per-purpose keys from UID
   - Never store keys in regular flash

6. **Layered Defense**
   - Combine multiple techniques
   - Chip-level + enclosure + software
   - Tamper detection + response

### Security Checklist

**Minimum (All Devices):**
- [ ] RDP Level 1 or higher
- [ ] Debug ports disabled/protected
- [ ] Watchdog enabled
- [ ] Voltage monitoring (PVD)
- [ ] Clock security (CSS)
- [ ] Encrypted key storage

**Enhanced (High-Value):**
- [ ] RDP Level 2 (permanent)
- [ ] Secure debug authentication
- [ ] External tamper detection
- [ ] Mesh network on PCB
- [ ] Epoxy potting
- [ ] Anti-cloning (device certificates)

**Maximum (Critical Infrastructure):**
- [ ] All of the above, plus:
- [ ] Dedicated HSM chip
- [ ] Active shields
- [ ] Hardened enclosure
- [ ] Environmental monitoring
- [ ] Zeroization on tamper

### Next Module

**Module 05: TF-M Specific Countermeasures**
- TF-M security architecture
- Isolation levels and effectiveness
- PSA Certified requirements
- Secure firmware update
- Runtime integrity checks

**Lab 28: Physical Security Configuration** (Section 6)
- Configure RDP protection
- Setup tamper detection
- Implement encrypted storage
- Test security features

---

## 📚 References

1. **NIST SP 800-57: Key Management** - Key storage best practices
2. **Common Criteria Protection Profiles** - Physical security requirements
3. **FIPS 140-2/140-3** - Hardware security module standards
4. **IEC 62443** - Industrial security standards
5. **STM32U5 Reference Manual** - Security features documentation
6. **ARM Platform Security Architecture** - Physical protection requirements
7. **"Hardware Security: A Hands-on Learning Approach"** - Bhunia, Tehranipoor

---

**End of Module 04**

Continue to [Module 05: TF-M Countermeasures](05_theory_tfm_countermeasures.md) →

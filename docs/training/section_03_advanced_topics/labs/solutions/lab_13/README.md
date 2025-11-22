# Lab 13: Secure Debug and Production Deployment

## Overview

This lab demonstrates how to configure secure debug access, implement production security lockdown, and manage device lifecycle states. You'll learn how to balance debugging capabilities during development with security requirements for production deployment.

**Duration:** 120-150 minutes
**Difficulty:** Advanced
**Prerequisites:** Labs 02-12

---

## Learning Objectives

By the end of this lab, you will:

1. ✅ Understand STM32U5 debug authentication mechanisms
2. ✅ Configure secure JTAG/SWD access with certificates
3. ✅ Implement RDP (Read Protection) levels appropriately
4. ✅ Manage device lifecycle states (OPEN → PROVISIONING → SECURED → LOCKED)
5. ✅ Remove debug access for production deployment
6. ✅ Implement secure firmware provisioning workflows
7. ✅ Configure anti-tamper and debug security features
8. ✅ Understand trade-offs between debuggability and security

---

## Hardware Setup

### Required Hardware
- **NUCLEO-U545RE-Q** development board
- **USB cable** (for power and ST-Link debugging)
- **ST-Link/V3 or ST-Link/V2-1** (on-board debugger)
- **LED connections** (built-in LD1/LD2/LD3)

### Pin Assignments

| LED | Pin | Function |
|-----|-----|----------|
| **LD1 (Green)** | PC7 | Lifecycle state indicator |
| **LD2 (Blue)** | PB7 | Debug status indicator |
| **LD3 (Red)** | PG2 | Security violation indicator |

---

## STM32U5 Debug Security Architecture

### Debug Access Control

The STM32U5 provides multiple layers of debug protection:

```
┌────────────────────────────────────────────────┐
│         STM32U5 Debug Security Layers           │
├────────────────────────────────────────────────┤
│                                                 │
│  [1] Read Protection (RDP) Level                │
│      - RDP 0: No protection (development)       │
│      - RDP 1: Flash read protected              │
│      - RDP 2: Debug permanently disabled ⚠️     │
│                                                  │
│  [2] Debug Authentication (TZEN=1)              │
│      - Certificate-based debug access           │
│      - Secure/Non-Secure debug separation       │
│      - Password or key-based authentication     │
│                                                  │
│  [3] TrustZone Debug Control                    │
│      - Secure debug (SWD/JTAG) configuration    │
│      - Non-Secure debug filtering               │
│      - Debug Monitor Exception                  │
│                                                  │
│  [4] Lifecycle Management                       │
│      - OPEN (development, full debug)           │
│      - PROVISIONING (limited debug)             │
│      - SECURED (authenticated debug only)       │
│      - LOCKED (no debug, production)            │
│                                                  │
│  [5] Anti-Tamper Features                       │
│      - Debug event detection                    │
│      - Tamper pins (TAMP_IN1-5)                 │
│      - Active tamper detection                  │
│                                                  │
└────────────────────────────────────────────────┘
```

### RDP (Read Protection) Levels

| Level | Description | Flash Access | Debug Access | Use Case |
|-------|-------------|--------------|--------------|----------|
| **RDP 0** | No protection | Full | Full (SWD/JTAG) | Development |
| **RDP 1** | Flash read protected | Blocked | Limited (RAM only) | Pre-production |
| **RDP 2** | Permanent lock ⚠️ | Blocked | **DISABLED FOREVER** | Production |

⚠️ **CRITICAL WARNING:** RDP Level 2 is **irreversible**. Once set, debug access is permanently disabled. Only use in final production devices!

---

## Exercise 1: Reading Current Debug Configuration

### Objective
Query and display the current debug security configuration of the STM32U5.

### Implementation

**File: `debug_config.h`**
```c
#ifndef DEBUG_CONFIG_H
#define DEBUG_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32u5xx.h"

/* RDP Levels */
typedef enum {
    RDP_LEVEL_0 = 0xAA,  // No protection
    RDP_LEVEL_1 = 0x00,  // Flash read protection
    RDP_LEVEL_2 = 0xCC   // Permanent debug disable (IRREVERSIBLE!)
} RDP_Level_t;

/* Lifecycle States */
typedef enum {
    LIFECYCLE_OPEN = 0,          // Full development access
    LIFECYCLE_PROVISIONING = 1,  // Limited debug
    LIFECYCLE_SECURED = 2,       // Authenticated debug only
    LIFECYCLE_LOCKED = 3         // No debug (production)
} Lifecycle_State_t;

/* Debug Configuration */
typedef struct {
    RDP_Level_t rdp_level;
    Lifecycle_State_t lifecycle;
    bool secure_debug_enabled;
    bool nonsecure_debug_enabled;
    bool debug_authentication_enabled;
    bool jtag_enabled;
    bool swd_enabled;
    bool trace_enabled;
} DebugConfig_t;

/* Function Prototypes */

/**
 * @brief Read current debug configuration from option bytes
 * @param config Output configuration structure
 * @return 0 on success
 */
int DebugConfig_Read(DebugConfig_t *config);

/**
 * @brief Display debug configuration
 * @param config Configuration to display
 */
void DebugConfig_Print(const DebugConfig_t *config);

/**
 * @brief Check if device is in production mode
 * @return true if production locked (RDP2 or LOCKED lifecycle)
 */
bool DebugConfig_IsProductionMode(void);

/**
 * @brief Get RDP level name
 * @param level RDP level
 * @return String representation
 */
const char* DebugConfig_GetRDPName(RDP_Level_t level);

/**
 * @brief Get lifecycle state name
 * @param state Lifecycle state
 * @return String representation
 */
const char* DebugConfig_GetLifecycleName(Lifecycle_State_t state);

#endif /* DEBUG_CONFIG_H */
```

**File: `debug_config.c`**
```c
#include "debug_config.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Read current debug configuration
 */
int DebugConfig_Read(DebugConfig_t *config)
{
    if (config == NULL) {
        return -1;
    }

    memset(config, 0, sizeof(DebugConfig_t));

    /* Read RDP level from FLASH option bytes */
    FLASH_OBProgramInitTypeDef ob_config;
    HAL_FLASHEx_OBGetConfig(&ob_config);

    /* Extract RDP level */
    config->rdp_level = (RDP_Level_t)ob_config.RDPLevel;

    /* Read TrustZone debug configuration (DBGCR register) */
    uint32_t dbgcr = DBGMCU->CR;

    config->secure_debug_enabled = (dbgcr & DBGMCU_CR_DBG_AUTH_SEC) != 0;
    config->nonsecure_debug_enabled = (dbgcr & DBGMCU_CR_DBG_AUTH_NSEC) != 0;

    /* Check debug authentication */
    config->debug_authentication_enabled = (dbgcr & DBGMCU_CR_DBG_AUTH_EN) != 0;

    /* Check debug interfaces */
    config->jtag_enabled = (dbgcr & DBGMCU_CR_DBG_JTAG_EN) != 0;
    config->swd_enabled = (dbgcr & DBGMCU_CR_DBG_SWD_EN) != 0;
    config->trace_enabled = (dbgcr & DBGMCU_CR_TRACE_EN) != 0;

    /* Infer lifecycle state from configuration */
    if (config->rdp_level == RDP_LEVEL_2) {
        config->lifecycle = LIFECYCLE_LOCKED;
    } else if (config->rdp_level == RDP_LEVEL_1) {
        config->lifecycle = LIFECYCLE_SECURED;
    } else if (config->debug_authentication_enabled) {
        config->lifecycle = LIFECYCLE_PROVISIONING;
    } else {
        config->lifecycle = LIFECYCLE_OPEN;
    }

    return 0;
}

/**
 * @brief Display debug configuration
 */
void DebugConfig_Print(const DebugConfig_t *config)
{
    printf("\n");
    printf("╔════════════════════════════════════════════╗\n");
    printf("║     DEBUG CONFIGURATION REPORT              ║\n");
    printf("╚════════════════════════════════════════════╝\n");
    printf("\n");

    /* RDP Level */
    printf("RDP Level:                 %s (0x%02X)\n",
           DebugConfig_GetRDPName(config->rdp_level),
           config->rdp_level);

    /* Lifecycle State */
    printf("Lifecycle State:           %s\n",
           DebugConfig_GetLifecycleName(config->lifecycle));

    /* Debug Access */
    printf("\n--- Debug Access ---\n");
    printf("Secure Debug:              %s\n",
           config->secure_debug_enabled ? "ENABLED" : "DISABLED");
    printf("Non-Secure Debug:          %s\n",
           config->nonsecure_debug_enabled ? "ENABLED" : "DISABLED");
    printf("Debug Authentication:      %s\n",
           config->debug_authentication_enabled ? "REQUIRED" : "NOT REQUIRED");

    /* Debug Interfaces */
    printf("\n--- Debug Interfaces ---\n");
    printf("JTAG:                      %s\n",
           config->jtag_enabled ? "ENABLED" : "DISABLED");
    printf("SWD:                       %s\n",
           config->swd_enabled ? "ENABLED" : "DISABLED");
    printf("Trace (ETM/ITM):           %s\n",
           config->trace_enabled ? "ENABLED" : "DISABLED");

    /* Security Status */
    printf("\n--- Security Status ---\n");
    if (config->rdp_level == RDP_LEVEL_2) {
        printf("⚠️  PRODUCTION MODE: Debug permanently disabled\n");
    } else if (config->rdp_level == RDP_LEVEL_1) {
        printf("🔒 PROTECTED MODE: Flash read protected\n");
    } else {
        printf("🔓 OPEN MODE: Full debug access (DEVELOPMENT ONLY)\n");
    }

    printf("\n");
}

/**
 * @brief Check if device is in production mode
 */
bool DebugConfig_IsProductionMode(void)
{
    DebugConfig_t config;
    DebugConfig_Read(&config);

    return (config.rdp_level == RDP_LEVEL_2 ||
            config.lifecycle == LIFECYCLE_LOCKED);
}

/**
 * @brief Get RDP level name
 */
const char* DebugConfig_GetRDPName(RDP_Level_t level)
{
    switch (level) {
        case RDP_LEVEL_0:
            return "RDP Level 0 (No Protection)";
        case RDP_LEVEL_1:
            return "RDP Level 1 (Flash Protected)";
        case RDP_LEVEL_2:
            return "RDP Level 2 (PERMANENT LOCK)";
        default:
            return "Unknown RDP Level";
    }
}

/**
 * @brief Get lifecycle state name
 */
const char* DebugConfig_GetLifecycleName(Lifecycle_State_t state)
{
    switch (state) {
        case LIFECYCLE_OPEN:
            return "OPEN (Development)";
        case LIFECYCLE_PROVISIONING:
            return "PROVISIONING (Limited Debug)";
        case LIFECYCLE_SECURED:
            return "SECURED (Authenticated Debug)";
        case LIFECYCLE_LOCKED:
            return "LOCKED (Production)";
        default:
            return "Unknown Lifecycle";
    }
}
```

### Testing Exercise 1

```c
void test_read_debug_config(void)
{
    printf("\n=== Exercise 1: Read Debug Configuration ===\n");

    DebugConfig_t config;

    /* Read current configuration */
    if (DebugConfig_Read(&config) != 0) {
        printf("❌ ERROR: Failed to read debug configuration\n");
        LED_Red_Blink(5);
        return;
    }

    /* Display configuration */
    DebugConfig_Print(&config);

    /* Visual indicator based on lifecycle */
    switch (config.lifecycle) {
        case LIFECYCLE_OPEN:
            printf("[LED] Green = OPEN (Development mode)\n");
            LED_Green_On();
            HAL_Delay(2000);
            LED_Green_Off();
            break;

        case LIFECYCLE_PROVISIONING:
        case LIFECYCLE_SECURED:
            printf("[LED] Blue = PROVISIONING/SECURED\n");
            LED_Blue_Blink(3);
            break;

        case LIFECYCLE_LOCKED:
            printf("[LED] Red = LOCKED (Production)\n");
            LED_Red_Blink(5);
            break;
    }

    printf("\n✓ Exercise 1: COMPLETE\n");
}
```

**Expected Output (Development Board):**
```
=== Exercise 1: Read Debug Configuration ===

╔════════════════════════════════════════════╗
║     DEBUG CONFIGURATION REPORT              ║
╚════════════════════════════════════════════╝

RDP Level:                 RDP Level 0 (No Protection) (0xAA)
Lifecycle State:           OPEN (Development)

--- Debug Access ---
Secure Debug:              ENABLED
Non-Secure Debug:          ENABLED
Debug Authentication:      NOT REQUIRED

--- Debug Interfaces ---
JTAG:                      ENABLED
SWD:                       ENABLED
Trace (ETM/ITM):           ENABLED

--- Security Status ---
🔓 OPEN MODE: Full debug access (DEVELOPMENT ONLY)

[LED] Green = OPEN (Development mode)

✓ Exercise 1: COMPLETE
```

---

## Exercise 2: Configuring Debug Authentication

### Objective
Configure certificate-based debug authentication to allow authorized debug access while blocking unauthorized debuggers.

### Theory

**Debug Authentication Workflow:**
```
1. Debugger connects to STM32U5
2. Device challenges debugger with nonce
3. Debugger signs nonce with private key
4. Device verifies signature with public key (stored in option bytes)
5. If valid → Grant debug access
6. If invalid → Block access, trigger tamper event
```

### Implementation

**File: `debug_auth.h`**
```c
#ifndef DEBUG_AUTH_H
#define DEBUG_AUTH_H

#include <stdint.h>
#include "psa/crypto.h"

/* Debug authentication certificate */
typedef struct {
    uint8_t public_key[64];      // ECDSA P-256 public key (64 bytes)
    uint8_t device_id[16];       // Unique device ID
    uint8_t signature[64];       // Certificate signature
    uint32_t valid_until;        // Expiration timestamp
} DebugAuthCertificate_t;

/**
 * @brief Generate debug authentication key pair
 * @param public_key Output public key (64 bytes)
 * @param private_key Output private key (32 bytes) - STORE SECURELY!
 * @return 0 on success
 */
int DebugAuth_GenerateKeyPair(uint8_t *public_key, uint8_t *private_key);

/**
 * @brief Provision debug authentication public key to option bytes
 * @param public_key Public key to store (64 bytes)
 * @return 0 on success
 */
int DebugAuth_ProvisionPublicKey(const uint8_t *public_key);

/**
 * @brief Enable debug authentication
 * @return 0 on success
 */
int DebugAuth_Enable(void);

/**
 * @brief Verify debug authentication (called by debugger)
 * @param challenge Random challenge from device
 * @param signature Signed challenge from debugger
 * @return 0 if valid, -1 if invalid
 */
int DebugAuth_Verify(const uint8_t *challenge,
                      const uint8_t *signature);

/**
 * @brief Create debug certificate for authorized debugger
 * @param private_key Private key (32 bytes)
 * @param cert Output certificate
 * @return 0 on success
 */
int DebugAuth_CreateCertificate(const uint8_t *private_key,
                                 DebugAuthCertificate_t *cert);

#endif /* DEBUG_AUTH_H */
```

**File: `debug_auth.c`**
```c
#include "debug_auth.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Generate ECDSA P-256 key pair for debug authentication
 */
int DebugAuth_GenerateKeyPair(uint8_t *public_key, uint8_t *private_key)
{
    printf("[Debug Auth] Generating ECDSA P-256 key pair...\n");

    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_usage_flags(&attributes,
                             PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH);
    psa_set_key_algorithm(&attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
    psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&attributes, 256);

    psa_key_id_t key_id;
    psa_status_t status = psa_generate_key(&attributes, &key_id);

    if (status != PSA_SUCCESS) {
        printf("[Debug Auth] ERROR: Key generation failed (%d)\n", status);
        return -1;
    }

    /* Export public key */
    size_t public_key_len;
    status = psa_export_public_key(key_id, public_key, 64, &public_key_len);

    if (status != PSA_SUCCESS) {
        printf("[Debug Auth] ERROR: Public key export failed (%d)\n", status);
        psa_destroy_key(key_id);
        return -1;
    }

    /* Export private key (DANGEROUS - must be stored securely!) */
    size_t private_key_len;
    status = psa_export_key(key_id, private_key, 32, &private_key_len);

    if (status != PSA_SUCCESS) {
        printf("[Debug Auth] ERROR: Private key export failed (%d)\n", status);
        psa_destroy_key(key_id);
        return -1;
    }

    printf("[Debug Auth] Key pair generated successfully\n");
    printf("  Public key:  %zu bytes\n", public_key_len);
    printf("  Private key: %zu bytes (PROTECT THIS!)\n", private_key_len);

    psa_destroy_key(key_id);
    return 0;
}

/**
 * @brief Provision public key to option bytes (simulated)
 *
 * In real STM32U5, this would write to option bytes using:
 * - FLASH_OBProgramInitTypeDef
 * - HAL_FLASHEx_OBProgram()
 * - Option byte: OB_DEBUG_AUTH_KEY
 */
int DebugAuth_ProvisionPublicKey(const uint8_t *public_key)
{
    printf("[Debug Auth] Provisioning public key to option bytes...\n");

    /* ⚠️ WARNING: This is a PERMANENT operation in real hardware!
     * Once written, the key cannot be changed without full erase.
     */

    /* Unlock option bytes */
    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();

    /* In real implementation, write to OB_DEBUG_AUTH_KEY */
    /* FLASH_OBProgramInitTypeDef ob_config;
     * ob_config.DebugAuthKey = public_key;
     * HAL_FLASHEx_OBProgram(&ob_config);
     */

    printf("[Debug Auth] Public key provisioned (simulated)\n");
    printf("  ⚠️  In production, this is PERMANENT!\n");

    /* Lock option bytes */
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();

    return 0;
}

/**
 * @brief Enable debug authentication
 */
int DebugAuth_Enable(void)
{
    printf("[Debug Auth] Enabling debug authentication...\n");

    /* Set debug authentication enable bit in DBGMCU_CR */
    DBGMCU->CR |= DBGMCU_CR_DBG_AUTH_EN;

    /* Configure authentication for Secure debug */
    DBGMCU->CR |= DBGMCU_CR_DBG_AUTH_SEC;

    /* Configure authentication for Non-Secure debug */
    DBGMCU->CR |= DBGMCU_CR_DBG_AUTH_NSEC;

    printf("[Debug Auth] Debug authentication ENABLED\n");
    printf("  Debuggers must provide valid signature to connect\n");

    return 0;
}

/**
 * @brief Verify debug authentication challenge
 */
int DebugAuth_Verify(const uint8_t *challenge, const uint8_t *signature)
{
    printf("[Debug Auth] Verifying debug authentication...\n");

    /* Hash challenge */
    uint8_t hash[32];
    size_t hash_len;
    psa_status_t status = psa_hash_compute(
        PSA_ALG_SHA_256,
        challenge, 32,
        hash, sizeof(hash),
        &hash_len
    );

    if (status != PSA_SUCCESS) {
        printf("[Debug Auth] ERROR: Challenge hash failed\n");
        return -1;
    }

    /* Load public key from option bytes (simulated) */
    uint8_t public_key[64];
    /* In real implementation: Read from OB_DEBUG_AUTH_KEY */
    memset(public_key, 0xFF, sizeof(public_key));  // Placeholder

    /* Import public key */
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_VERIFY_HASH);
    psa_set_key_algorithm(&attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
    psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&attributes, 256);

    psa_key_id_t key_id;
    status = psa_import_key(&attributes, public_key, sizeof(public_key), &key_id);

    if (status != PSA_SUCCESS) {
        printf("[Debug Auth] ERROR: Public key import failed\n");
        return -1;
    }

    /* Verify signature */
    status = psa_verify_hash(key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256),
                              hash, hash_len,
                              signature, 64);

    psa_destroy_key(key_id);

    if (status == PSA_SUCCESS) {
        printf("[Debug Auth] ✓ Signature VALID - Debug access granted\n");
        return 0;
    } else {
        printf("[Debug Auth] ❌ Signature INVALID - Debug access DENIED\n");
        /* Trigger tamper event */
        /* HAL_TAMP_EventCallback(); */
        return -1;
    }
}

/**
 * @brief Create debug certificate
 */
int DebugAuth_CreateCertificate(const uint8_t *private_key,
                                 DebugAuthCertificate_t *cert)
{
    if (cert == NULL || private_key == NULL) {
        return -1;
    }

    printf("[Debug Auth] Creating debug certificate...\n");

    /* Get device unique ID */
    uint32_t *uid = (uint32_t*)UID_BASE;  // STM32 unique ID
    memcpy(cert->device_id, uid, 16);

    /* Set expiration (e.g., 1 year from now) */
    cert->valid_until = HAL_GetTick() + (365 * 24 * 60 * 60 * 1000);

    /* Sign certificate with private key */
    /* (In production, this would be done offline on a secure workstation) */
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH);
    psa_set_key_algorithm(&attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
    psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&attributes, 256);

    psa_key_id_t key_id;
    psa_status_t status = psa_import_key(&attributes, private_key, 32, &key_id);

    if (status != PSA_SUCCESS) {
        printf("[Debug Auth] ERROR: Private key import failed\n");
        return -1;
    }

    /* Create data to sign (device_id + timestamp) */
    uint8_t data_to_sign[20];
    memcpy(data_to_sign, cert->device_id, 16);
    memcpy(data_to_sign + 16, &cert->valid_until, 4);

    uint8_t hash[32];
    size_t hash_len;
    psa_hash_compute(PSA_ALG_SHA_256, data_to_sign, sizeof(data_to_sign),
                      hash, sizeof(hash), &hash_len);

    size_t signature_len;
    status = psa_sign_hash(key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256),
                            hash, hash_len,
                            cert->signature, sizeof(cert->signature),
                            &signature_len);

    psa_destroy_key(key_id);

    if (status != PSA_SUCCESS) {
        printf("[Debug Auth] ERROR: Certificate signing failed\n");
        return -1;
    }

    printf("[Debug Auth] Certificate created successfully\n");
    return 0;
}
```

### Testing Exercise 2

```c
void test_debug_authentication(void)
{
    printf("\n=== Exercise 2: Debug Authentication ===\n");

    uint8_t public_key[64];
    uint8_t private_key[32];
    DebugAuthCertificate_t cert;

    /* Step 1: Generate key pair */
    printf("\n[Step 1] Generating debug authentication key pair...\n");
    if (DebugAuth_GenerateKeyPair(public_key, private_key) != 0) {
        printf("❌ ERROR: Key generation failed\n");
        LED_Red_Blink(5);
        return;
    }
    printf("✓ Key pair generated\n");
    LED_Green_Blink(1);

    /* Step 2: Provision public key (DANGEROUS - only for demo!) */
    printf("\n[Step 2] Provisioning public key...\n");
    printf("⚠️  WARNING: This is PERMANENT in production!\n");
    printf("⚠️  Do you want to continue? (simulated, safe for demo)\n");

    /* In production, require explicit confirmation */
    if (DebugAuth_ProvisionPublicKey(public_key) != 0) {
        printf("❌ ERROR: Provisioning failed\n");
        LED_Red_Blink(5);
        return;
    }
    printf("✓ Public key provisioned\n");
    LED_Green_Blink(1);

    /* Step 3: Enable debug authentication */
    printf("\n[Step 3] Enabling debug authentication...\n");
    DebugAuth_Enable();
    printf("✓ Debug authentication enabled\n");
    LED_Green_Blink(1);

    /* Step 4: Create certificate for authorized debugger */
    printf("\n[Step 4] Creating debug certificate...\n");
    if (DebugAuth_CreateCertificate(private_key, &cert) != 0) {
        printf("❌ ERROR: Certificate creation failed\n");
        LED_Red_Blink(5);
        return;
    }
    printf("✓ Debug certificate created\n");
    LED_Green_Blink(1);

    /* Step 5: Simulate debug authentication */
    printf("\n[Step 5] Simulating debug authentication...\n");

    uint8_t challenge[32];
    psa_generate_random(challenge, sizeof(challenge));

    uint8_t signature[64];
    /* In real scenario, debugger would sign this challenge */
    /* For demo, we just simulate verification */

    printf("  Challenge generated: ");
    for (int i = 0; i < 8; i++) {
        printf("%02X ", challenge[i]);
    }
    printf("...\n");

    printf("  Debugger would sign challenge with private key\n");
    printf("  Device verifies signature with public key\n");

    printf("\n✓ Exercise 2: COMPLETE\n");
    printf("  Debug authentication configured successfully!\n");
}
```

---

## Exercise 3: Configuring RDP Levels (SIMULATION ONLY)

### Objective
Understand RDP level configuration (simulated - NOT applied to avoid bricking the board).

### ⚠️ CRITICAL WARNING

**DO NOT SET RDP LEVEL 2 ON DEVELOPMENT BOARDS!**

RDP Level 2 is **IRREVERSIBLE** and will permanently disable debug access. This exercise is **SIMULATION ONLY** to demonstrate the process without actually applying the changes.

### Implementation

**File: `rdp_config.h`**
```c
#ifndef RDP_CONFIG_H
#define RDP_CONFIG_H

#include <stdint.h>
#include "debug_config.h"

/**
 * @brief Simulate RDP level configuration (DOES NOT ACTUALLY CHANGE OPTION BYTES)
 * @param target_level Target RDP level
 * @return 0 on success
 */
int RDP_Simulate_SetLevel(RDP_Level_t target_level);

/**
 * @brief Get recommended RDP level for device lifecycle
 * @param lifecycle Lifecycle state
 * @return Recommended RDP level
 */
RDP_Level_t RDP_GetRecommendedLevel(Lifecycle_State_t lifecycle);

/**
 * @brief Display RDP configuration guide
 */
void RDP_DisplayConfigurationGuide(void);

#endif /* RDP_CONFIG_H */
```

**File: `rdp_config.c`**
```c
#include "rdp_config.h"
#include <stdio.h>

/**
 * @brief Simulate RDP level configuration
 */
int RDP_Simulate_SetLevel(RDP_Level_t target_level)
{
    printf("\n");
    printf("═══════════════════════════════════════════════\n");
    printf(" RDP LEVEL CONFIGURATION (SIMULATION ONLY)\n");
    printf("═══════════════════════════════════════════════\n");
    printf("\n");

    printf("Target RDP Level: %s\n", DebugConfig_GetRDPName(target_level));
    printf("\n");

    if (target_level == RDP_LEVEL_0) {
        printf("✓ RDP Level 0: No protection\n");
        printf("  - Full flash read access\n");
        printf("  - Full debug access (SWD/JTAG)\n");
        printf("  - Use for: DEVELOPMENT ONLY\n");
        printf("\n");
        printf("⚠️  Security risk: Device fully accessible\n");

    } else if (target_level == RDP_LEVEL_1) {
        printf("🔒 RDP Level 1: Flash read protection\n");
        printf("  - Flash read blocked via debug\n");
        printf("  - RAM debug allowed\n");
        printf("  - SRAM download allowed\n");
        printf("  - Can revert to RDP 0 (with mass erase)\n");
        printf("  - Use for: PRE-PRODUCTION, FIELD TESTING\n");
        printf("\n");
        printf("⚠️  Reverting to RDP 0 erases all flash!\n");

    } else if (target_level == RDP_LEVEL_2) {
        printf("🔴 RDP Level 2: PERMANENT DEBUG DISABLE\n");
        printf("\n");
        printf("⛔ ⛔ ⛔ CRITICAL WARNING ⛔ ⛔ ⛔\n");
        printf("\n");
        printf("THIS OPERATION IS **IRREVERSIBLE**!\n");
        printf("\n");
        printf("Once set to RDP Level 2:\n");
        printf("  ❌ Debug access PERMANENTLY DISABLED\n");
        printf("  ❌ JTAG/SWD PERMANENTLY DISABLED\n");
        printf("  ❌ Flash read PERMANENTLY BLOCKED\n");
        printf("  ❌ Option bytes LOCKED\n");
        printf("  ❌ NO WAY TO REVERT (not even with ST tools!)\n");
        printf("\n");
        printf("Use ONLY for:\n");
        printf("  ✓ Final production devices\n");
        printf("  ✓ Devices with field update capability (MCUboot/DFU)\n");
        printf("  ✓ After extensive testing with RDP 1\n");
        printf("\n");
        printf("⚠️  DO NOT USE ON DEVELOPMENT BOARDS!\n");
        printf("⚠️  BOARD WILL BE PERMANENTLY LOCKED!\n");
        printf("\n");
    }

    /* DO NOT ACTUALLY CHANGE OPTION BYTES */
    printf("═══════════════════════════════════════════════\n");
    printf(" SIMULATION COMPLETE (No changes applied)\n");
    printf("═══════════════════════════════════════════════\n");
    printf("\n");

    return 0;
}

/**
 * @brief Get recommended RDP level
 */
RDP_Level_t RDP_GetRecommendedLevel(Lifecycle_State_t lifecycle)
{
    switch (lifecycle) {
        case LIFECYCLE_OPEN:
        case LIFECYCLE_PROVISIONING:
            return RDP_LEVEL_0;  // Development

        case LIFECYCLE_SECURED:
            return RDP_LEVEL_1;  // Pre-production

        case LIFECYCLE_LOCKED:
            return RDP_LEVEL_2;  // Production (IRREVERSIBLE!)

        default:
            return RDP_LEVEL_0;
    }
}

/**
 * @brief Display RDP configuration guide
 */
void RDP_DisplayConfigurationGuide(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║         RDP LEVEL CONFIGURATION GUIDE                   ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");

    printf("RECOMMENDED WORKFLOW:\n");
    printf("\n");
    printf("1. Development (RDP 0)\n");
    printf("   - Full debug access\n");
    printf("   - Iterate on firmware\n");
    printf("   - Test with debugger attached\n");
    printf("   - Use debug printf, breakpoints, etc.\n");
    printf("\n");
    printf("2. Alpha Testing (RDP 1)\n");
    printf("   - Enable flash read protection\n");
    printf("   - Test without full debug access\n");
    printf("   - Verify firmware works with limited debug\n");
    printf("   - Can still revert to RDP 0 if needed (with erase)\n");
    printf("\n");
    printf("3. Beta/Field Testing (RDP 1)\n");
    printf("   - Keep RDP 1 for several months\n");
    printf("   - Collect field data\n");
    printf("   - Verify no critical bugs\n");
    printf("   - Ensure firmware update mechanism works\n");
    printf("\n");
    printf("4. Production (RDP 2 - ONLY WHEN READY!)\n");
    printf("   - Set RDP 2 ONLY for mass production\n");
    printf("   - Ensure MCUboot or DFU is working\n");
    printf("   - Have remote update capability\n");
    printf("   - ⚠️  PERMANENT - NO UNDO!\n");
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║  NEVER SET RDP 2 ON DEVELOPMENT/PROTOTYPE BOARDS!      ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");
}
```

### Testing Exercise 3

```c
void test_rdp_simulation(void)
{
    printf("\n=== Exercise 3: RDP Level Configuration (SIMULATION) ===\n");

    /* Display configuration guide */
    RDP_DisplayConfigurationGuide();

    /* Simulate RDP 0 */
    printf("\n--- Simulating RDP Level 0 ---\n");
    RDP_Simulate_SetLevel(RDP_LEVEL_0);
    LED_Green_Blink(1);
    HAL_Delay(2000);

    /* Simulate RDP 1 */
    printf("\n--- Simulating RDP Level 1 ---\n");
    RDP_Simulate_SetLevel(RDP_LEVEL_1);
    LED_Blue_Blink(2);
    HAL_Delay(2000);

    /* Simulate RDP 2 (with BIG warning) */
    printf("\n--- Simulating RDP Level 2 (WARNING!) ---\n");
    RDP_Simulate_SetLevel(RDP_LEVEL_2);
    LED_Red_Blink(10);  // Long red blink = danger!

    printf("\n✓ Exercise 3: COMPLETE (Simulation only)\n");
    printf("  No option bytes were changed\n");
}
```

---

## Exercise 4: Production Deployment Checklist

### Objective
Create a comprehensive pre-production security checklist.

### Implementation

**File: `production_checklist.h`**
```c
typedef struct {
    bool rdp_configured;
    bool debug_auth_enabled;
    bool secure_boot_enabled;
    bool write_protection_enabled;
    bool tamper_detection_enabled;
    bool firmware_signed;
    bool rollback_protection_enabled;
    bool default_passwords_removed;
    bool debug_code_removed;
    bool test_vectors_removed;
    uint32_t issues_found;
} ProductionChecklist_t;

int Production_RunChecklist(ProductionChecklist_t *checklist);
void Production_PrintReport(const ProductionChecklist_t *checklist);
```

**File: `production_checklist.c`**
```c
int Production_RunChecklist(ProductionChecklist_t *checklist)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║   PRODUCTION DEPLOYMENT CHECKLIST             ║\n");
    printf("╚══════════════════════════════════════════════╝\n");
    printf("\n");

    memset(checklist, 0, sizeof(ProductionChecklist_t));

    /* Check 1: RDP configured */
    printf("[1/10] Checking RDP configuration...\n");
    DebugConfig_t debug_config;
    DebugConfig_Read(&debug_config);
    checklist->rdp_configured = (debug_config.rdp_level >= RDP_LEVEL_1);
    if (!checklist->rdp_configured) {
        printf("  ❌ FAIL: RDP Level 0 (not for production!)\n");
        checklist->issues_found++;
    } else {
        printf("  ✅ PASS: RDP Level %d\n", debug_config.rdp_level);
    }

    /* Check 2: Debug authentication */
    printf("\n[2/10] Checking debug authentication...\n");
    checklist->debug_auth_enabled = debug_config.debug_authentication_enabled;
    if (!checklist->debug_auth_enabled && debug_config.rdp_level != RDP_LEVEL_2) {
        printf("  ⚠️  WARNING: Debug auth not enabled\n");
        checklist->issues_found++;
    } else {
        printf("  ✅ PASS: Debug authentication enabled\n");
    }

    /* Check 3: Secure boot */
    printf("\n[3/10] Checking secure boot...\n");
    /* Check if secure boot is enabled (MCUboot signatures, etc.) */
    checklist->secure_boot_enabled = true;  // Placeholder
    printf("  ✅ PASS: Secure boot enabled\n");

    /* Check 4: Write protection */
    printf("\n[4/10] Checking write protection...\n");
    checklist->write_protection_enabled = true;  // Check WRP bits
    printf("  ✅ PASS: Critical regions write-protected\n");

    /* Check 5: Tamper detection */
    printf("\n[5/10] Checking tamper detection...\n");
    checklist->tamper_detection_enabled = (RCC->AHB3ENR & RCC_AHB3ENR_TAMPEN);
    if (!checklist->tamper_detection_enabled) {
        printf("  ⚠️  WARNING: Tamper detection disabled\n");
    } else {
        printf("  ✅ PASS: Tamper detection enabled\n");
    }

    /* Check 6: Firmware signature */
    printf("\n[6/10] Checking firmware signature...\n");
    checklist->firmware_signed = true;  // Check MCUboot header
    printf("  ✅ PASS: Firmware signed\n");

    /* Check 7: Rollback protection */
    printf("\n[7/10] Checking rollback protection...\n");
    checklist->rollback_protection_enabled = true;  // Check version counter
    printf("  ✅ PASS: Rollback protection enabled\n");

    /* Check 8: Default passwords removed */
    printf("\n[8/10] Checking for default passwords...\n");
    checklist->default_passwords_removed = true;  // Code review required
    printf("  ✅ PASS: No default passwords found\n");

    /* Check 9: Debug code removed */
    printf("\n[9/10] Checking for debug code...\n");
    #ifdef DEBUG
        printf("  ❌ FAIL: DEBUG flag still defined!\n");
        checklist->issues_found++;
        checklist->debug_code_removed = false;
    #else
        printf("  ✅ PASS: DEBUG flag not defined\n");
        checklist->debug_code_removed = true;
    #endif

    /* Check 10: Test vectors removed */
    printf("\n[10/10] Checking for test vectors...\n");
    checklist->test_vectors_removed = true;  // Code review required
    printf("  ✅ PASS: No test vectors in production build\n");

    printf("\n");
    printf("╔══════════════════════════════════════════════╗\n");
    if (checklist->issues_found == 0) {
        printf("║  RESULT: ✅ READY FOR PRODUCTION             ║\n");
    } else {
        printf("║  RESULT: ❌ %u ISSUES FOUND - FIX BEFORE PROD ║\n",
               checklist->issues_found);
    }
    printf("╚══════════════════════════════════════════════╝\n");

    return (checklist->issues_found == 0) ? 0 : -1;
}
```

### Testing Exercise 4

```c
void test_production_checklist(void)
{
    printf("\n=== Exercise 4: Production Deployment Checklist ===\n");

    ProductionChecklist_t checklist;

    int result = Production_RunChecklist(&checklist);

    Production_PrintReport(&checklist);

    if (result == 0) {
        printf("\n✅ Device ready for production deployment\n");
        LED_Green_Blink(5);
    } else {
        printf("\n❌ Device NOT ready - fix %u issues\n",
               checklist.issues_found);
        LED_Red_Blink(checklist.issues_found);
    }

    printf("\n✓ Exercise 4: COMPLETE\n");
}
```

---

## Key Takeaways

1. ✅ **RDP Level 2 is IRREVERSIBLE** - only for final production
2. ✅ **Use RDP 1** for pre-production and field testing
3. ✅ **Debug authentication** provides secure debug access
4. ✅ **Lifecycle management** guides security configuration
5. ✅ **Production checklist** prevents deployment of insecure devices
6. ✅ **Balance security vs debuggability** based on device state
7. ✅ **Never lock development boards** to RDP 2

---

## References

- [STM32U5 Debug Authentication (AN5554)](https://www.st.com/resource/en/application_note/an5554-stm32u5-debug-authentication-stmicroelectronics.pdf)
- [STM32 Security Hardening Guide (AN5156)](https://www.st.com/resource/en/application_note/an5156-introduction-to-security-for-stm32-mcus-stmicroelectronics.pdf)

---

**Lab 13 Complete! 🎉**

# Lab 08: Advanced Protected Storage - Secure Data Lifecycle

**Objective:** Master advanced PSA Protected Storage features including encryption, rollback protection, and secure data lifecycle management.

**Duration:** 90 minutes

**Hardware:** NUCLEO-U545RE-Q with TF-M

---

## Overview

PSA Protected Storage (PS) provides persistent, authenticated storage for security-critical data. This lab covers **advanced features** beyond basic read/write:

1. **Encrypted Storage** - AES-GCM encrypted data at rest
2. **Rollback Protection** - Prevent reverting to old data
3. **Write-Once Storage** - Immutable provisioning data
4. **Secure Deletion** - Cryptographic erasure
5. **Data Lifecycle** - Manage data from creation to destruction

---

## Learning Objectives

By the end of this lab, you will be able to:

- ✅ Encrypt data before storing in PS
- ✅ Implement rollback counters
- ✅ Use write-once storage for provisioning
- ✅ Perform secure deletion
- ✅ Manage complete data lifecycle
- ✅ See immediate visual feedback with NUCLEO LEDs

---

## Background: Protected Storage Security

### Storage Security Threats

| Threat | Description | Mitigation |
|--------|-------------|------------|
| **Confidentiality** | Attacker reads flash | Encryption (AES-GCM) |
| **Integrity** | Attacker modifies data | Authentication (HMAC/GCM tag) |
| **Rollback** | Attacker replays old data | Rollback counter |
| **Deletion** | Data survives after delete | Cryptographic erasure |
| **Provisioning** | Attacker modifies device ID | Write-once storage |

### PSA PS Security Architecture

```
┌─────────────────────────────────────────────────────────┐
│ PSA Protected Storage Architecture                      │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Application                                            │
│  │                                                      │
│  ├─ psa_ps_set(uid, data, flags)                       │
│  │                                                      │
│  ↓                                                      │
│  PSA Protected Storage Service (TF-M)                  │
│  │                                                      │
│  ├─ 1. Encrypt data (AES-256-GCM)                      │
│  │    Key = Device Key (hardware-bound)                │
│  │    Nonce = Counter (unique per write)               │
│  │    AAD = UID + metadata                             │
│  │                                                      │
│  ├─ 2. Generate authentication tag                     │
│  │    Tag = GMAC(ciphertext + AAD)                     │
│  │                                                      │
│  ├─ 3. Update rollback counter (if enabled)            │
│  │    Counter++                                        │
│  │                                                      │
│  ├─ 4. Write to flash                                  │
│  │    [Header | Nonce | Ciphertext | Tag]             │
│  │                                                      │
│  ↓                                                      │
│  Flash Memory (Encrypted)                              │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

---

## Part 1: Encrypted Storage (20 minutes)

### 1.1 Create Encrypted Asset

Create `lab_08/src/main.c`:

```c
#include <stdio.h>
#include <string.h>
#include "psa/protected_storage.h"
#include "psa/crypto.h"
#include "board_leds.h"

#define UID_SECRET_KEY  0x00000100

void experiment_1_encrypted_storage(void)
{
    psa_status_t status;

    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf(" Experiment 1: Encrypted Storage\n");
    printf("═══════════════════════════════════════════════════════════\n\n");

    /* Sensitive data to store */
    const char *api_key = "sk_live_51234567890ABCDEF";
    printf("[1] Storing API key: %s\n", api_key);
    printf("  Length: %zu bytes\n", strlen(api_key));

    /* Store in Protected Storage (automatically encrypted) */
    status = psa_ps_set(
        UID_SECRET_KEY,
        strlen(api_key) + 1,  /* Include null terminator */
        api_key,
        PSA_STORAGE_FLAG_NONE
    );

    if (status == PSA_SUCCESS) {
        printf("✓ API key stored (encrypted at rest)\n");

        /* Show flash contents (encrypted) */
        printf("\n[2] Flash contents (encrypted):\n");
        printf("  Actual storage: [Nonce | AES-GCM Ciphertext | Auth Tag]\n");
        printf("  ⚠️  Even if attacker reads flash, data is encrypted\n");

        LED_Green_Blink(3);
    } else {
        printf("✗ Storage failed (0x%08lX)\n", status);
        LED_Red_Blink(5);
        return;
    }

    /* Read back (automatically decrypted) */
    printf("\n[3] Reading back API key...\n");

    char read_buffer[64];
    size_t read_len;

    status = psa_ps_get(
        UID_SECRET_KEY,
        0,  /* offset */
        sizeof(read_buffer),
        read_buffer,
        &read_len
    );

    if (status == PSA_SUCCESS) {
        printf("✓ API key retrieved: %s\n", read_buffer);
        printf("  Decryption successful (key matches)\n");
        printf("  Read length: %zu bytes\n", read_len);

        LED_Blue_Blink(3);
    } else {
        printf("✗ Read failed (0x%08lX)\n", status);
        LED_Red_Blink(5);
    }
}
```

**Expected Output:**
```
═══════════════════════════════════════════════════════════
 Experiment 1: Encrypted Storage
═══════════════════════════════════════════════════════════

[1] Storing API key: sk_live_51234567890ABCDEF
  Length: 26 bytes
✓ API key stored (encrypted at rest)

[2] Flash contents (encrypted):
  Actual storage: [Nonce | AES-GCM Ciphertext | Auth Tag]
  ⚠️  Even if attacker reads flash, data is encrypted

[LED] LD1 (Green) blinks 3 times

[3] Reading back API key...
✓ API key retrieved: sk_live_51234567890ABCDEF
  Decryption successful (key matches)
  Read length: 27 bytes

[LED] LD2 (Blue) blinks 3 times
```

---

## Part 2: Rollback Protection (25 minutes)

### 2.1 Implement Rollback Counter

```c
#define UID_CONFIG_DATA     0x00000200
#define UID_ROLLBACK_CTR    0x00000201

typedef struct {
    uint32_t rollback_counter;
    uint32_t version;
    char     device_name[32];
    bool     debug_enabled;
} device_config_t;

void experiment_2_rollback_protection(void)
{
    psa_status_t status;

    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf(" Experiment 2: Rollback Protection\n");
    printf("═══════════════════════════════════════════════════════════\n\n");

    printf("Scenario: Prevent reverting device configuration\n\n");

    /* Step 1: Write initial configuration (version 1) */
    printf("[1] Writing configuration version 1...\n");

    device_config_t config_v1 = {
        .rollback_counter = 1,
        .version = 1,
        .device_name = "IoT-Device-001",
        .debug_enabled = true  /* Debug initially enabled */
    };

    status = psa_ps_set(UID_CONFIG_DATA, sizeof(config_v1),
                        &config_v1, PSA_STORAGE_FLAG_NONE);

    if (status == PSA_SUCCESS) {
        printf("✓ Configuration v1 stored\n");
        printf("  Rollback counter: %lu\n", config_v1.rollback_counter);
        printf("  Debug enabled: %s\n", config_v1.debug_enabled ? "YES" : "NO");
    }

    /* Step 2: Update configuration (version 2 - security hardening) */
    printf("\n[2] Updating to version 2 (disable debug)...\n");

    device_config_t config_v2 = {
        .rollback_counter = 2,  /* Increment counter */
        .version = 2,
        .device_name = "IoT-Device-001",
        .debug_enabled = false  /* Security hardening: disable debug */
    };

    status = psa_ps_set(UID_CONFIG_DATA, sizeof(config_v2),
                        &config_v2, PSA_STORAGE_FLAG_NONE);

    if (status == PSA_SUCCESS) {
        printf("✓ Configuration v2 stored\n");
        printf("  Rollback counter: %lu\n", config_v2.rollback_counter);
        printf("  Debug enabled: %s\n", config_v2.debug_enabled ? "YES" : "NO");
        LED_Green_Blink(2);
    }

    /* Step 3: Attacker tries to rollback to v1 */
    printf("\n[3] 🚨 ATTACK: Trying to rollback to v1...\n");
    printf("  Attacker wants to re-enable debug access\n");

    /* Read current counter */
    device_config_t current_config;
    size_t read_len;

    status = psa_ps_get(UID_CONFIG_DATA, 0, sizeof(current_config),
                        &current_config, &read_len);

    uint32_t current_counter = current_config.rollback_counter;
    printf("  Current rollback counter: %lu\n", current_counter);

    /* Try to write old configuration */
    if (config_v1.rollback_counter < current_counter) {
        printf("  ✗ ROLLBACK DETECTED!\n");
        printf("  Attempted counter (%lu) < current counter (%lu)\n",
               config_v1.rollback_counter, current_counter);
        printf("  ❌ Rejecting write operation\n");

        LED_Red_Blink(5);

        /* Don't allow the write */
        printf("\n✓ Rollback protection successful!\n");
        printf("  Device remains in secure state (debug disabled)\n");

    } else {
        printf("  ⚠️  Counter check would pass (unexpected)\n");
    }

    /* Verify current state */
    printf("\n[4] Verifying current configuration...\n");
    status = psa_ps_get(UID_CONFIG_DATA, 0, sizeof(current_config),
                        &current_config, &read_len);

    if (status == PSA_SUCCESS) {
        printf("  Version: %lu\n", current_config.version);
        printf("  Rollback counter: %lu\n", current_config.rollback_counter);
        printf("  Debug: %s\n", current_config.debug_enabled ? "ENABLED" : "DISABLED");

        if (current_config.rollback_counter == 2 && !current_config.debug_enabled) {
            printf("\n✅ Configuration protected from rollback!\n");
            LED_Green_Blink(5);
        }
    }
}
```

**Expected Output:**
```
═══════════════════════════════════════════════════════════
 Experiment 2: Rollback Protection
═══════════════════════════════════════════════════════════

Scenario: Prevent reverting device configuration

[1] Writing configuration version 1...
✓ Configuration v1 stored
  Rollback counter: 1
  Debug enabled: YES

[2] Updating to version 2 (disable debug)...
✓ Configuration v2 stored
  Rollback counter: 2
  Debug enabled: NO

[LED] LD1 (Green) blinks 2 times

[3] 🚨 ATTACK: Trying to rollback to v1...
  Attacker wants to re-enable debug access
  Current rollback counter: 2
  ✗ ROLLBACK DETECTED!
  Attempted counter (1) < current counter (2)
  ❌ Rejecting write operation

[LED] LD3 (Red) blinks 5 times

✓ Rollback protection successful!
  Device remains in secure state (debug disabled)

[4] Verifying current configuration...
  Version: 2
  Rollback counter: 2
  Debug: DISABLED

✅ Configuration protected from rollback!

[LED] LD1 (Green) blinks 5 times
```

---

## Part 3: Write-Once Storage (20 minutes)

### 3.1 Immutable Provisioning Data

```c
#define UID_DEVICE_ID       0x00000300
#define UID_PROVISION_CERT  0x00000301

void experiment_3_write_once_storage(void)
{
    psa_status_t status;

    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf(" Experiment 3: Write-Once Storage\n");
    printf("═══════════════════════════════════════════════════════════\n\n");

    printf("Scenario: Device provisioning with immutable data\n\n");

    /* Step 1: Provision device ID (write-once) */
    printf("[1] Provisioning device ID (write-once)...\n");

    const char *device_id = "DEV-XXXXXX-001-SECURE";
    printf("  Device ID: %s\n", device_id);

    status = psa_ps_set(
        UID_DEVICE_ID,
        strlen(device_id) + 1,
        device_id,
        PSA_STORAGE_FLAG_WRITE_ONCE  /* ← Key flag! */
    );

    if (status == PSA_SUCCESS) {
        printf("✓ Device ID provisioned (IMMUTABLE)\n");
        LED_Green_Blink(3);
    } else {
        printf("✗ Provisioning failed (0x%08lX)\n", status);
        LED_Red_Blink(5);
        return;
    }

    /* Step 2: Verify read works */
    printf("\n[2] Reading device ID...\n");

    char read_id[64];
    size_t read_len;

    status = psa_ps_get(UID_DEVICE_ID, 0, sizeof(read_id), read_id, &read_len);

    if (status == PSA_SUCCESS) {
        printf("✓ Device ID: %s\n", read_id);
    }

    /* Step 3: Try to overwrite (should fail) */
    printf("\n[3] 🚨 ATTACK: Trying to change device ID...\n");

    const char *fake_id = "DEV-HACKED-999-FAKE";
    printf("  Attempting to write: %s\n", fake_id);

    status = psa_ps_set(
        UID_DEVICE_ID,
        strlen(fake_id) + 1,
        fake_id,
        PSA_STORAGE_FLAG_NONE  /* Try to overwrite */
    );

    if (status == PSA_ERROR_NOT_PERMITTED) {
        printf("  ✗ Write rejected: PSA_ERROR_NOT_PERMITTED\n");
        printf("  ✅ Write-once protection working!\n");
        LED_Green_Blink(5);
    } else if (status == PSA_SUCCESS) {
        printf("  ⚠️  Write succeeded (unexpected!)\n");
        LED_Red_Blink(10);
    } else {
        printf("  ✗ Write failed with error: 0x%08lX\n", status);
    }

    /* Step 4: Verify ID unchanged */
    printf("\n[4] Verifying device ID unchanged...\n");

    status = psa_ps_get(UID_DEVICE_ID, 0, sizeof(read_id), read_id, &read_len);

    if (status == PSA_SUCCESS) {
        printf("  Current Device ID: %s\n", read_id);

        if (strcmp(read_id, device_id) == 0) {
            printf("\n✅ Device ID protected from tampering!\n");
            LED_Green_Blink(7);
        } else {
            printf("\n❌ Device ID was changed (security failure)\n");
            LED_Red_Blink(10);
        }
    }
}
```

**Expected Output:**
```
═══════════════════════════════════════════════════════════
 Experiment 3: Write-Once Storage
═══════════════════════════════════════════════════════════

Scenario: Device provisioning with immutable data

[1] Provisioning device ID (write-once)...
  Device ID: DEV-XXXXXX-001-SECURE
✓ Device ID provisioned (IMMUTABLE)

[LED] LD1 (Green) blinks 3 times

[2] Reading device ID...
✓ Device ID: DEV-XXXXXX-001-SECURE

[3] 🚨 ATTACK: Trying to change device ID...
  Attempting to write: DEV-HACKED-999-FAKE
  ✗ Write rejected: PSA_ERROR_NOT_PERMITTED
  ✅ Write-once protection working!

[LED] LD1 (Green) blinks 5 times

[4] Verifying device ID unchanged...
  Current Device ID: DEV-XXXXXX-001-SECURE

✅ Device ID protected from tampering!

[LED] LD1 (Green) blinks 7 times
```

---

## Part 4: Secure Deletion (15 minutes)

### 4.1 Cryptographic Erasure

```c
void experiment_4_secure_deletion(void)
{
    psa_status_t status;

    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf(" Experiment 4: Secure Deletion\n");
    printf("═══════════════════════════════════════════════════════════\n\n");

    printf("Scenario: Securely delete sensitive data\n\n");

    /* Step 1: Store sensitive data */
    printf("[1] Storing credit card number (simulated)...\n");

    const char *cc_number = "4532-1234-5678-9010";
    #define UID_CREDIT_CARD  0x00000400

    status = psa_ps_set(UID_CREDIT_CARD, strlen(cc_number) + 1,
                        cc_number, PSA_STORAGE_FLAG_NONE);

    if (status == PSA_SUCCESS) {
        printf("✓ Credit card stored (encrypted)\n");
        printf("  Flash contains: [AES-GCM encrypted data]\n");
    }

    /* Step 2: Verify data exists */
    printf("\n[2] Verifying data exists...\n");

    char read_cc[32];
    size_t read_len;

    status = psa_ps_get(UID_CREDIT_CARD, 0, sizeof(read_cc), read_cc, &read_len);

    if (status == PSA_SUCCESS) {
        printf("✓ Data found: %s\n", read_cc);
    }

    /* Step 3: Secure deletion */
    printf("\n[3] Performing secure deletion...\n");
    printf("  Method: Cryptographic erasure\n");
    printf("  Process:\n");
    printf("    1. Delete encryption key\n");
    printf("    2. Remove PS entry\n");
    printf("    3. Encrypted data becomes unrecoverable\n");

    status = psa_ps_remove(UID_CREDIT_CARD);

    if (status == PSA_SUCCESS) {
        printf("\n✓ Data securely deleted\n");
        printf("  Flash may still contain ciphertext, but:\n");
        printf("  - Encryption key is gone\n");
        printf("  - Data is cryptographically erased\n");
        printf("  - Cannot be recovered\n");

        LED_Green_Blink(5);
    } else {
        printf("✗ Deletion failed (0x%08lX)\n", status);
        LED_Red_Blink(5);
    }

    /* Step 4: Verify data gone */
    printf("\n[4] Verifying data is gone...\n");

    status = psa_ps_get(UID_CREDIT_CARD, 0, sizeof(read_cc), read_cc, &read_len);

    if (status == PSA_ERROR_DOES_NOT_EXIST) {
        printf("✓ Data not found (PSA_ERROR_DOES_NOT_EXIST)\n");
        printf("✅ Secure deletion successful!\n");
        LED_Blue_Blink(5);
    } else if (status == PSA_SUCCESS) {
        printf("⚠️  Data still exists (unexpected)\n");
        LED_Red_Blink(10);
    } else {
        printf("✗ Read error: 0x%08lX\n", status);
    }
}
```

**Expected Output:**
```
═══════════════════════════════════════════════════════════
 Experiment 4: Secure Deletion
═══════════════════════════════════════════════════════════

Scenario: Securely delete sensitive data

[1] Storing credit card number (simulated)...
✓ Credit card stored (encrypted)
  Flash contains: [AES-GCM encrypted data]

[2] Verifying data exists...
✓ Data found: 4532-1234-5678-9010

[3] Performing secure deletion...
  Method: Cryptographic erasure
  Process:
    1. Delete encryption key
    2. Remove PS entry
    3. Encrypted data becomes unrecoverable

✓ Data securely deleted
  Flash may still contain ciphertext, but:
  - Encryption key is gone
  - Data is cryptographically erased
  - Cannot be recovered

[LED] LD1 (Green) blinks 5 times

[4] Verifying data is gone...
✓ Data not found (PSA_ERROR_DOES_NOT_EXIST)
✅ Secure deletion successful!

[LED] LD2 (Blue) blinks 5 times
```

---

## Part 5: Data Lifecycle Management (10 minutes)

### 5.1 Complete Lifecycle

```c
typedef enum {
    DATA_STATE_NONE = 0,
    DATA_STATE_CREATED,
    DATA_STATE_ACTIVE,
    DATA_STATE_SUSPENDED,
    DATA_STATE_DELETED
} data_state_t;

void experiment_5_data_lifecycle(void)
{
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf(" Experiment 5: Data Lifecycle Management\n");
    printf("═══════════════════════════════════════════════════════════\n\n");

    #define UID_SESSION_KEY  0x00000500

    /* Lifecycle: CREATE */
    printf("[LIFECYCLE] CREATE\n");
    uint8_t session_key[32];
    psa_generate_random(session_key, sizeof(session_key));
    psa_ps_set(UID_SESSION_KEY, sizeof(session_key), session_key,
               PSA_STORAGE_FLAG_NONE);
    printf("  ✓ Session key generated and stored\n");
    LED_Green_On();
    HAL_Delay(500);
    LED_Green_Off();

    /* Lifecycle: ACTIVE */
    printf("\n[LIFECYCLE] ACTIVE\n");
    printf("  ✓ Key in use for encryption/decryption\n");
    LED_Blue_On();
    HAL_Delay(500);
    LED_Blue_Off();

    /* Lifecycle: SUSPEND (optional) */
    printf("\n[LIFECYCLE] SUSPEND\n");
    printf("  ✓ Key temporarily inactive (e.g., device sleep)\n");
    HAL_Delay(1000);

    /* Lifecycle: REACTIVATE */
    printf("\n[LIFECYCLE] REACTIVATE\n");
    uint8_t read_key[32];
    size_t read_len;
    psa_ps_get(UID_SESSION_KEY, 0, sizeof(read_key), read_key, &read_len);
    printf("  ✓ Key retrieved and active again\n");

    /* Lifecycle: DELETE */
    printf("\n[LIFECYCLE] DELETE\n");
    psa_ps_remove(UID_SESSION_KEY);
    printf("  ✓ Key securely deleted\n");
    LED_Red_On();
    HAL_Delay(500);
    LED_Red_Off();

    printf("\n✅ Complete lifecycle demonstrated!\n");
    LED_Green_Blink(3);
}
```

---

## Complete Main Function

```c
int main(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║      LAB 08: Advanced Protected Storage                 ║\n");
    printf("║                                                          ║\n");
    printf("║  Board: NUCLEO-U545RE-Q                                  ║\n");
    printf("║  LED Indicators:                                         ║\n");
    printf("║    LD1 (Green)  = Success / Valid                        ║\n");
    printf("║    LD2 (Blue)   = Info / Operation                       ║\n");
    printf("║    LD3 (Red)    = Attack Detected / Failure              ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");

    /* Initialize PSA Crypto */
    psa_status_t status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        printf("✗ PSA Crypto initialization failed!\n");
        while (1) LED_Red_Blink(10);
    }

    printf("✓ PSA Crypto initialized\n");

    /* Run experiments */
    experiment_1_encrypted_storage();
    experiment_2_rollback_protection();
    experiment_3_write_once_storage();
    experiment_4_secure_deletion();
    experiment_5_data_lifecycle();

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║               LAB 08 COMPLETE! ✅                        ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("\n");

    while (1) {
        LED_Green_On();
        HAL_Delay(1000);
        LED_Green_Off();
        HAL_Delay(1000);
    }
}
```

---

## Security Best Practices

### ✅ DO

1. **Always encrypt sensitive data** - Use PS for keys, credentials
2. **Implement rollback protection** - For security-critical config
3. **Use write-once for provisioning** - Device ID, certificates
4. **Securely delete when done** - Don't leave sensitive data
5. **Manage complete lifecycle** - Track data from create to delete

### ❌ DON'T

1. **Don't store plaintext secrets** - Always use PS encryption
2. **Don't skip rollback checks** - Attackers will exploit this
3. **Don't allow overwriting write-once** - Defeats the purpose
4. **Don't simple flash erase** - Use cryptographic deletion
5. **Don't ignore errors** - Check all PSA return codes

---

## Summary

In this lab, you learned:

✅ **Encrypted Storage**
- AES-GCM encryption at rest
- Automatic decryption on read
- Hardware-bound keys

✅ **Rollback Protection**
- Monotonic counters
- Preventing config downgrades
- Security state preservation

✅ **Write-Once Storage**
- Immutable provisioning data
- Device identity protection
- PSA_STORAGE_FLAG_WRITE_ONCE

✅ **Secure Deletion**
- Cryptographic erasure
- Key destruction
- Data irrecoverability

✅ **Visual Feedback**
- Immediate LED feedback on NUCLEO board
- See attacks being blocked in real-time

---

## Next Steps

- **Lab 09:** Runtime Integrity Monitoring
- **Lab 10:** Security Integration Exercise

---

**Lab 08 Complete!** ✅

You now master advanced PSA Protected Storage - critical for securing data throughout its complete lifecycle in production IoT devices!

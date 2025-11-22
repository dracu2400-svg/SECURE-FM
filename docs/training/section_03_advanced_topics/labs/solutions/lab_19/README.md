# Lab 19: Hardware Security Module (HSM) Integration

## Overview

Integrate external secure elements (ATECC608A) with TF-M for hardware-based key storage, secure boot, and cryptographic acceleration.

**Duration:** 120 minutes
**Difficulty:** Advanced
**Prerequisites:** Labs 02-18

---

## Learning Objectives

1. ✅ Understand HSM architecture and use cases
2. ✅ Integrate ATECC608A secure element via I2C
3. ✅ Store cryptographic keys in hardware
4. ✅ Perform hardware-accelerated crypto operations
5. ✅ Implement secure boot with HSM root of trust
6. ✅ Compare HSM vs software crypto performance

---

## ATECC608A Secure Element

### Features
- **Secure Key Storage:** 16 slots for ECC/AES keys
- **Hardware Crypto:** ECDSA P-256, SHA-256, AES-128
- **Tamper Protection:** Physical and logical protection
- **Secure Boot:** Stores root public key
- **I2C Interface:** Easy integration with STM32

---

## Exercise 1: ATECC608A Initialization

### I2C Communication

```c
#include "atca_basic.h"
#include "atca_cfgs.h"

ATCAIfaceCfg atca_i2c_cfg = {
    .iface_type = ATCA_I2C_IFACE,
    .devtype = ATECC608A,
    .atcai2c.slave_address = 0xC0,
    .atcai2c.bus = 1,
    .atcai2c.baud = 400000,
    .wake_delay = 1500,
    .rx_retries = 20
};

int HSM_Init(void)
{
    ATCA_STATUS status;

    printf("[HSM] Initializing ATECC608A...\n");

    /* Initialize CryptoAuthLib */
    status = atcab_init(&atca_i2c_cfg);
    if (status != ATCA_SUCCESS) {
        printf("[HSM] ERROR: Init failed (0x%02X)\n", status);
        return -1;
    }

    /* Read device serial number */
    uint8_t serial_num[9];
    atcab_read_serial_number(serial_num);

    printf("[HSM] ATECC608A initialized\n");
    printf("  Serial: ");
    for (int i = 0; i < 9; i++) {
        printf("%02X", serial_num[i]);
    }
    printf("\n");

    return 0;
}
```

---

## Exercise 2: Hardware Key Storage

### Generate and Store ECC Key

```c
int HSM_GenerateKey(uint16_t slot_id)
{
    ATCA_STATUS status;
    uint8_t public_key[64];

    printf("[HSM] Generating ECC P-256 key in slot %u...\n", slot_id);

    /* Generate private key (stored in device, never exported) */
    status = atcab_genkey(slot_id, public_key);

    if (status == ATCA_SUCCESS) {
        printf("[HSM] ✓ Key generated successfully\n");
        printf("  Public key: ");
        for (int i = 0; i < 64; i++) {
            printf("%02X", public_key[i]);
        }
        printf("\n");
        printf("  Private key: PROTECTED (never leaves HSM)\n");
        return 0;
    } else {
        printf("[HSM] ❌ Key generation failed (0x%02X)\n", status);
        return -1;
    }
}
```

---

## Exercise 3: Hardware-Accelerated Signing

### Sign Data with HSM

```c
int HSM_SignData(uint16_t slot_id, const uint8_t *data, size_t data_len,
                  uint8_t *signature)
{
    ATCA_STATUS status;
    uint8_t digest[32];

    /* Hash data with SHA-256 */
    status = atcab_sha(data_len, data, digest);
    if (status != ATCA_SUCCESS) {
        return -1;
    }

    /* Sign hash with private key (in HSM) */
    status = atcab_sign(slot_id, digest, signature);

    if (status == ATCA_SUCCESS) {
        printf("[HSM] ✓ Data signed with hardware key\n");
        return 0;
    } else {
        printf("[HSM] ❌ Signing failed (0x%02X)\n", status);
        return -1;
    }
}

void test_hsm_signing(void)
{
    uint16_t slot_id = 0;
    uint8_t data[] = "Secure firmware v1.0";
    uint8_t signature[64];

    /* Generate key in HSM */
    HSM_GenerateKey(slot_id);

    /* Sign data */
    if (HSM_SignData(slot_id, data, sizeof(data), signature) == 0) {
        printf("✓ Signature: ");
        for (int i = 0; i < 64; i++) {
            printf("%02X", signature[i]);
        }
        printf("\n");
        LED_Green_Blink(3);
    }
}
```

---

## Exercise 4: HSM-Based Secure Boot

### Store Root Public Key in HSM

```c
int HSM_ProvisionRootKey(const uint8_t *root_public_key)
{
    uint16_t root_key_slot = 15;  // Reserved for root key

    /* Write public key to HSM (one-time, locked) */
    ATCA_STATUS status = atcab_write_pubkey(root_key_slot, root_public_key);

    if (status == ATCA_SUCCESS) {
        /* Lock slot (cannot be changed) */
        atcab_lock_data_slot(root_key_slot);

        printf("[HSM] ✓ Root public key provisioned and LOCKED\n");
        return 0;
    }

    return -1;
}

int HSM_VerifyFirmwareSignature(const uint8_t *firmware,
                                  size_t firmware_len,
                                  const uint8_t *signature)
{
    uint8_t digest[32];
    bool is_verified = false;

    /* Hash firmware */
    atcab_sha(firmware_len, firmware, digest);

    /* Verify signature with root public key (from HSM) */
    ATCA_STATUS status = atcab_verify_extern(digest, signature,
                                              NULL, &is_verified);

    if (status == ATCA_SUCCESS && is_verified) {
        printf("[HSM] ✓ Firmware signature VALID\n");
        return 0;
    } else {
        printf("[HSM] ❌ Firmware signature INVALID\n");
        return -1;
    }
}
```

---

## Exercise 5: Performance Comparison

### Benchmark: HSM vs Software Crypto

```c
void benchmark_hsm_vs_software(void)
{
    uint8_t data[256];
    uint8_t signature_hsm[64];
    uint8_t signature_sw[64];

    /* Benchmark HSM signing */
    uint32_t start = HAL_GetTick();
    for (int i = 0; i < 100; i++) {
        HSM_SignData(0, data, sizeof(data), signature_hsm);
    }
    uint32_t hsm_time = HAL_GetTick() - start;

    /* Benchmark software signing (PSA Crypto) */
    start = HAL_GetTick();
    for (int i = 0; i < 100; i++) {
        psa_sign_hash(key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256),
                      data, 32, signature_sw, 64, &sig_len);
    }
    uint32_t sw_time = HAL_GetTick() - start;

    printf("\n=== Performance Comparison (100 operations) ===\n");
    printf("HSM (ATECC608A):  %lu ms (%lu ms/op)\n",
           hsm_time, hsm_time / 100);
    printf("Software (PSA):   %lu ms (%lu ms/op)\n",
           sw_time, sw_time / 100);

    if (hsm_time < sw_time) {
        printf("✓ HSM is FASTER by %lu ms\n", sw_time - hsm_time);
    } else {
        printf("⚠️  Software is faster (HSM limited by I2C)\n");
    }
}
```

---

## Key Takeaways

1. ✅ **HSM provides tamper-proof key storage** (keys never leave device)
2. ✅ **Hardware crypto acceleration** for ECDSA, SHA-256, AES
3. ✅ **Root of trust for secure boot** (immutable root keys)
4. ✅ **Physical security** against probing and side-channel attacks
5. ✅ **I2C bottleneck** may limit performance vs software crypto
6. ✅ **Best for high-security applications** (payment, medical, automotive)

---

**Lab 19 Complete! 🎉**

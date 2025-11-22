---
marp: true
theme: default
paginate: true
backgroundColor: #ffffff
header: 'TF-M Training Package - Section 4'
footer: 'Integration & Deployment | © 2025'
---

<!-- _class: lead -->
<!-- _paginate: false -->

# Integration & Deployment

**RTOS, Debug & Production**

![bg right:40%](https://via.placeholder.com/400x300/0066CC/ffffff?text=Production)

*TF-M Training Package - Section 4*

---

## Section Overview

### Topics

1. **RTOS Integration** (FreeRTOS, Zephyr)
2. **MCUboot Secure Boot**
3. **Firmware Update (OTA)**
4. **Debug & Testing**
5. **Production Deployment**

---

## FreeRTOS + TrustZone-M

### Task Isolation

```c
/* Non-Secure FreeRTOS tasks */
void vTask1(void *pvParameters)
{
    while (1) {
        /* Call secure service */
        psa_crypto_init();

        /* Regular task work */
        process_data();

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Secure world has TF-M SPM (not FreeRTOS) */
```

**FreeRTOS runs in Non-Secure world only**

---

## Context Switching Security

```c
/* Secure context saved on NS→S transition */
typedef struct {
    uint32_t r0_r3[4];
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;
} SecureContext_t;

/* Hardware automatically:
   1. Saves NS context
   2. Clears NS registers
   3. Switches to S stack
   4. Loads S context
*/
```

---

## FreeRTOS Configuration

```c
/* FreeRTOSConfig.h for TrustZone */
#define configENABLE_TRUSTZONE             1
#define configRUN_FREERTOS_SECURE_ONLY     0

/* Secure context allocation */
#define configMINIMAL_SECURE_STACK_SIZE    1024

/* Allocate secure context for each task */
SecureContextHandle_t xSecureContext;
xSecureContext = SecureContext_AllocateContext(
    configMINIMAL_SECURE_STACK_SIZE);
```

---

## Zephyr RTOS Integration

```c
/* Zephyr with TF-M */
CONFIG_TFM_PARTITION_CRYPTO=y
CONFIG_TFM_PARTITION_STORAGE=y
CONFIG_TFM_PARTITION_ATTESTATION=y

/* Application code */
#include <tfm_ns_interface.h>

void main(void)
{
    tfm_ns_interface_init();

    /* Use PSA APIs */
    psa_crypto_init();
}
```

**Zephyr has built-in TF-M support**

---

## Secure Boot Process

```
Power On
    │
    ▼
ROM Bootloader
    │ (Verify MCUboot signature)
    ▼
MCUboot
    │ (Verify TF-M + App signature)
    ▼
TF-M Secure Partition Manager
    │
    ▼
Non-Secure Application
```

**Each stage cryptographically verified**

---

## MCUboot Configuration

```c
/* mcuboot_config.h */
#define MCUBOOT_SIGN_EC256           /* ECDSA P-256 */
#define MCUBOOT_ENCRYPT_EC256        /* Encrypted images */
#define MCUBOOT_HW_ROLLBACK_PROT     /* Rollback protection */
#define MCUBOOT_VALIDATE_PRIMARY_SLOT
#define MCUBOOT_VALIDATE_SECONDARY_SLOT

/* Flash layout */
#define FLASH_AREA_IMAGE_0_OFFSET    0x10000  /* Primary */
#define FLASH_AREA_IMAGE_0_SIZE      0x80000  /* 512 KB */
#define FLASH_AREA_IMAGE_1_OFFSET    0x90000  /* Secondary */
#define FLASH_AREA_IMAGE_1_SIZE      0x80000  /* 512 KB */
```

---

## Firmware Update Over-The-Air (OTA)

### Update Flow

```c
void ota_update(void)
{
    /* 1. Download new firmware */
    https_download("https://server.com/fw.bin", fw_buffer);

    /* 2. Verify signature before writing */
    if (verify_signature(fw_buffer) != 0) {
        printf("Invalid signature!\n");
        return;
    }

    /* 3. Write to secondary slot */
    flash_write(SECONDARY_SLOT_ADDR, fw_buffer, fw_size);

    /* 4. Mark for swap */
    boot_set_pending(0);

    /* 5. Reboot */
    NVIC_SystemReset();
}
```

---

## OTA Security Considerations

### Secure Download

```c
/* TLS 1.3 with certificate pinning */
mbedtls_ssl_config_defaults(&conf,
    MBEDTLS_SSL_IS_CLIENT,
    MBEDTLS_SSL_TRANSPORT_STREAM,
    MBEDTLS_SSL_PRESET_DEFAULT);

/* Pin server certificate */
mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);

/* Minimum TLS 1.2 */
mbedtls_ssl_conf_min_version(&conf,
    MBEDTLS_SSL_MAJOR_VERSION_3,
    MBEDTLS_SSL_MINOR_VERSION_3);
```

---

## Incremental Firmware Updates

```c
/* Download and verify in chunks */
SHA256_CTX ctx;
SHA256_Init(&ctx);

for (uint32_t offset = 0; offset < total_size; offset += CHUNK_SIZE) {
    /* Download chunk */
    uint8_t chunk[CHUNK_SIZE];
    https_download_range(url, offset, CHUNK_SIZE, chunk);

    /* Update hash */
    SHA256_Update(&ctx, chunk, CHUNK_SIZE);

    /* Write to flash */
    flash_write(SECONDARY_SLOT + offset, chunk, CHUNK_SIZE);

    /* Show progress */
    printf("Progress: %lu%%\n", (offset * 100) / total_size);
}

/* Verify final hash */
uint8_t hash[32];
SHA256_Final(hash, &ctx);
verify_hash(hash);
```

---

## Debug Configurations

### Development vs Production

| Feature | Development | Production |
|---------|-------------|------------|
| **JTAG** | ✅ Enabled | ❌ Disabled |
| **RDP** | Level 0 | Level 2 |
| **Logs** | Verbose | Minimal |
| **Assertions** | Enabled | Disabled |
| **Test Code** | Included | Removed |

---

## Debug Authentication

### Secure Debug Access

```c
/* Debug certificate (signed by manufacturer) */
typedef struct {
    uint8_t device_id[16];
    uint32_t permissions;  /* JTAG, Flash read, etc. */
    uint64_t expiration;
    uint8_t signature[64];  /* ECDSA P-256 */
} DebugCertificate_t;

/* Device validates cert before enabling debug */
int enable_debug(const DebugCertificate_t *cert)
{
    if (verify_debug_cert(cert) != 0) {
        return -1;  /* Invalid cert */
    }

    /* Temporarily enable JTAG */
    enable_jtag(cert->permissions);
    return 0;
}
```

---

## RDP (Read Protection) Levels

### STM32U5 Protection

**Level 0 (OPEN):**
- Full debug access
- Flash readable
- **Development only**

**Level 1 (PROTECTED):**
- Debug disabled while running
- Flash readable when halted
- **Testing phase**

**Level 2 (LOCKED):**
- ⚠️ **PERMANENT** - Cannot revert!
- JTAG disabled
- Flash unreadable
- **Production only**

---

## Setting RDP Level

```c
/* STM32CubeProgrammer CLI */
$ STM32_Programmer_CLI -c port=SWD -ob RDP=0xBB  /* Level 1 */

/* WARNING: Level 2 is IRREVERSIBLE! */
$ STM32_Programmer_CLI -c port=SWD -ob RDP=0xCC  /* Level 2 */
```

**⚠️ CRITICAL:** Test extensively before Level 2!

---

## Production Readiness Checklist

### Security Audit

- [ ] Secure boot enabled and tested
- [ ] RDP Level 2 configured
- [ ] Debug ports disabled
- [ ] Test code removed
- [ ] Secrets provisioned
- [ ] OTA update tested
- [ ] Rollback protection verified
- [ ] Tamper detection active
- [ ] Watchdog enabled
- [ ] Stack overflow protection
- [ ] Memory encryption enabled
- [ ] Certificate chain validated

---

## Factory Provisioning

```c
void factory_provision(void)
{
    /* 1. Generate unique device ID */
    uint8_t device_id[16];
    psa_generate_random(device_id, sizeof(device_id));
    psa_its_set(UID_DEVICE_ID, 16, device_id,
                PSA_STORAGE_FLAG_WRITE_ONCE);

    /* 2. Generate device key pair */
    psa_key_id_t device_key;
    psa_generate_key(&attributes, &device_key);

    /* 3. Create CSR and get certificate */
    create_csr(device_id, device_key);
    uint8_t cert[1024];
    receive_certificate(cert);

    /* 4. Store certificate */
    psa_ps_set(UID_DEVICE_CERT, sizeof(cert), cert, 0);

    /* 5. Set RDP Level 1 */
    set_rdp_level(RDP_LEVEL_1);

    printf("Provisioning complete\n");
}
```

---

## Manufacturing Test Points

```c
/* GPIO test points for automated testing */
#define TEST_POINT_CRYPTO   GPIO_PIN_0
#define TEST_POINT_STORAGE  GPIO_PIN_1
#define TEST_POINT_COMM     GPIO_PIN_2

void manufacturing_test(void)
{
    /* Test 1: Crypto */
    if (test_crypto() == 0) {
        GPIO_SetHigh(TEST_POINT_CRYPTO);
    }

    /* Test 2: Storage */
    if (test_storage() == 0) {
        GPIO_SetHigh(TEST_POINT_STORAGE);
    }

    /* Test 3: Communication */
    if (test_communication() == 0) {
        GPIO_SetHigh(TEST_POINT_COMM);
    }

    /* All tests passed → Set RDP Level 2 */
    if (all_tests_passed()) {
        set_rdp_level(RDP_LEVEL_2);
    }
}
```

---

## Continuous Integration (CI/CD)

```yaml
# .github/workflows/firmware-build.yml
name: Firmware Build & Test

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2

      - name: Install ARM GCC
        run: sudo apt install gcc-arm-none-eabi

      - name: Build Secure + Non-Secure
        run: |
          cmake -S . -B build
          cmake --build build

      - name: Run Unit Tests
        run: ./build/tests/unit_tests

      - name: Sign Firmware
        run: |
          imgtool sign --key keys/ec256-key.pem \
                       build/app.bin build/app-signed.bin

      - name: Upload Artifacts
        uses: actions/upload-artifact@v2
        with:
          name: firmware
          path: build/app-signed.bin
```

---

## Logging Best Practices

### Development

```c
#ifdef DEBUG
    #define LOG_DEBUG(fmt, ...) \
        printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
    #define LOG_DEBUG(fmt, ...) /* Nothing */
#endif

/* Verbose logging in development */
LOG_DEBUG("Key ID: %lu, Size: %zu", key_id, key_size);
```

### Production

```c
/* Minimal logging, no secrets */
LOG_INFO("Crypto operation completed");
/* Never log: keys, passwords, tokens, etc. */
```

---

## Error Handling Strategy

```c
/* Define error codes */
typedef enum {
    ERR_OK = 0,
    ERR_INVALID_PARAM = -1,
    ERR_CRYPTO_FAIL = -2,
    ERR_STORAGE_FULL = -3,
    ERR_AUTH_FAIL = -4
} error_code_t;

/* Centralized error handling */
void handle_error(error_code_t err)
{
    switch (err) {
        case ERR_CRYPTO_FAIL:
            log_error("Crypto failure");
            increment_fault_counter();
            break;
        case ERR_AUTH_FAIL:
            log_security_event("Authentication failed");
            trigger_tamper_response();
            break;
    }
}
```

---

## Watchdog Configuration

```c
/* Independent Watchdog (IWDG) */
void watchdog_init(void)
{
    /* 4-second timeout */
    IWDG->KR = 0x5555;  /* Enable write */
    IWDG->PR = IWDG_PR_PR_4;  /* Prescaler /256 */
    IWDG->RLR = 4000;  /* Reload value */
    IWDG->KR = 0xCCCC;  /* Start */
}

void main_loop(void)
{
    while (1) {
        process_tasks();

        /* Kick watchdog every iteration */
        IWDG->KR = 0xAAAA;

        HAL_Delay(100);
    }
}
```

**Prevents firmware hangs**

---

## Secure Boot Verification Test

```c
void test_secure_boot(void)
{
    printf("Testing secure boot...\n");

    /* 1. Create invalid image (wrong signature) */
    uint8_t fake_fw[1024] = { /* ... */ };
    flash_write(SECONDARY_SLOT, fake_fw, sizeof(fake_fw));

    boot_set_pending(0);

    /* 2. Reboot */
    NVIC_SystemReset();

    /* Expected: MCUboot rejects image, boots primary */
    /* This code should run after reboot */

    if (boot_verify_rejected()) {
        printf("✅ Secure boot test PASSED\n");
    } else {
        printf("❌ Secure boot test FAILED\n");
    }
}
```

---

## Product Lifecycle Management

```
┌─────────────┐
│   FACTORY   │ → Generate keys, provision secrets
└──────┬──────┘
       │
┌──────▼──────┐
│  TESTING    │ → RDP Level 1, debug enabled
└──────┬──────┘
       │
┌──────▼──────┐
│ PRODUCTION  │ → RDP Level 2, debug disabled
└──────┬──────┘
       │
┌──────▼──────┐
│    FIELD    │ → OTA updates, monitoring
└──────┬──────┘
       │
┌──────▼──────┐
│ RETIREMENT  │ → Secure decommissioning
└─────────────┘
```

---

## Secure Decommissioning

```c
void device_decommission(void)
{
    printf("Decommissioning device...\n");

    /* 1. Revoke cloud credentials */
    cloud_revoke_certificate(device_id);

    /* 2. Erase all storage */
    psa_its_remove(UID_DEVICE_KEY);
    psa_its_remove(UID_API_KEY);
    psa_ps_remove(UID_DEVICE_CERT);

    /* 3. Erase cryptographic keys */
    for (psa_key_id_t id = 1; id <= MAX_KEYS; id++) {
        psa_destroy_key(id);
    }

    /* 4. Mass erase flash */
    flash_mass_erase();

    /* 5. Disable device permanently */
    burn_efuse_decommissioned();

    printf("Device decommissioned\n");
    while (1);  /* Halt */
}
```

---

## Section Summary

### Skills Acquired

✓ Integrate TF-M with FreeRTOS/Zephyr
✓ Implement secure boot with MCUboot
✓ Deploy OTA firmware updates
✓ Configure debug authentication
✓ Set RDP protection levels
✓ Prepare devices for production
✓ Manage product lifecycle

---

<!-- _class: lead -->
<!-- _paginate: false -->

# Questions?

**Next:** Section 5 - Performance & Optimization

---

**End of Section 4**
*Total Slides: 45*
*Estimated Duration: 2 hours*

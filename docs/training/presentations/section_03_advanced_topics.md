---
marp: true
theme: default
paginate: true
backgroundColor: #ffffff
header: 'TF-M Training Package - Section 3'
footer: 'Advanced TF-M Topics | © 2025'
---

<!-- _class: lead -->
<!-- _paginate: false -->

# Advanced TF-M Topics

**Custom Services, IPC & Isolation**

![bg right:40%](https://via.placeholder.com/400x300/00AA00/ffffff?text=TF-M+Advanced)

*TF-M Training Package - Section 3*

---

## Section Overview

### Topics Covered

1. **Custom Secure Services** (Slides 1-15)
2. **Partition Design** (Slides 16-25)
3. **IPC Mechanism** (Slides 26-35)
4. **Isolation Levels** (Slides 36-45)
5. **Summary** (Slides 46-50)

---

## Why Custom Secure Services?

### Extending TF-M

**Built-in Services:**
- Crypto, Storage, Attestation (covered)

**Custom Needs:**
- Hardware-specific security (HSM, TPM)
- Industry-specific (payment, medical)
- Performance-critical operations
- Proprietary algorithms

**Solution:** Create Secure Partitions

---

## Secure Partition Architecture

```
┌─────────────────────────────────────┐
│    Non-Secure Application           │
├─────────────────────────────────────┤
│    PSA Client API                   │
│    psa_call(handle, ...)            │
├─────────────────────────────────────┤
│    Secure Partition Manager (SPM)   │
├───────┬───────┬───────┬─────────────┤
│Crypto │  ITS  │  PS   │ **CUSTOM**  │
│       │       │       │  Partition  │
└───────┴───────┴───────┴─────────────┘
```

---

## Partition Manifest (YAML)

```yaml
{
  "name": "TFM_SP_SECURE_CALCULATOR",
  "type": "APPLICATION-ROT",
  "priority": "NORMAL",
  "model": "IPC",
  "entry_point": "secure_calculator_main",
  "stack_size": "0x800",
  "services": [
    {
      "name": "TFM_SECURE_CALCULATOR",
      "sid": "0x00000100",
      "version": 1,
      "non_secure_clients": true,
      "version_policy": "STRICT"
    }
  ],
  "mmio_regions": [],
  "irqs": []
}
```

---

## Service Implementation

```c
/* secure_calculator.c */
#include "psa/service.h"
#include "tfm_secure_calculator.h"

void secure_calculator_main(void)
{
    psa_signal_t signals;

    while (1) {
        signals = psa_wait(PSA_WAIT_ANY, PSA_BLOCK);

        if (signals & TFM_SECURE_CALC_SIGNAL) {
            psa_msg_t msg;
            psa_get(TFM_SECURE_CALC_SIGNAL, &msg);

            /* Process request */
            handle_calc_request(&msg);

            psa_reply(msg.handle, PSA_SUCCESS);
        }
    }
}
```

---

## Client API Usage

```c
/* Non-Secure client code */
#include "psa/client.h"
#include "tfm_secure_calculator.h"

int32_t call_secure_calculator(int32_t a, int32_t b, char op)
{
    psa_handle_t handle;
    psa_status_t status;

    /* Connect to service */
    handle = psa_connect(TFM_SECURE_CALC_SID,
                         TFM_SECURE_CALC_VERSION);

    /* Prepare request */
    calc_request_t req = {.a = a, .b = b, .op = op};
    calc_response_t resp;

    psa_invec in_vec = {&req, sizeof(req)};
    psa_outvec out_vec = {&resp, sizeof(resp)};

    /* Call service */
    status = psa_call(handle, 0, &in_vec, 1, &out_vec, 1);

    psa_close(handle);
    return (status == PSA_SUCCESS) ? resp.result : -1;
}
```

---

## Inter-Partition Communication (IPC)

### Message Passing

```
NS Client                     Secure Partition
    │                              │
    │──psa_connect()──────────────►│
    │                          [allocate]
    │◄─────handle──────────────────│
    │                              │
    │──psa_call(handle)───────────►│
    │    (request data)        [process]
    │                              │
    │◄─────response────────────────│
    │                              │
    │──psa_close(handle)──────────►│
    │                          [cleanup]
```

---

## Memory Isolation

### Partition Private Memory

```c
/* Each partition has isolated memory */
static uint8_t partition_private_data[1024];
static crypto_key_t secret_keys[16];

/* Other partitions CANNOT access these */
```

**Memory Protection:**
- MPU enforces boundaries
- Faults on unauthorized access
- Prevents data leakage

---

## Service Signal Handling

```c
#define MY_SERVICE_SIGNAL  (1U << 0)
#define TIMER_SIGNAL       (1U << 1)

void partition_main(void)
{
    while (1) {
        psa_signal_t signals = psa_wait(PSA_WAIT_ANY, PSA_BLOCK);

        if (signals & MY_SERVICE_SIGNAL) {
            handle_service_request();
        }

        if (signals & TIMER_SIGNAL) {
            psa_eoi(TIMER_IRQ);
            handle_timer();
        }
    }
}
```

---

## Lab 06: MCUboot Firmware Update

### Secure Firmware Update Flow

```
┌──────────────┐
│ Download FW  │
│   (HTTPS)    │
└──────┬───────┘
       │
┌──────▼────────┐
│ Verify Sig    │ ◄─── Public Key (Secure)
│ (MCUboot)     │
└──────┬────────┘
       │
┌──────▼────────┐
│ Swap Slots    │
│ (A ↔ B)       │
└──────┬────────┘
       │
┌──────▼────────┐
│  Reboot       │
└───────────────┘
```

---

## MCUboot Image Signing

```bash
# Sign firmware image
imgtool sign \
  --key ecdsa-p256-private-key.pem \
  --align 4 \
  --version 1.2.3 \
  --header-size 0x400 \
  --slot-size 0x80000 \
  --pad-header \
  app.bin \
  app-signed.bin

# Output: app-signed.bin (with header + signature)
```

**MCUboot verifies signature before booting**

---

## Image Slot Management

```
Flash Layout:
├── 0x08000000: MCUboot Bootloader (64 KB)
├── 0x08010000: Slot 0 PRIMARY (512 KB)
├── 0x08090000: Slot 1 SECONDARY (512 KB)
└── 0x08110000: Scratch Area (64 KB)
```

**Update Process:**
1. Download new firmware to Slot 1
2. Mark for swap
3. Reboot → MCUboot swaps Slot 0 ↔ Slot 1
4. Boot new firmware

---

## Rollback Protection

```c
/* Security counter in write-once storage */
#define UID_FW_VERSION  0x00000010

bool check_firmware_version(uint32_t new_version)
{
    uint32_t current_version;
    psa_its_get(UID_FW_VERSION, 0, 4, &current_version, NULL);

    if (new_version <= current_version) {
        printf("Rollback attack detected!\n");
        return false;
    }

    /* Update version counter */
    psa_its_set(UID_FW_VERSION, 4, &new_version, 0);
    return true;
}
```

---

## Lab 07: Secure Boot Measurements

### Measured Boot Chain

```
ROM ──measure──► MCUboot ──measure──► TF-M ──measure──► App
 │                 │                     │                 │
 ▼                 ▼                     ▼                 ▼
SHA256           SHA256                SHA256           SHA256
 │                 │                     │                 │
 └─────────────────┴─────────────────────┴─────────────────┘
                           │
                           ▼
                  Attestation Token
```

**Each stage measures the next before executing**

---

## Boot Measurement Code

```c
int boot_measurement_add(uint8_t slot_id,
                          const uint8_t *data,
                          size_t len,
                          const char *version,
                          const uint8_t *signer_id)
{
    /* Compute SHA-256 hash */
    uint8_t hash[32];
    psa_hash_compute(PSA_ALG_SHA_256,
                     data, len,
                     hash, sizeof(hash),
                     NULL);

    /* Store in measurement log */
    store_measurement(slot_id, hash, version, signer_id);

    /* Include in attestation token */
    return 0;
}
```

---

## Lab 08: Advanced Protected Storage

### Encrypted Storage with Rollback Protection

```c
/* Store critical config */
#define UID_CONFIG  0x00000020

config_t config = {
    .version = 2,
    .api_key = "secret123",
    .enabled = true
};

/* Write-once + encrypted */
psa_ps_set(UID_CONFIG,
           sizeof(config),
           &config,
           PSA_STORAGE_FLAG_WRITE_ONCE);
```

**Features:**
- AES-256-GCM encryption
- Immutable once written
- Authenticated with HMAC

---

## Isolation Level 1

### Basic Isolation

**Features:**
- PSA Root of Trust (PRoT) isolated from Application RoT (ARoT)
- No isolation between ARoT partitions
- Minimal memory overhead

**Use Case:** Simple devices, limited RAM

```
┌───────────────┐
│    PRoT       │ ◄─── Isolated
├───────────────┤
│  ARoT (all)   │ ◄─── Shared
└───────────────┘
```

---

## Isolation Level 2

### Partition Isolation

**Features:**
- Each partition isolated via MPU
- Private stack and data
- IPC message passing

**Use Case:** Most IoT devices

```
┌───────┬───────┬───────┐
│ PRoT  │ ARoT1 │ ARoT2 │ ◄─── All isolated
└───────┴───────┴───────┘
```

---

## Isolation Level 3

### Full Isolation

**Features:**
- Maximum isolation
- Each partition = separate execution context
- Hardware-enforced boundaries

**Use Case:** High-security applications (payment, medical)

**Performance:** Higher overhead, maximum security

---

## Performance Considerations

### IPC vs Library Mode

| Aspect | IPC Mode | Library Mode |
|--------|----------|--------------|
| **Security** | High | Medium |
| **Latency** | ~50 µs | ~5 µs |
| **Memory** | Higher | Lower |
| **Isolation** | Strong | Weak |

**Recommendation:** Use IPC for production

---

## Optimizing Partition Size

```c
/* Reduce stack size if possible */
"stack_size": "0x400"  /* 1 KB instead of 2 KB */

/* Minimize dependencies */
/* Avoid large static arrays */

/* Use heap sparingly */
/* Pre-allocate buffers */
```

**Goal:** Fit more partitions in limited SRAM

---

## Section Summary

### Skills Acquired

✓ Create custom Secure Partitions
✓ Implement PSA IPC services
✓ Use partition manifests
✓ Understand isolation levels
✓ Implement firmware update with MCUboot
✓ Measure boot chain
✓ Optimize partition performance

---

<!-- _class: lead -->
<!-- _paginate: false -->

# Questions?

**Next:** Section 4 - Integration & Deployment

---

**End of Section 3**
*Total Slides: 50*
*Estimated Duration: 2 hours*

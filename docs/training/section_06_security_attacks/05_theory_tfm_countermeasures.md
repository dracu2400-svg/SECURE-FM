# Section 6: Security & Attack Resistance

## Module 05: TF-M Specific Countermeasures

**Learning Objectives:**
- Understand TF-M's security architecture for attack resistance
- Learn how TrustZone provides isolation
- Configure TF-M security features
- Implement runtime integrity checks
- Apply PSA Certified requirements
- Design secure applications on TF-M

---

## 📋 Table of Contents

1. [TF-M Security Architecture](#1-tfm-security-architecture)
2. [TrustZone Isolation](#2-trustzone-isolation)
3. [Secure Boot Chain](#3-secure-boot-chain)
4. [Runtime Protections](#4-runtime-protections)
5. [PSA Certified Requirements](#5-psa-certified-requirements)
6. [Application Security Guidelines](#6-application-security-guidelines)
7. [Complete Security Configuration](#7-complete-security-configuration)
8. [Security Verification](#8-security-verification)

---

## 1. TF-M Security Architecture

### 1.1 Defense in Depth

**TF-M Multi-Layer Security:**

```
Layer 7: Application Security
         ├─ Input validation
         ├─ Secure coding practices
         └─ Constant-time algorithms
              ↓
Layer 6: PSA API Security
         ├─ Caller authentication
         ├─ Parameter validation
         └─ Access control
              ↓
Layer 5: Secure Partition Manager (SPM)
         ├─ IPC message validation
         ├─ Partition isolation
         └─ Handle management
              ↓
Layer 4: TrustZone Isolation
         ├─ Secure/Non-Secure separation
         ├─ SAU configuration
         └─ MPU enforcement
              ↓
Layer 3: Secure Boot (MCUboot)
         ├─ Image signature verification
         ├─ Rollback protection
         └─ Authenticated firmware
              ↓
Layer 2: Hardware Security
         ├─ RDP protection
         ├─ Tamper detection
         └─ Crypto accelerators
              ↓
Layer 1: Physical Security
         ├─ Enclosure
         ├─ Mesh detection
         └─ Active shields
```

### 1.2 Attack Surface Minimization

**TF-M Attack Surface:**

```
Non-Secure World (Large Attack Surface)
┌────────────────────────────────────────┐
│  Application Code                      │
│  - Network stack                       │
│  - File system                         │
│  - User interfaces                     │
│  - ~100,000 lines of code             │
│                                        │
│  Attack Surface: LARGE                 │
└────────────┬───────────────────────────┘
             │ PSA API Calls Only
             ↓
Secure World (Minimal Attack Surface)
┌────────────────────────────────────────┐
│  TF-M SPM + Secure Partitions          │
│  - Well-defined PSA APIs (~50 funcs)   │
│  - Validated inputs                    │
│  - ~20,000 lines of code               │
│                                        │
│  Attack Surface: MINIMAL               │
└────────────────────────────────────────┘
```

**Key Benefit:** Even if Non-Secure world compromised, Secure assets remain protected.

### 1.3 Isolation Levels

**TF-M Isolation Levels:**

| Level | Description | MPU | Overhead | Security |
|-------|-------------|-----|----------|----------|
| **1** | No isolation between partitions | No | 0% | Low |
| **2** | PSA Level 2 - SPM isolation | Yes | ~10% | Medium |
| **3** | PSA Level 3 - Full isolation | Yes | ~20% | High |

**Recommendation for Security:** Use Level 2 or 3

```c
// CMake configuration
cmake .. \
    -DTFM_ISOLATION_LEVEL=2 \  // PSA Level 2
    ...
```

---

## 2. TrustZone Isolation

### 2.1 Security Attribution Unit (SAU)

**SAU Configuration (TF-M Handles This):**

```c
/**
 * TF-M configures SAU regions to separate Secure/Non-Secure
 * This is done automatically by TF-M platform code
 */

// Example SAU regions for STM32U545
typedef struct {
    uint32_t start;
    uint32_t limit;
    bool secure;
    bool nsc;  // Non-Secure Callable
} sau_region_t;

const sau_region_t sau_config[] = {
    // Region 0: Secure Code
    {
        .start = 0x0800A000,      // After BL2
        .limit = 0x0803BFFF,      // 200 KB
        .secure = true,
        .nsc = false
    },

    // Region 1: Non-Secure Code
    {
        .start = 0x0803C000,
        .limit = 0x0807BFFF,      // 256 KB
        .secure = false,
        .nsc = false
    },

    // Region 2: NSC (Veneer table for PSA APIs)
    {
        .start = 0x0803BF00,
        .limit = 0x0803BFFF,      // 256 bytes
        .secure = true,
        .nsc = true               // Non-Secure can call
    },

    // Region 3: Secure RAM
    {
        .start = 0x20000000,
        .limit = 0x2001FFFF,      // 128 KB
        .secure = true,
        .nsc = false
    },

    // Region 4: Non-Secure RAM
    {
        .start = 0x20020000,
        .limit = 0x2003FFFF,      // 128 KB
        .secure = false,
        .nsc = false
    }
};
```

**Protection Provided:**
- Non-Secure **cannot** access Secure memory
- Attempts trigger SecureFault exception
- SPM handles fault → System reset

### 2.2 Memory Protection Unit (MPU)

**MPU for Partition Isolation (Isolation Level 2/3):**

```c
/**
 * TF-M configures MPU to isolate secure partitions
 */

// Example: Crypto partition MPU configuration
void configure_crypto_partition_mpu(void)
{
    MPU_Region_InitTypeDef MPU_Config;

    // Region 0: Crypto partition code (RX only)
    MPU_Config.Number = MPU_REGION_NUMBER0;
    MPU_Config.BaseAddress = CRYPTO_PARTITION_CODE_START;
    MPU_Config.Size = MPU_REGION_SIZE_16KB;
    MPU_Config.AccessPermission = MPU_REGION_PRIV_RO;
    MPU_Config.IsExecute = MPU_INSTRUCTION_ACCESS_ENABLE;
    HAL_MPU_ConfigRegion(&MPU_Config);

    // Region 1: Crypto partition data (RW, no execute)
    MPU_Config.Number = MPU_REGION_NUMBER1;
    MPU_Config.BaseAddress = CRYPTO_PARTITION_DATA_START;
    MPU_Config.Size = MPU_REGION_SIZE_4KB;
    MPU_Config.AccessPermission = MPU_REGION_PRIV_RW;
    MPU_Config.IsExecute = MPU_INSTRUCTION_ACCESS_DISABLE;  // No exec!
    HAL_MPU_ConfigRegion(&MPU_Config);

    // Enable MPU
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}
```

**Result:** If crypto partition compromised, cannot:
- Access other partition's memory
- Execute code from data sections
- Call other partition functions directly

### 2.3 Secure Interrupt Handling

**NVIC Configuration:**

```c
/**
 * TF-M configures interrupts to prevent NS from hijacking Secure interrupts
 */

// Secure interrupts
void configure_secure_interrupts(void)
{
    // Crypto accelerator interrupt → Secure
    NVIC_SetTargetState(AES_IRQn, NVIC_SECURE);
    NVIC_SetPriority(AES_IRQn, 2);

    // RNG interrupt → Secure
    NVIC_SetTargetState(RNG_IRQn, NVIC_SECURE);
    NVIC_SetPriority(RNG_IRQn, 2);

    // Tamper interrupt → Secure
    NVIC_SetTargetState(TAMP_STAMP_IRQn, NVIC_SECURE);
    NVIC_SetPriority(TAMP_STAMP_IRQn, 0);  // Highest priority!
}
```

**Protection:** Non-Secure cannot:
- Trigger Secure interrupts
- Modify Secure interrupt priorities
- Register handlers for Secure interrupts

---

## 3. Secure Boot Chain

### 3.1 MCUboot Integration

**TF-M Boot Flow:**

```
Power-On Reset
     ↓
┌────────────────────────────────────────┐
│ ROM Bootloader (Immutable)             │
│ - Verify BL2 signature                 │
│ - Check RDP level                      │
│ - Jump to BL2                          │
└─────────────────┬──────────────────────┘
                  ↓
┌────────────────────────────────────────┐
│ BL2 (MCUboot)                          │
│ - Verify TF-M Secure signature         │
│ - Verify NS application signature      │
│ - Check security counter (rollback)    │
│ - Configure TrustZone (SAU/MPU)        │
│ - Jump to TF-M Secure                  │
└─────────────────┬──────────────────────┘
                  ↓
┌────────────────────────────────────────┐
│ TF-M Secure (SPM + Partitions)         │
│ - Initialize secure partitions         │
│ - Setup IPC                            │
│ - Configure security peripherals       │
│ - Jump to NS application               │
└─────────────────┬──────────────────────┘
                  ↓
┌────────────────────────────────────────┐
│ Non-Secure Application                 │
│ - Application code runs                │
│ - Calls PSA APIs for secure services   │
└────────────────────────────────────────┘
```

### 3.2 Image Signature Verification

**MCUboot Verification (ECDSA P-256):**

```c
/**
 * Simplified MCUboot image verification
 * (actual code is in MCUboot repository)
 */

#define IMAGE_MAGIC 0x96f3b83d

typedef struct {
    uint32_t magic;           // IMAGE_MAGIC
    uint32_t load_addr;       // Load address
    uint16_t header_size;     // Header size
    uint16_t protected_tlv_size;
    uint32_t img_size;        // Image size
    uint32_t flags;
    struct {
        uint8_t major;
        uint8_t minor;
        uint16_t revision;
        uint32_t build_num;
    } version;
    uint32_t _pad1;
} image_header_t;

typedef enum {
    IMAGE_TLV_SHA256 = 0x10,      // SHA256 hash
    IMAGE_TLV_ECDSA256 = 0x22,    // ECDSA-P256 signature
    IMAGE_TLV_SEC_CNT = 0x50      // Security counter
} image_tlv_type_t;

int boot_verify_image(int image_id)
{
    image_header_t *hdr = get_image_header(image_id);

    // 1. Check magic
    if (hdr->magic != IMAGE_MAGIC) {
        return -1;
    }

    // 2. Compute SHA-256 hash of image
    uint8_t hash[32];
    sha256(hdr + 1, hdr->img_size, hash);

    // 3. Find signature TLV
    uint8_t *signature = find_tlv(hdr, IMAGE_TLV_ECDSA256);
    if (!signature) {
        return -1;
    }

    // 4. Verify ECDSA signature
    psa_key_id_t public_key = get_root_public_key();

    psa_status_t status = psa_verify_hash(
        public_key,
        PSA_ALG_ECDSA(PSA_ALG_SHA_256),
        hash,
        32,
        signature,
        64
    );

    if (status != PSA_SUCCESS) {
        return -1;  // Invalid signature!
    }

    // 5. Check security counter (anti-rollback)
    uint32_t *sec_counter_tlv = find_tlv(hdr, IMAGE_TLV_SEC_CNT);
    uint32_t stored_counter = read_nv_counter(image_id);

    if (*sec_counter_tlv < stored_counter) {
        return -1;  // Rollback attack detected!
    }

    // 6. All checks passed
    return 0;
}
```

### 3.3 Rollback Protection

**Non-Volatile Counter:**

```c
/**
 * TF-M rollback protection
 * Uses monotonic counters in protected flash
 */

#define NV_COUNTER_ADDR 0x080FE000  // In protected region

typedef struct {
    uint32_t bl2_counter;          // BL2 version counter
    uint32_t s_counter;            // Secure firmware counter
    uint32_t ns_counter;           // Non-Secure firmware counter
    uint32_t checksum;             // Integrity check
} nv_counters_t;

/**
 * Increment security counter (one-way operation)
 */
int increment_nv_counter(uint32_t counter_id)
{
    nv_counters_t counters;

    // Read current values
    memcpy(&counters, (void *)NV_COUNTER_ADDR, sizeof(counters));

    // Verify integrity
    uint32_t expected_checksum = crc32(&counters, sizeof(counters) - 4);
    if (counters.checksum != expected_checksum) {
        // Corruption detected
        return -1;
    }

    // Increment counter (cannot decrease!)
    switch (counter_id) {
    case 0: counters.bl2_counter++; break;
    case 1: counters.s_counter++; break;
    case 2: counters.ns_counter++; break;
    default: return -1;
    }

    // Update checksum
    counters.checksum = crc32(&counters, sizeof(counters) - 4);

    // Write back to flash
    HAL_FLASH_Unlock();
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                      NV_COUNTER_ADDR,
                      (uint64_t *)&counters);
    HAL_FLASH_Lock();

    return 0;
}
```

---

## 4. Runtime Protections

### 4.1 Stack Protection

**Stack Canaries:**

```c
/**
 * TF-M uses stack canaries to detect stack overflow
 * (Enabled with -fstack-protector compiler flag)
 */

// GCC inserts canary check automatically:
void secure_function(void)
{
    // GCC-inserted code (before function body):
    // uint32_t canary = __stack_chk_guard;

    // Function body
    uint8_t buffer[64];
    process_data(buffer);

    // GCC-inserted code (before return):
    // if (canary != __stack_chk_guard) {
    //     __stack_chk_fail();  // Stack smashed!
    // }
}

// Stack check fail handler
void __stack_chk_fail(void)
{
    // Stack overflow detected!
    tfm_core_panic();  // Reset system
}
```

**MPU Stack Guard:**

```c
/**
 * MPU-based stack overflow detection
 */
void configure_stack_guard(void *stack_base, size_t stack_size)
{
    MPU_Region_InitTypeDef MPU_Config;

    // Create guard region (no access) below stack
    MPU_Config.Number = MPU_REGION_NUMBER7;
    MPU_Config.BaseAddress = (uint32_t)stack_base - 32;  // 32-byte guard
    MPU_Config.Size = MPU_REGION_SIZE_32B;
    MPU_Config.AccessPermission = MPU_REGION_NO_ACCESS;  // Triggers fault
    MPU_Config.IsExecute = MPU_INSTRUCTION_ACCESS_DISABLE;

    HAL_MPU_ConfigRegion(&MPU_Config);
}

// Any access to guard region triggers MemManage fault
void MemManage_Handler(void)
{
    // Stack overflow detected
    tfm_core_panic();
}
```

### 4.2 Control Flow Integrity

**TF-M SPM Call Validation:**

```c
/**
 * TF-M validates all PSA API calls
 */

psa_status_t tfm_crypto_key_derivation_setup(
    psa_invec *in_vec,
    size_t in_len,
    psa_outvec *out_vec,
    size_t out_len)
{
    // 1. Validate caller is Non-Secure
    if (!tfm_is_caller_non_secure()) {
        return PSA_ERROR_PROGRAMMER_ERROR;
    }

    // 2. Validate input vectors
    if (in_len != 2 || out_len != 0) {
        return PSA_ERROR_PROGRAMMER_ERROR;
    }

    // 3. Validate pointers are in Non-Secure memory
    if (!tfm_memory_check(in_vec[0].base, in_vec[0].len, TFM_MEMORY_ACCESS_RO)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    // 4. Extract parameters
    psa_key_derivation_operation_t *operation;
    psa_algorithm_t alg;

    operation = (psa_key_derivation_operation_t *)in_vec[0].base;
    alg = *((psa_algorithm_t *)in_vec[1].base);

    // 5. Validate algorithm
    if (!PSA_ALG_IS_KEY_DERIVATION(alg)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    // 6. Perform operation (finally!)
    return psa_key_derivation_setup(operation, alg);
}
```

**Protection Provided:**
- Invalid callers rejected
- Memory access violations prevented
- Parameter validation
- Type checking

### 4.3 Secure Storage Integrity

**TF-M ITS/PS Protection:**

```c
/**
 * TF-M Internal Trusted Storage (ITS)
 * Protected against:
 * - Rollback attacks (monotonic counter)
 * - Tampering (AES-GCM authentication)
 * - Replay attacks (nonce)
 */

typedef struct {
    uint32_t magic;               // 0x54535049 ("IPST")
    uint32_t version;             // Format version
    uint32_t max_asset_size;      // Maximum size
    uint32_t num_assets;          // Number of assets
    uint8_t iv[12];               // GCM IV/nonce
    uint8_t tag[16];              // GCM authentication tag
} its_metadata_t;

typedef struct {
    psa_storage_uid_t uid;        // Unique identifier
    psa_storage_create_flags_t flags;
    uint32_t size;                // Data size
    uint32_t offset;              // Offset in flash
} its_asset_t;

/**
 * Write to ITS (protected)
 */
psa_status_t psa_its_set(psa_storage_uid_t uid,
                         size_t data_length,
                         const void *p_data,
                         psa_storage_create_flags_t create_flags)
{
    // 1. Derive encryption key from hardware UID
    psa_key_id_t storage_key;
    derive_storage_key(&storage_key);

    // 2. Generate random IV
    uint8_t iv[12];
    psa_generate_random(iv, sizeof(iv));

    // 3. Prepare AAD (Additional Authenticated Data)
    struct {
        psa_storage_uid_t uid;
        psa_storage_create_flags_t flags;
        uint32_t length;
    } aad = {uid, create_flags, data_length};

    // 4. Encrypt with AES-GCM
    uint8_t ciphertext[data_length + 16];  // +16 for tag
    size_t ciphertext_len;

    psa_status_t status = psa_aead_encrypt(
        storage_key,
        PSA_ALG_GCM,
        iv, sizeof(iv),
        (uint8_t *)&aad, sizeof(aad),
        p_data, data_length,
        ciphertext, sizeof(ciphertext),
        &ciphertext_len
    );

    if (status != PSA_SUCCESS) {
        return status;
    }

    // 5. Write to protected flash
    write_to_its_flash(uid, iv, ciphertext, ciphertext_len);

    // 6. Increment monotonic counter (rollback protection)
    increment_its_version();

    return PSA_SUCCESS;
}
```

**Result:** Even if flash dumped:
- Data is encrypted (AES-GCM)
- Cannot modify without detection (authentication tag)
- Cannot rollback to old version (monotonic counter)

---

## 5. PSA Certified Requirements

### 5.1 PSA Certified Levels

**Three Levels of Certification:**

| Level | Requirements | Use Cases |
|-------|--------------|-----------|
| **Level 1** | Software review | Development, low-risk |
| **Level 2** | + Functional testing | Most IoT devices |
| **Level 3** | + Security evaluation | High-security (payment, medical) |

**Level 2 Requirements (Most Common):**

```
1. Secure Boot
   ✓ Root of Trust in hardware
   ✓ Signature verification
   ✓ Rollback protection

2. Secure Storage
   ✓ Confidentiality (encryption)
   ✓ Integrity (authentication)
   ✓ Isolation from NS

3. Cryptography
   ✓ NIST-approved algorithms
   ✓ Secure key storage
   ✓ Side-channel resistance

4. Isolation
   ✓ TrustZone or equivalent
   ✓ Secure/Non-Secure separation
   ✓ Partition isolation (Level 2/3)

5. Attestation
   ✓ Device identity
   ✓ Firmware measurements
   ✓ Signed attestation tokens

6. Update
   ✓ Authenticated updates
   ✓ Rollback protection
   ✓ Secure delivery
```

### 5.2 TF-M PSA Certified Configuration

```cmake
# CMake configuration for PSA Certified Level 2

cmake .. \
    # Core
    -DTFM_PLATFORM=stm/nucleo_l552ze_q \
    -DTFM_TOOLCHAIN_FILE=../toolchain_GNUARM.cmake \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    \
    # Isolation (Level 2 required for PSA Cert)
    -DTFM_ISOLATION_LEVEL=2 \
    -DTFM_PROFILE=profile_medium \
    \
    # Secure Boot (required)
    -DBL2=ON \
    -DMCUBOOT_HW_KEY=ON \
    -DMCUBOOT_UPGRADE_STRATEGY=SWAP_USING_SCRATCH \
    \
    # Secure Services (required)
    -DTFM_PARTITION_CRYPTO=ON \
    -DTFM_PARTITION_INTERNAL_TRUSTED_STORAGE=ON \
    -DTFM_PARTITION_PROTECTED_STORAGE=ON \
    -DTFM_PARTITION_INITIAL_ATTESTATION=ON \
    -DTFM_PARTITION_PLATFORM=ON \
    \
    # Cryptography (use hardware accelerator)
    -DCRYPTO_HW_ACCELERATOR=ON \
    \
    # Security hardening
    -DTFM_DUMMY_PROVISIONING=OFF \    # Real provisioning for production!
    -DPLATFORM_DEFAULT_NV_COUNTERS=ON \
    -DPLATFORM_DEFAULT_ROTPK=ON \
    -DPLATFORM_DEFAULT_IAK=ON
```

### 5.3 Security Lifecycle

**PSA Certified Lifecycle States:**

```c
typedef enum {
    PSA_LIFECYCLE_UNKNOWN           = 0x0000,
    PSA_LIFECYCLE_ASSEMBLY          = 0x1000,  // Manufacturing
    PSA_LIFECYCLE_PSA_ROT_PROVISIONING = 0x2000,  // Provisioning
    PSA_LIFECYCLE_SECURED           = 0x3000,  // Normal operation
    PSA_LIFECYCLE_NON_PSA_ROT_DEBUG = 0x4000,  // Debug allowed
    PSA_LIFECYCLE_RECOVERABLE_PSA_ROT_DEBUG = 0x5000,
    PSA_LIFECYCLE_DECOMMISSIONED    = 0x6000   // End of life
} psa_lifecycle_t;

/**
 * Get current lifecycle state
 */
psa_lifecycle_t psa_security_lifecycle_state(void)
{
    // Read from OTP or option bytes
    uint32_t state = *(volatile uint32_t *)(OTP_LIFECYCLE_ADDR);

    return (psa_lifecycle_t)(state & 0xF000);
}

/**
 * Application behavior based on lifecycle
 */
void main_application(void)
{
    psa_lifecycle_t lifecycle = psa_security_lifecycle_state();

    switch (lifecycle) {
    case PSA_LIFECYCLE_ASSEMBLY:
        // Manufacturing mode
        run_factory_tests();
        provision_device();
        transition_to_secured();
        break;

    case PSA_LIFECYCLE_PSA_ROT_PROVISIONING:
        // Provisioning mode
        load_device_credentials();
        transition_to_secured();
        break;

    case PSA_LIFECYCLE_SECURED:
        // Normal operation
        run_secure_application();
        break;

    case PSA_LIFECYCLE_DECOMMISSIONED:
        // End of life - erase all data
        erase_all_data();
        infinite_sleep();
        break;

    default:
        // Invalid state
        tfm_core_panic();
    }
}
```

---

## 6. Application Security Guidelines

### 6.1 Secure Coding Practices for TF-M

**DO:**
```c
// ✓ Use PSA Crypto API (not custom crypto)
psa_hash_compute(PSA_ALG_SHA_256, data, len, hash, 32, &hash_len);

// ✓ Use constant-time comparisons
bool secrets_equal = constant_time_compare(secret1, secret2, 32);

// ✓ Validate all inputs from Non-Secure
if (len > MAX_BUFFER_SIZE) {
    return PSA_ERROR_INVALID_ARGUMENT;
}

// ✓ Clear sensitive data after use
memset(password, 0, sizeof(password));

// ✓ Check return values
psa_status_t status = psa_hash_compute(...);
if (status != PSA_SUCCESS) {
    // Handle error
}
```

**DON'T:**
```c
// ❌ Don't implement your own crypto
uint32_t my_sha256(uint8_t *data, size_t len);  // NO!

// ❌ Don't use secret-dependent branches
if (password[i] != stored[i]) {
    return false;  // Timing leak!
}

// ❌ Don't trust Non-Secure input without validation
memcpy(buffer, ns_data, ns_len);  // Buffer overflow!

// ❌ Don't leave secrets in memory
// password still in memory!

// ❌ Don't ignore errors
psa_hash_compute(...);  // Didn't check return value!
```

### 6.2 Input Validation

**Comprehensive Validation:**

```c
/**
 * Validate input from Non-Secure world
 */
psa_status_t validate_ns_input(const void *ptr, size_t len, bool writable)
{
    // 1. Check pointer is not NULL
    if (ptr == NULL) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    // 2. Check pointer is in Non-Secure memory
    if (!tfm_memory_is_nonsecure(ptr, len)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    // 3. Check length is reasonable
    if (len == 0 || len > MAX_ALLOWED_SIZE) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    // 4. Check memory is readable
    if (!tfm_memory_check(ptr, len, TFM_MEMORY_ACCESS_RO)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    // 5. If writable required, check write permission
    if (writable && !tfm_memory_check(ptr, len, TFM_MEMORY_ACCESS_RW)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    // 6. Check for integer overflow in size calculations
    if (len > SIZE_MAX / 2) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    return PSA_SUCCESS;
}
```

### 6.3 Error Handling

**Secure Error Handling:**

```c
/**
 * Example of secure error handling
 */
psa_status_t process_sensitive_data(const uint8_t *data, size_t len)
{
    psa_status_t status;
    uint8_t *temp_buffer = NULL;
    psa_key_id_t key_id = PSA_KEY_ID_NULL;

    // Allocate resources
    temp_buffer = malloc(len);
    if (temp_buffer == NULL) {
        status = PSA_ERROR_INSUFFICIENT_MEMORY;
        goto cleanup;
    }

    // Import key
    status = psa_import_key(&attr, key_data, key_len, &key_id);
    if (status != PSA_SUCCESS) {
        goto cleanup;  // Jump to cleanup on error
    }

    // Process data
    status = psa_cipher_encrypt(key_id, ...);
    if (status != PSA_SUCCESS) {
        goto cleanup;
    }

    // Success path
    status = PSA_SUCCESS;

cleanup:
    // ALWAYS clean up, even on error

    // Clear sensitive data
    if (temp_buffer != NULL) {
        memset(temp_buffer, 0, len);
        free(temp_buffer);
    }

    // Destroy key
    if (key_id != PSA_KEY_ID_NULL) {
        psa_destroy_key(key_id);
    }

    return status;
}
```

---

## 7. Complete Security Configuration

### 7.1 Production Security Checklist

```c
/**
 * Complete security configuration for production device
 */
void configure_production_security(void)
{
    // ══════════════════════════════════════════════════
    // 1. HARDWARE SECURITY
    // ══════════════════════════════════════════════════

    // Flash Read Protection Level 1 (can upgrade to 2 later)
    configure_flash_rdp(1);

    // Disable JTAG (keep SWD for emergency recovery)
    disable_jtag();

    // Configure tamper detection
    configure_tamper_pins();
    configure_voltage_monitoring();
    configure_clock_security();

    // ══════════════════════════════════════════════════
    // 2. TRUSTZONE CONFIGURATION
    // ══════════════════════════════════════════════════

    // SAU regions (done by TF-M platform code)
    // - Secure flash
    // - Non-Secure flash
    // - NSC region (veneers)
    // - Secure RAM
    // - Non-Secure RAM

    // MPU for partition isolation (done by TF-M SPM)

    // NVIC security (Secure interrupts)
    configure_secure_interrupts();

    // ══════════════════════════════════════════════════
    // 3. CRYPTOGRAPHIC KEYS
    // ══════════════════════════════════════════════════

    // Initialize device-unique keys
    init_device_encryption_key();

    // Load attestation keys from OTP
    load_attestation_key();

    // ══════════════════════════════════════════════════
    // 4. SECURE STORAGE
    // ══════════════════════════════════════════════════

    // Initialize ITS/PS
    tfm_its_init();
    tfm_ps_init();

    // Verify rollback counters
    verify_nv_counters();

    // ══════════════════════════════════════════════════
    // 5. RUNTIME PROTECTIONS
    // ══════════════════════════════════════════════════

    // Enable stack protection
    configure_stack_guards();

    // Enable watchdog
    configure_watchdog();

    // ══════════════════════════════════════════════════
    // 6. DEBUGGING & LOGGING
    // ══════════════════════════════════════════════════

    // Disable debug UART
    disable_debug_uart();

    // Configure secure logging (if needed)
    configure_secure_logging();

    // ══════════════════════════════════════════════════
    // 7. VERIFICATION
    // ══════════════════════════════════════════════════

    // Verify all configurations
    verify_security_configuration();

    // Set production flag
    set_production_mode_flag();
}
```

### 7.2 Runtime Security Monitoring

```c
/**
 * Periodic security health check
 * Run from TF-M secure partition or NS with PSA calls
 */
void security_health_check(void)
{
    static uint32_t check_counter = 0;

    // 1. Verify code integrity (hash of critical sections)
    if (!verify_code_integrity()) {
        handle_integrity_failure();
    }

    // 2. Check for tampering
    if (check_tamper_status()) {
        handle_tamper_event(TAMPER_EVENT_RUNTIME);
    }

    // 3. Verify stack canaries
    if (!verify_stack_canaries()) {
        handle_stack_corruption();
    }

    // 4. Check watchdog is running
    if (!is_watchdog_active()) {
        restart_watchdog();
    }

    // 5. Verify secure storage integrity
    if (!verify_its_integrity()) {
        handle_storage_corruption();
    }

    // 6. Increment check counter (detect fault injection loops)
    check_counter++;
    uint32_t check_counter_inv = ~check_counter;

    if ((check_counter + check_counter_inv) != ~0U) {
        // Counter corrupted by fault injection
        tfm_core_panic();
    }
}
```

---

## 8. Security Verification

### 8.1 Verification Checklist

**Pre-Deployment Verification:**

```markdown
## Hardware Security
- [ ] RDP Level 1 or 2 enabled
- [ ] JTAG disabled or protected
- [ ] Tamper detection configured
- [ ] PVD voltage monitoring enabled
- [ ] CSS clock security enabled
- [ ] Watchdog configured and tested

## TrustZone Configuration
- [ ] SAU regions properly configured
- [ ] MPU isolation enabled (Level 2/3)
- [ ] Secure interrupts configured
- [ ] Non-Secure cannot access Secure memory (verified)

## Secure Boot
- [ ] MCUboot signature verification works
- [ ] Rollback protection tested
- [ ] Security counters incrementing correctly
- [ ] Invalid images rejected

## Cryptography
- [ ] All keys in secure storage (ITS)
- [ ] No keys in Non-Secure flash/RAM
- [ ] PSA Crypto API used (not custom implementations)
- [ ] Constant-time algorithms for sensitive operations

## Secure Storage
- [ ] ITS encryption enabled
- [ ] PS encryption enabled
- [ ] Rollback protection tested
- [ ] Integrity checks working

## Attestation
- [ ] Device identity unique
- [ ] Attestation tokens signed correctly
- [ ] Server-side verification works

## Application Security
- [ ] All NS inputs validated
- [ ] No buffer overflows (static analysis)
- [ ] Error handling secure
- [ ] Sensitive data cleared after use
- [ ] No secrets in logs

## Physical Security
- [ ] Enclosure tamper-evident
- [ ] Debug connector removed/protected
- [ ] Mesh sensors (if applicable)
- [ ] Epoxy potting (if required)
```

### 8.2 Automated Security Testing

```python
#!/usr/bin/env python3
"""
Automated security test suite for TF-M device
"""

import serial
import time
import hashlib
import ecdsa

class TFMSecurityTests:
    def __init__(self, serial_port):
        self.ser = serial.Serial(serial_port, 115200, timeout=5)

    def test_secure_boot(self):
        """Test 1: Verify secure boot rejects unsigned firmware"""
        print("[TEST] Secure Boot - Invalid Signature")

        # Flash unsigned firmware
        self.flash_unsigned_firmware()

        # Reset device
        self.reset_device()

        # Check boot log
        boot_log = self.read_boot_log()

        if "signature verification failed" in boot_log:
            print("✓ PASS: Unsigned firmware rejected")
            return True
        else:
            print("✗ FAIL: Unsigned firmware accepted!")
            return False

    def test_rollback_protection(self):
        """Test 2: Verify rollback protection"""
        print("[TEST] Rollback Protection")

        # Flash version 2.0
        self.flash_firmware(version="2.0.0")
        time.sleep(1)

        # Try to flash version 1.0 (older)
        self.flash_firmware(version="1.0.0")

        # Reset device
        self.reset_device()

        # Check boot log
        boot_log = self.read_boot_log()

        if "rollback detected" in boot_log:
            print("✓ PASS: Rollback prevented")
            return True
        else:
            print("✗ FAIL: Rollback allowed!")
            return False

    def test_debug_port_locked(self):
        """Test 3: Verify debug port is locked"""
        print("[TEST] Debug Port Security")

        # Try to connect with debugger
        result = self.try_debug_connection()

        if result == "connection_refused":
            print("✓ PASS: Debug port locked")
            return True
        elif result == "rdp_level_1":
            print("✓ PASS: RDP Level 1 active")
            return True
        else:
            print("✗ FAIL: Debug port accessible!")
            return False

    def test_memory_isolation(self):
        """Test 4: Verify NS cannot access Secure memory"""
        print("[TEST] Memory Isolation")

        # Send command to try reading Secure memory from NS
        self.send_command("read_secure_memory 0x20000000 256")

        response = self.read_response()

        if "access violation" in response or "fault" in response:
            print("✓ PASS: Secure memory protected")
            return True
        else:
            print("✗ FAIL: Secure memory accessible!")
            return False

    def test_encrypted_storage(self):
        """Test 5: Verify storage is encrypted"""
        print("[TEST] Encrypted Storage")

        # Write known data
        test_data = b"SECRET_DATA_12345"
        self.send_command(f"its_set 1234 {test_data.hex()}")

        # Read flash directly (simulating flash dump)
        flash_contents = self.dump_flash_region(0x080F6000, 0x080FA000)

        # Check if plaintext is NOT present
        if test_data in flash_contents:
            print("✗ FAIL: Data stored in plaintext!")
            return False
        else:
            print("✓ PASS: Data encrypted in flash")
            return True

    def run_all_tests(self):
        """Run all security tests"""
        print("=" * 60)
        print("TF-M Security Test Suite")
        print("=" * 60)

        tests = [
            self.test_secure_boot,
            self.test_rollback_protection,
            self.test_debug_port_locked,
            self.test_memory_isolation,
            self.test_encrypted_storage
        ]

        results = []
        for test in tests:
            result = test()
            results.append(result)
            print()

        # Summary
        print("=" * 60)
        print(f"Results: {sum(results)}/{len(results)} tests passed")

        if all(results):
            print("✓ ALL TESTS PASSED")
        else:
            print("✗ SOME TESTS FAILED")

        print("=" * 60)

        return all(results)

# Run tests
if __name__ == "__main__":
    tester = TFMSecurityTests("/dev/ttyACM0")
    success = tester.run_all_tests()
    exit(0 if success else 1)
```

---

## 📊 Summary

### TF-M Security Features Summary

| Feature | Protection Against | Enabled By Default |
|---------|-------------------|-------------------|
| TrustZone (SAU/MPU) | Memory access violations | ✅ Yes |
| Secure Boot (MCUboot) | Malicious firmware | ✅ Yes |
| Rollback Protection | Downgrade attacks | ✅ Yes |
| Encrypted Storage (ITS/PS) | Flash readout | ✅ Yes |
| Stack Canaries | Buffer overflow | ⚠️ Compiler flag |
| Attestation | Device cloning | ✅ Yes |
| Secure Interrupts | Interrupt hijacking | ✅ Yes |
| Input Validation | Code injection | ⚠️ Application responsibility |

### Attack Resistance Matrix

| Attack Type | TF-M Protection | Additional Measures |
|-------------|----------------|---------------------|
| **Software Exploits** | Isolation, MPU | Secure coding, fuzzing |
| **Privilege Escalation** | TrustZone, SPM | Input validation |
| **Side-Channel (Power)** | Hardware crypto | Masking, noise |
| **Side-Channel (Timing)** | Constant-time | Application design |
| **Fault Injection** | Redundant checks | PVD, CSS, watchdog |
| **Debug Port** | Authentication | RDP, lockdown |
| **Flash Readout** | Encryption | RDP Level 2 |
| **Tampering** | Attestation | Tamper sensors |
| **Rollback** | Monotonic counters | Secure update |
| **Supply Chain** | Provisioning | Certificates |

### Best Practices Summary

**Development:**
1. Use TF-M Isolation Level 2 or 3
2. Enable all TF-M security partitions
3. Use PSA Crypto API (never custom crypto)
4. Validate all inputs from Non-Secure
5. Use constant-time algorithms
6. Clear secrets after use
7. Enable compiler security flags (`-fstack-protector`, `-D_FORTIFY_SOURCE=2`)

**Production:**
1. Set RDP Level 1 or 2
2. Disable/protect debug ports
3. Enable tamper detection
4. Configure voltage/clock monitoring
5. Provision unique device credentials
6. Erase provisioning/debug data
7. Verify security configuration

**Deployment:**
1. Use secure boot chain
2. Sign all firmware images
3. Increment security counters
4. Use encrypted updates
5. Implement attestation
6. Monitor for tampering
7. Regular security updates

---

## 📚 References

1. **PSA Certified Documentation:** https://www.psacertified.org/
2. **TF-M Security Incident Management:** https://tf-m-user-guide.trustedfirmware.org/
3. **ARM Platform Security Architecture:** https://developer.arm.com/architectures/security-architectures/platform-security-architecture
4. **NIST Cybersecurity Framework**
5. **OWASP IoT Top 10:** https://owasp.org/www-project-internet-of-things/
6. **Common Criteria Protection Profiles**
7. **ISO/IEC 15408** - Security evaluation criteria

---

**End of Module 05 - Section 6 Complete!**

**Next: Lab 26-30** - Practical implementation of security countermeasures

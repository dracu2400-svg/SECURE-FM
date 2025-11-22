---
marp: true
theme: default
paginate: true
backgroundColor: #ffffff
header: 'TF-M Training Package - Section 6'
footer: 'Security & Attack Mitigations | © 2025'
---

<!-- _class: lead -->
<!-- _paginate: false -->

# Security & Attack Mitigations

**Threat Modeling & Defense Strategies**

![bg right:40%](https://via.placeholder.com/400x300/CC0000/ffffff?text=Security)

*TF-M Training Package - Section 6*

---

## Section Overview

### Topics

1. **Threat Modeling** (STRIDE, TARA)
2. **Software Attacks**
3. **Hardware Attacks**
4. **Side-Channel Attacks**
5. **Fault Injection**
6. **Physical Security**
7. **Countermeasures**
8. **Secure Coding Practices**

---

## STRIDE Threat Model

### Six Threat Categories

| Threat | Description | Example |
|--------|-------------|---------|
| **S**poofing | Impersonation | Fake device ID |
| **T**ampering | Unauthorized modification | Flash reprogramming |
| **R**epudiation | Deny actions | No audit logs |
| **I**nformation Disclosure | Data leakage | Debug port access |
| **D**enial of Service | Availability loss | Watchdog disable |
| **E**levation of Privilege | Unauthorized access | NS→S exploit |

---

## TARA (Threat Analysis & Risk Assessment)

### Automotive (ISO/SAE 21434)

```
1. Asset Identification
   └─> Cryptographic keys, firmware, user data

2. Threat Scenarios
   └─> Flash dump, debugger attach, supply chain

3. Impact Rating
   └─> Safety: Critical | Financial: High | Privacy: Medium

4. Attack Feasibility
   └─> Skill: Expert | Resources: $10K | Time: 1 week

5. Risk Level
   └─> CVSS Score: 8.5 (High)

6. Mitigation
   └─> RDP Level 2, encrypted storage, attestation
```

---

## Attack Surface Analysis

### Entry Points

**Physical:**
- JTAG/SWD debug port
- UART debug console
- SPI flash chip
- Power/reset pins

**Logical:**
- Network stack (TCP/IP, BLE, LoRa)
- Firmware update mechanism
- API endpoints
- IPC messages

**Goal:** Minimize and harden each entry point

---

## Buffer Overflow Attack

### Classic Vulnerability

```c
/* ❌ VULNERABLE CODE */
void process_command(const char *cmd)
{
    char buffer[64];
    strcpy(buffer, cmd);  /* No bounds check! */
    execute(buffer);
}

/* Attack: Send 100-byte string → overwrite return address */
char exploit[100] = { /* shellcode */ };
process_command(exploit);  /* Code execution! */
```

---

## Buffer Overflow Mitigation

### Stack Canaries

```c
/* Compiler inserts canary between locals and return address */
void protected_function(const char *input)
{
    uint32_t __stack_chk_guard = CANARY_VALUE;
    char buffer[64];

    /* Process data */
    safe_strcpy(buffer, input, sizeof(buffer));

    /* Check canary before return */
    if (__stack_chk_guard != CANARY_VALUE) {
        __stack_chk_fail();  /* Stack overflow detected! */
    }
}
```

**Enable:** `-fstack-protector-all` compiler flag

---

## Safe String Functions

```c
/* ✅ SAFE CODE */
void process_command_safe(const char *cmd, size_t cmd_len)
{
    char buffer[64];

    /* Bounds-checked copy */
    if (cmd_len >= sizeof(buffer)) {
        return;  /* Input too large */
    }

    strncpy(buffer, cmd, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';  /* Ensure null termination */

    execute(buffer);
}
```

**Never use:** `strcpy`, `sprintf`, `gets`
**Always use:** `strncpy`, `snprintf`, `fgets`

---

## Integer Overflow Attack

```c
/* ❌ VULNERABLE */
void allocate_buffer(uint32_t count, uint32_t size)
{
    uint32_t total = count * size;  /* Overflow! */
    uint8_t *buffer = malloc(total);
    /* If count=0x10000000, size=16 → total=0 (overflow) */
    /* malloc(0) succeeds, but buffer is tiny! */
}

/* ✅ SAFE */
void allocate_buffer_safe(uint32_t count, uint32_t size)
{
    if (count > SIZE_MAX / size) {
        return;  /* Overflow would occur */
    }

    uint32_t total = count * size;
    uint8_t *buffer = malloc(total);
}
```

---

## Format String Vulnerability

```c
/* ❌ VULNERABLE */
void log_message(const char *user_input)
{
    printf(user_input);  /* DANGER! */
    /* Attack: user_input = "%x %x %x %x" → leaks stack */
    /* Attack: user_input = "%n" → writes to memory */
}

/* ✅ SAFE */
void log_message_safe(const char *user_input)
{
    printf("%s", user_input);  /* Format string is literal */
}
```

---

## Pointer Validation (TrustZone)

```c
__attribute__((cmse_nonsecure_entry))
int secure_process_data(uint8_t *ns_buffer, size_t len)
{
    /* ❌ VULNERABLE: Trust NS pointer */
    uint8_t first_byte = *ns_buffer;  /* Crash or exploit! */

    /* ✅ SAFE: Validate pointer */
    uint8_t *checked = cmse_check_address_range(
        ns_buffer, len, CMSE_NONSECURE | CMSE_MPU_READ);

    if (checked == NULL) {
        return -1;  /* Invalid pointer attack */
    }

    /* Safe to use */
    process_data(checked, len);
    return 0;
}
```

---

## TOCTOU (Time-of-Check to Time-of-Use)

```c
/* ❌ VULNERABLE */
__attribute__((cmse_nonsecure_entry))
int toctou_vuln(uint32_t *ns_value)
{
    /* Check */
    if (cmse_check_address_range(ns_value, 4, CMSE_NONSECURE)) {
        uint32_t val = *ns_value;  /* Read 1 */

        /* NS attacker changes *ns_value here! */

        if (val < 100) {
            use_value(*ns_value);  /* Read 2 - different value! */
        }
    }
}

/* ✅ SAFE: Copy to secure memory first */
int toctou_safe(uint32_t *ns_value)
{
    uint32_t val;
    memcpy(&val, ns_value, sizeof(val));  /* Single read */

    if (val < 100) {
        use_value(val);  /* Use copy */
    }
}
```

---

## Hardware Attacks: JTAG/SWD

### Debug Port Exploitation

**Attack:**
1. Connect debugger to SWD pins
2. Halt CPU
3. Read flash memory
4. Extract keys, firmware, secrets

**Mitigation:**
```c
/* Disable debug in production */
void secure_debug_interface(void)
{
    /* Set RDP Level 2 (PERMANENT) */
    FLASH->OPTR |= FLASH_OPTR_RDP_LEVEL_2;

    /* Disable JTAG/SWD pins */
    __HAL_AFIO_REMAP_SWJ_DISABLE();
}
```

---

## Hardware Attacks: Flash Readout

### Physical Memory Extraction

**Attack:**
1. Desolder flash chip
2. Read with programmer
3. Reverse engineer firmware

**Mitigation:**
- **Encrypted firmware** (MCUboot)
- **Internal flash only** (no external chips)
- **RDP Level 2** (flash read protection)
- **Secure boot** (signature verification)

---

## Side-Channel Attacks: Power Analysis

### Simple Power Analysis (SPA)

```
Power consumption reveals operations:

High power spike = AES encryption
Low power = idle
Medium power = XOR operation

Attacker measures power → infers key bits
```

**Mitigation:**
- Hardware crypto engine (constant power)
- Randomized execution order
- Dummy operations (power noise)

---

## Side-Channel Attacks: Timing

```c
/* ❌ VULNERABLE: Timing leak */
bool check_password(const char *input, const char *correct)
{
    for (int i = 0; input[i] != '\0'; i++) {
        if (input[i] != correct[i]) {
            return false;  /* Early exit! */
        }
    }
    return true;
}
/* Attacker measures time → knows how many chars are correct */

/* ✅ SAFE: Constant-time comparison */
bool check_password_safe(const char *input, const char *correct)
{
    volatile uint8_t diff = 0;
    for (int i = 0; i < MAX_LEN; i++) {
        diff |= (input[i] ^ correct[i]);
    }
    return (diff == 0);  /* Always checks all bytes */
}
```

---

## Side-Channel Attacks: EM Emission

### Electromagnetic Analysis

**Attack:**
- Place EM probe near CPU
- Measure electromagnetic emissions
- Extract AES key from EM patterns

**Mitigation:**
- Metal shielding (Faraday cage)
- EM filters on power rails
- Random delays in crypto operations
- Hardware countermeasures

---

## Fault Injection: Voltage Glitching

### Attack Technique

```
Normal voltage:    ████████████████████
Glitch:            ████▼▼████████████
                      ↑ CPU skips instruction

Target: Skip signature check
if (verify_signature(fw) == 0) {  ← Glitch here
    boot_firmware(fw);
}
```

**Result:** Boots unsigned firmware

---

## Fault Injection Countermeasures

```c
/* ✅ Redundant checks */
bool verify_firmware_safe(const uint8_t *fw)
{
    bool result1 = verify_signature(fw);
    bool result2 = verify_signature(fw);  /* Check twice */
    bool result3 = verify_signature(fw);  /* Three times */

    /* Majority vote */
    uint8_t votes = result1 + result2 + result3;

    if (votes < 2) {
        return false;  /* At least 2 must pass */
    }

    /* Additional check with different algorithm */
    if (!verify_hash(fw)) {
        return false;
    }

    return true;
}
```

---

## Fault Injection: Clock Glitching

**Attack:**
- Inject extra clock pulses
- CPU executes instructions incorrectly
- Skip security checks

**Mitigation:**
```c
/* Clock monitoring */
void configure_clock_security(void)
{
    /* CSS: Clock Security System */
    RCC->CR |= RCC_CR_CSSON;

    /* Triggers NMI on clock failure */
    HAL_RCC_NMI_IRQHandler();
}

void NMI_Handler(void)
{
    /* Clock glitch detected! */
    trigger_tamper_response();
}
```

---

## Tamper Detection

### Physical Tampering

```c
/* Tamper switch on enclosure */
#define TAMPER_PIN  GPIO_PIN_13

void configure_tamper_detection(void)
{
    /* Configure tamper pin */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = TAMPER_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* Enable interrupt */
    HAL_NVIC_EnableIRQ(EXTI13_IRQn);
}

void EXTI13_IRQHandler(void)
{
    /* Tamper detected! */
    erase_all_keys();
    disable_device();
    while (1);
}
```

---

## Active Mesh Protection

### PCB Protection

```
    +──────────────────────────+
    │  ┌────────────────────┐  │
    │  │ Secure IC          │  │ ◄── Copper mesh
    │  │ (Crypto keys)      │  │     around chip
    │  └────────────────────┘  │
    +──────────────────────────+
         │              │
         └──────────────┘
         Continuity check

If mesh is broken → keys erased
```

**Used in:** Payment terminals, HSMs

---

## Supply Chain Attacks

### Firmware Compromise

**Attack:**
1. Compromise build server
2. Inject backdoor into firmware
3. Sign with stolen key
4. Distribute to devices

**Mitigation:**
- **Reproducible builds** (same binary from same source)
- **Multi-party signing** (requires 2-of-3 keys)
- **Code review** (all changes audited)
- **Attestation** (devices report firmware hash)

---

## Secure Boot Verification

```c
/* Multi-stage verification */
bool secure_boot(void)
{
    /* Stage 1: Verify MCUboot (by ROM) */
    if (!rom_verify_mcuboot()) {
        halt_system();
    }

    /* Stage 2: Verify TF-M (by MCUboot) */
    if (!mcuboot_verify_tfm()) {
        halt_system();
    }

    /* Stage 3: Verify App (by TF-M) */
    if (!tfm_verify_application()) {
        halt_system();
    }

    /* All stages verified */
    return true;
}
```

---

## Downgrade Attack Prevention

```c
/* Monotonic counter in OTP */
#define OTP_FIRMWARE_VERSION_ADDR  0x1FFF7800

uint32_t get_minimum_firmware_version(void)
{
    return *(volatile uint32_t *)OTP_FIRMWARE_VERSION_ADDR;
}

bool check_firmware_rollback(uint32_t new_version)
{
    uint32_t min_version = get_minimum_firmware_version();

    if (new_version < min_version) {
        printf("🚨 Rollback attack detected!\n");
        return false;
    }

    return true;
}

void update_minimum_version(uint32_t version)
{
    /* Write to OTP (one-time programmable) */
    write_otp(OTP_FIRMWARE_VERSION_ADDR, version);
}
```

---

## Replay Attack Prevention

```c
/* Use nonces to prevent replay */
typedef struct {
    uint8_t nonce[32];
    uint8_t command[64];
    uint8_t signature[64];
} SecureCommand_t;

bool verify_command(const SecureCommand_t *cmd)
{
    /* Check if nonce was used before */
    if (nonce_already_used(cmd->nonce)) {
        printf("🚨 Replay attack detected!\n");
        return false;
    }

    /* Verify signature */
    if (!verify_signature(cmd->command, cmd->signature)) {
        return false;
    }

    /* Mark nonce as used */
    store_used_nonce(cmd->nonce);

    return true;
}
```

---

## Secure Random Number Generation

```c
/* ❌ INSECURE: Weak RNG */
uint32_t weak_random(void)
{
    static uint32_t seed = 12345;
    seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return seed;  /* Predictable! */
}

/* ✅ SECURE: Hardware TRNG */
uint32_t secure_random(void)
{
    uint32_t random;
    psa_generate_random((uint8_t *)&random, sizeof(random));
    return random;  /* Cryptographically secure */
}
```

---

## Secure Key Storage

### Key Hierarchy

```
            OTP Root Key (256-bit)
                    │
         ┌──────────┴──────────┐
         │                     │
    Storage Key          Device Key
         │                     │
    ┌────┴────┐           ┌────┴────┐
  API Key  Config      Cert    Signing

• Root key never leaves OTP
• Derived keys in PSA ITS
• Keys never in plaintext RAM
```

---

## Key Zeroization

```c
void process_sensitive_data(const uint8_t *key)
{
    uint8_t local_key[32];

    /* Copy key to local buffer */
    memcpy(local_key, key, sizeof(local_key));

    /* Use key */
    encrypt_data(local_key, ...);

    /* CRITICAL: Zeroize immediately after use */
    memset(local_key, 0, sizeof(local_key));

    /* Even better: Use volatile */
    volatile uint8_t *vptr = local_key;
    for (size_t i = 0; i < sizeof(local_key); i++) {
        vptr[i] = 0;
    }

    /* Best: Use psa_key_id_t (never exposed) */
}
```

---

## Secure Coding: Input Validation

```c
/* Validate ALL inputs from untrusted sources */
typedef struct {
    uint32_t length;  /* ← Attacker-controlled */
    uint8_t data[];
} NetworkPacket_t;

bool process_packet(const NetworkPacket_t *pkt)
{
    /* Check 1: Reasonable length */
    if (pkt->length == 0 || pkt->length > MAX_PACKET_SIZE) {
        return false;
    }

    /* Check 2: Buffer bounds */
    uint8_t buffer[MAX_PACKET_SIZE];
    if (pkt->length > sizeof(buffer)) {
        return false;
    }

    /* Check 3: Validate data format */
    if (!validate_packet_format(pkt->data, pkt->length)) {
        return false;
    }

    /* Safe to process */
    return true;
}
```

---

## Principle of Least Privilege

```c
/* Don't give excessive permissions */

/* ❌ BAD: Key can do everything */
psa_set_key_usage_flags(&attrs,
    PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT |
    PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH |
    PSA_KEY_USAGE_EXPORT);

/* ✅ GOOD: Only what's needed */
psa_set_key_usage_flags(&attrs,
    PSA_KEY_USAGE_ENCRYPT);  /* Only encrypt, cannot decrypt/sign */
```

---

## Defense in Depth

### Layered Security

```
┌─────────────────────────────────────┐
│ 8. Physical Security (tamper)       │
├─────────────────────────────────────┤
│ 7. Monitoring (intrusion detection) │
├─────────────────────────────────────┤
│ 6. Secure Boot (signature verify)   │
├─────────────────────────────────────┤
│ 5. Encryption (data protection)     │
├─────────────────────────────────────┤
│ 4. Isolation (TrustZone)            │
├─────────────────────────────────────┤
│ 3. Authentication (certificates)    │
├─────────────────────────────────────┤
│ 2. Input Validation (bounds checks) │
├─────────────────────────────────────┤
│ 1. Secure Coding (no vulnerabilities)│
└─────────────────────────────────────┘
```

**Multiple independent layers**

---

## Security Audit Checklist

### Pre-Production Verification

- [ ] All secrets provisioned securely
- [ ] Debug interfaces disabled (RDP Level 2)
- [ ] Firmware signature verification enabled
- [ ] Rollback protection active
- [ ] Tamper detection functional
- [ ] Watchdog configured
- [ ] Input validation on all APIs
- [ ] No hardcoded credentials
- [ ] Secure random number generation
- [ ] Stack overflow protection enabled
- [ ] Crypto keys never in plaintext
- [ ] Audit logging implemented

---

## Penetration Testing

### Security Validation

```c
void pentest_report(void)
{
    printf("=== Security Penetration Test ===\n");

    /* Test 1: Buffer overflow */
    test_buffer_overflow();

    /* Test 2: Invalid pointer attack */
    test_pointer_validation();

    /* Test 3: Firmware tampering */
    test_signature_bypass();

    /* Test 4: Debug access */
    test_debug_disabled();

    /* Test 5: Key extraction */
    test_key_readout();

    printf("=== Tests Complete ===\n");
}
```

---

## Incident Response Plan

```c
typedef enum {
    INCIDENT_TAMPER_DETECTED,
    INCIDENT_AUTH_FAILURE,
    INCIDENT_INVALID_FIRMWARE,
    INCIDENT_CRYPTO_FAULT
} IncidentType_t;

void handle_security_incident(IncidentType_t type)
{
    /* 1. Log incident */
    log_security_event(type);

    /* 2. Alert monitoring system */
    send_alert_to_cloud(type);

    /* 3. Take protective action */
    switch (type) {
        case INCIDENT_TAMPER_DETECTED:
            erase_all_keys();
            disable_device();
            break;

        case INCIDENT_AUTH_FAILURE:
            increment_failure_counter();
            if (failure_count > 5) {
                lock_device_temporarily();
            }
            break;
    }
}
```

---

## Secure Decommissioning

```c
void secure_device_decommission(void)
{
    printf("🚨 DECOMMISSIONING DEVICE\n");

    /* 1. Revoke certificates */
    cloud_revoke_device_certificate();

    /* 2. Erase all storage */
    psa_its_remove(UID_DEVICE_KEY);
    psa_its_remove(UID_API_KEY);
    psa_ps_remove(UID_CERTIFICATE);

    /* 3. Destroy all crypto keys */
    for (psa_key_id_t id = 1; id <= MAX_KEYS; id++) {
        psa_destroy_key(id);
    }

    /* 4. Mass erase flash */
    flash_mass_erase();

    /* 5. Burn decommission fuse (permanent) */
    burn_otp_decommission_flag();

    /* Device is now permanently disabled */
    while (1);
}
```

---

## Regulatory Compliance

### Industry Standards

| Industry | Standard | Requirements |
|----------|----------|-------------|
| **Industrial** | IEC 62443 | Network segmentation, access control |
| **Automotive** | ISO/SAE 21434 | TARA, secure development lifecycle |
| **Medical** | IEC 62304 | Risk management, verification |
| **Payment** | PCI PTS 6.0 | Physical security, key management |
| **IoT** | ETSI EN 303 645 | Secure defaults, updates, privacy |

---

## Secure Development Lifecycle (SDL)

```
1. Requirements
   └─> Define security requirements

2. Design
   └─> Threat modeling (STRIDE, TARA)

3. Implementation
   └─> Secure coding guidelines

4. Verification
   └─> Code review, static analysis

5. Testing
   └─> Penetration testing, fuzzing

6. Deployment
   └─> Secure provisioning, RDP Level 2

7. Operations
   └─> Monitoring, incident response

8. Decommissioning
   └─> Secure data erasure
```

---

## Static Analysis Tools

```bash
# Cppcheck (free, open-source)
cppcheck --enable=all --inconclusive src/

# Flawfinder (security-focused)
flawfinder src/

# Coverity (commercial, comprehensive)
cov-build --dir cov-int make
cov-analyze --dir cov-int
cov-commit-defects --dir cov-int
```

**Catch vulnerabilities before deployment**

---

## Fuzzing for Security

```c
/* Fuzz test input validation */
void fuzz_packet_parser(void)
{
    for (int i = 0; i < 100000; i++) {
        /* Generate random packet */
        uint8_t fuzz_data[1024];
        psa_generate_random(fuzz_data, sizeof(fuzz_data));

        /* Try to parse */
        parse_packet(fuzz_data, sizeof(fuzz_data));

        /* Check for crashes, hangs, memory leaks */
    }

    printf("Fuzzing complete: no crashes\n");
}
```

---

## Section Summary

### Defense Strategies

✓ Threat modeling (STRIDE, TARA)
✓ Secure coding practices
✓ Input validation and bounds checking
✓ Hardware security (RDP, tamper detection)
✓ Side-channel attack mitigations
✓ Fault injection countermeasures
✓ Defense in depth architecture
✓ Incident response planning

**Security is a continuous process, not a one-time event**

---

## Key Takeaways

### Security Mindset

1. **Assume breach:** Design for compromise
2. **Least privilege:** Minimal permissions
3. **Defense in depth:** Multiple layers
4. **Fail securely:** Safe defaults on error
5. **Validate everything:** Trust nothing
6. **Monitor continuously:** Detect anomalies
7. **Update regularly:** Patch vulnerabilities
8. **Test thoroughly:** Pentest before deployment

---

<!-- _class: lead -->
<!-- _paginate: false -->

# Questions?

**Course Complete!**
**You are now a TF-M security expert** 🎓

---

**End of Section 6**
*Total Slides: 60*
*Estimated Duration: 3 hours*

---

<!-- _class: lead -->
<!-- _paginate: false -->

# 🎉 Training Package Complete!

**Total: 295 Slides**
**Total Duration: ~14 hours**

### Thank You!

**Contact:** training@example.com
**Resources:** secure-fm-training.com

*Continue learning and building secure embedded systems!*

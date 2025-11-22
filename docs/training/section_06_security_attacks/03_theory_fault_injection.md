# Section 6: Security & Attack Resistance

## Module 03: Fault Injection Attacks

**Learning Objectives:**
- Understand fault injection attack principles
- Learn voltage and clock glitching techniques
- Understand electromagnetic and laser fault injection
- Implement fault detection and countermeasures
- Design redundant security checks
- Configure hardware fault detection on STM32U5

---

## 📋 Table of Contents

1. [Introduction to Fault Injection](#1-introduction)
2. [Voltage Glitching](#2-voltage-glitching)
3. [Clock Glitching](#3-clock-glitching)
4. [Electromagnetic Fault Injection](#4-electromagnetic-fault-injection)
5. [Laser Fault Injection](#5-laser-fault-injection)
6. [Fault Attack Examples](#6-fault-attack-examples)
7. [Countermeasures](#7-countermeasures)
8. [Practical Implementation](#8-practical-implementation)

---

## 1. Introduction to Fault Injection

### 1.1 What is Fault Injection?

**Definition:** Intentionally inducing errors (faults) in a device's operation to cause security failures or extract secrets.

**Goal:** Cause the processor to:
- Skip security checks
- Corrupt data values
- Execute incorrect instructions
- Jump to attacker-controlled code

**Key Difference from Side-Channel Attacks:**
```
Side-Channel Attacks          Fault Injection Attacks
────────────────────────────────────────────────────
Passive observation           Active manipulation
No modification needed        Introduces errors
Extracts secrets              Bypasses security
Non-invasive                  May be invasive
```

### 1.2 Fault Attack Workflow

```
1. Target Identification
   ↓
   Identify security-critical code (signature check, access control)

2. Fault Injection
   ↓
   Apply voltage glitch, clock glitch, EM pulse, or laser

3. Fault Exploitation
   ↓
   Exploit induced error to bypass security or extract data

4. Result Verification
   ↓
   Confirm security was bypassed or data extracted
```

### 1.3 Types of Faults

**Transient Faults:** Temporary, single-event errors
```c
// Normal execution:
if (signature_valid == 1) {
    allow_access();
}

// With transient fault (bit flip):
if (signature_valid == 0) {  // ← Fault flipped the comparison!
    allow_access();          // Executed despite invalid signature!
}
```

**Permanent Faults:** Long-lasting or irreversible errors
- Used less often for attacks (can damage device)
- Example: Burning fuses, corrupting flash

**Instruction Skips:** CPU skips instruction execution
```assembly
; Normal:
CMP R0, #1      ; Compare
BEQ allow       ; Branch if equal

; With fault (instruction skip):
NOP             ; ← CMP replaced with NOP!
BEQ allow       ; Branch always taken!
```

### 1.4 Fault Models

**Bit Flip Model:**
- Single bit changes: 0→1 or 1→0
- Most common in practical attacks

**Instruction Skip Model:**
- Entire instruction not executed
- Replaced with NOP (no operation)

**Random Corruption Model:**
- Multiple bits flip randomly
- Harder to exploit, but possible

**Set/Reset Model:**
- All bits set to 0 or all to 1
- Specific to certain attack techniques

---

## 2. Voltage Glitching

### 2.1 Principle

**How it Works:**
1. Briefly drop or spike the supply voltage (VDD)
2. Causes logic gates to malfunction
3. CPU may skip instructions or corrupt data
4. Voltage returns to normal before crash

**Timing is Critical:**
```
VDD
 │
 │  Normal: 3.3V ──────────────────────────
 │                    ↓ Glitch (100-500ns)
 │           ─────────┐  ┌─────────────────
 │                    └──┘  ← Drop to ~2.5V
 │
 └──────────────────────────────────────────→ Time
                      ↑
                 Target instruction
```

### 2.2 Voltage Glitch Equipment

**Basic Setup:**
```
┌──────────────────────────────────────────────────┐
│                                                   │
│  ┌────────────┐        ┌──────────────┐         │
│  │ Power      │        │ MOSFET       │         │
│  │ Supply 3.3V├───────→│ Switch       ├────┐    │
│  └────────────┘        │ (controlled) │    │    │
│                        └──────┬───────┘    │    │
│                               │            │    │
│  ┌────────────────────────────┴────────────┤    │
│  │ Trigger Signal (from logic analyzer)    │    │
│  └──────────────────────────────────────────┘   │
│                                             │    │
│                                             ↓    │
│                                      ┌──────────┐│
│                                      │ Target   ││
│                                      │ MCU      ││
│                                      │ VDD pin  ││
│                                      └──────────┘│
└──────────────────────────────────────────────────┘
```

**Professional Tools:**
- ChipWhisperer (~$500-$2000)
- Riscure Inspector (~$50,000+)

**DIY Setup:**
- MOSFET switch controlled by FPGA/microcontroller
- Oscilloscope for monitoring
- Logic analyzer for triggering
- Cost: ~$100-$500

### 2.3 Attack Example: Signature Verification Bypass

**Target Code:**
```c
bool verify_firmware_signature(uint8_t *firmware, size_t len,
                                uint8_t *signature)
{
    uint8_t computed_hash[32];

    // Compute SHA-256 hash of firmware
    sha256(firmware, len, computed_hash);

    // Verify ECDSA signature
    bool valid = ecdsa_verify(computed_hash, 32, signature);

    if (valid) {  // ← Target this branch!
        return true;
    }

    return false;
}
```

**Attack Steps:**
1. **Identify target:** The `if (valid)` comparison
2. **Trigger setup:** Logic analyzer monitors GPIO or UART
3. **Inject glitch:** Drop VDD for 200ns at exact moment
4. **Result:** CPU skips comparison, returns true even if signature invalid!

**ARM Assembly for `if (valid)`:**
```assembly
    CMP   R0, #1       ; Compare valid with 1
    BNE   .L_false     ; Branch if not equal
    MOVS  R0, #1       ; Return true
    BX    LR
.L_false:
    MOVS  R0, #0       ; Return false
    BX    LR
```

**With Voltage Glitch:**
```assembly
    CMP   R0, #1       ; Compare
    NOP                ; ← BNE replaced with NOP (glitch!)
    MOVS  R0, #1       ; Always returns true!
    BX    LR
```

### 2.4 Voltage Glitch Parameters

**Critical Parameters:**

1. **Glitch Offset:** Time from trigger to glitch
   - Typical range: 0 - 10,000 cycles
   - Needs precise calibration

2. **Glitch Width:** Duration of voltage drop
   - Typical: 10ns - 1µs
   - Too short: No effect
   - Too long: Device resets

3. **Glitch Depth:** How much voltage drops
   - Typical: 0.5V - 1.5V drop from nominal
   - Too shallow: No effect
   - Too deep: Device crashes

**Parameter Space Exploration:**
```python
# Example parameter sweep for successful glitch
best_offset = None
best_width = None

for offset in range(0, 10000, 10):      # Offset in clock cycles
    for width in range(10, 500, 5):     # Width in nanoseconds
        result = inject_glitch(offset, width, depth=0.8)

        if result == "bypass_success":
            best_offset = offset
            best_width = width
            print(f"Found: offset={offset}, width={width}")
            break
```

---

## 3. Clock Glitching

### 3.1 Principle

**How it Works:**
1. Inject extra clock edges
2. CPU samples data before it's stable
3. Reads incorrect values or skips instructions
4. Normal clock resumes

**Clock Glitch Visualization:**
```
Normal Clock:
     ┌─┐   ┌─┐   ┌─┐   ┌─┐
  ───┘ └───┘ └───┘ └───┘ └───

Clock with Glitch:
     ┌─┐   ┌┐┌─┐   ┌─┐   ┌─┐
  ───┘ └───┘└┘ └───┘ └───┘ └───
           ↑
      Extra edge!
```

### 3.2 Clock Glitch Equipment

**Requirements:**
- Access to clock signal (external crystal or internal oscillator)
- Fast clock multiplexer (FPGA-based)
- Trigger mechanism

**Setup:**
```
┌────────────────────────────────────────────┐
│  Normal Clock                              │
│  (from crystal)                            │
│      │                                     │
│      ↓                                     │
│  ┌───────────────┐                        │
│  │ Clock MUX     │   Trigger              │
│  │ (FPGA)        │←──────────             │
│  │               │                        │
│  │ - Normal: Pass through                │
│  │ - Glitch: Insert extra edge           │
│  └───────┬───────┘                        │
│          │                                 │
│          ↓                                 │
│    ┌──────────┐                           │
│    │ Target   │                           │
│    │ MCU      │                           │
│    └──────────┘                           │
└────────────────────────────────────────────┘
```

### 3.3 Clock Glitching vs. Voltage Glitching

| Aspect | Voltage Glitching | Clock Glitching |
|--------|------------------|-----------------|
| **Access Required** | VDD pin | Clock input |
| **Equipment Cost** | Low (~$100) | Medium (~$500) |
| **Success Rate** | Medium | High |
| **Device Damage Risk** | Medium | Low |
| **Repeatability** | Medium | High |
| **Precision** | ±10ns | ±1ns |

### 3.4 Attack Example: Loop Counter Corruption

**Target Code:**
```c
// PIN entry with 3 attempts
bool authenticate_pin(void)
{
    int attempts = 0;

    while (attempts < 3) {  // ← Target this comparison!
        uint32_t entered_pin = read_pin_from_keypad();

        if (entered_pin == stored_pin) {
            return true;
        }

        attempts++;
    }

    return false;  // Failed after 3 attempts
}
```

**Attack:**
1. **Inject clock glitch** when `attempts` is incremented
2. **Result:** `attempts++` may not execute or corrupt value
3. **Outcome:** Unlimited PIN attempts!

**Even Better Attack:**
Target the comparison `attempts < 3`:
```assembly
    LDR   R0, [attempts]
    CMP   R0, #3         ; ← Glitch here
    BGE   .L_failed      ; Branch if >= 3
```

With glitch: `BGE` never taken → infinite attempts

---

## 4. Electromagnetic Fault Injection (EMFI)

### 4.1 Principle

**How it Works:**
1. Generate strong electromagnetic pulse
2. Induces current in chip's internal wiring
3. Causes bit flips or instruction skips
4. Non-contact (works through plastic enclosures)

**EM Pulse Generator:**
```
High Voltage Pulse
(several hundred volts)
         ↓
    ┌─────────┐
    │ Coil    │
    │ (EM     │──→ EM Field (very strong, localized)
    │ Probe)  │
    └────┬────┘
         │
         ↓
    [Target Chip]
    (induces currents in internal traces)
```

### 4.2 EMFI Equipment

**Professional:**
- Riscure EM-FI Transient Probe (~$30,000)
- Langer LF-R 400 probe (~$5,000)

**DIY/Research:**
- Camera flash capacitor discharge (~$50)
- Small coil (1-2mm diameter)
- High-voltage switch (MOSFET or spark gap)

**Safety Warning:** ⚠️
- High voltage (200-400V)
- Can damage equipment
- Wear ESD protection
- Use shielding

### 4.3 EMFI vs. Other Techniques

**Advantages:**
- Non-contact (no need to touch pins)
- Works through plastic enclosures
- Spatially localized (can target specific chip areas)
- Highly repeatable

**Disadvantages:**
- Expensive equipment
- Requires precise positioning
- May damage device

### 4.4 EMFI Attack Example: AES Key Extraction

**Differential Fault Analysis (DFA) on AES:**

```c
void aes_encrypt(uint8_t *plaintext, uint8_t *key, uint8_t *ciphertext)
{
    uint8_t state[16];
    memcpy(state, plaintext, 16);

    add_round_key(state, key, 0);

    for (int round = 1; round <= 9; round++) {
        sub_bytes(state);
        shift_rows(state);
        mix_columns(state);
        add_round_key(state, key, round);
    }

    // Final round (no mix_columns)
    sub_bytes(state);        // ← Inject fault here!
    shift_rows(state);
    add_round_key(state, key, 10);

    memcpy(ciphertext, state, 16);
}
```

**Attack:**
1. **Normal encryption:** Capture correct ciphertext
2. **Faulted encryption:** Inject EM pulse during round 9
3. **Fault causes:** Single byte error in state
4. **Differential analysis:** Compare normal vs. faulted ciphertext
5. **Result:** Extract last round key (128 bits)
6. **Recover full key:** Reverse key schedule

**Mathematics:**
With ~200 faulted encryptions, full AES-128 key can be extracted!

---

## 5. Laser Fault Injection

### 5.1 Principle

**How it Works:**
1. Focus laser beam on chip die
2. Photoelectric effect creates electron-hole pairs
3. Locally disrupts transistor operation
4. Causes bit flips in memory or registers

**Laser Types:**
- **White light:** Low precision, easy to use
- **Infrared laser:** Penetrates silicon, precise
- **UV laser:** Surface only, very precise

### 5.2 Requirements

**Equipment:**
- High-power laser (several watts)
- Microscope with precise XYZ stage
- Decapsulated chip (IC package removed)
- Timing control (nanosecond precision)

**Cost:** $100,000+ (professional setup)

### 5.3 Laser Fault Injection Process

```
1. Decapsulation
   ↓
   Remove IC plastic package with fuming nitric acid
   Expose silicon die

2. Die Inspection
   ↓
   Use microscope to identify target structures
   (RAM cells, flip-flops, logic gates)

3. Laser Calibration
   ↓
   Test laser power and focus
   Find minimum energy for fault

4. Attack Execution
   ↓
   Trigger laser at precise moment
   Target specific memory bit or register

5. Result Analysis
   ↓
   Check if desired fault occurred
   Iterate to optimize parameters
```

### 5.4 Laser Fault Attack Example: Privilege Escalation

**Target:** Secure/Non-Secure flag in CPU register

```c
// TF-M: Check if caller is from Secure world
bool is_caller_secure(void)
{
    uint32_t control = __get_CONTROL();
    return (control & CONTROL_nPRIV_Msk) == 0;  // Privileged = Secure
}

void secure_function(void)
{
    if (!is_caller_secure()) {
        return;  // Reject Non-Secure callers
    }

    // Perform privileged operation
    access_secret_keys();
}
```

**Attack:**
1. **Target:** CONTROL register bit
2. **Laser:** Focus on CPU core register file
3. **Timing:** When `is_caller_secure()` reads CONTROL
4. **Result:** Flip nPRIV bit → Non-Secure caller appears Secure!

---

## 6. Fault Attack Examples

### 6.1 RSA-CRT Fault Attack (Bellcore Attack)

**Background:** RSA with Chinese Remainder Theorem (CRT) is faster but vulnerable to faults.

**RSA-CRT Algorithm:**
```c
// RSA private key operation with CRT
uint32_t rsa_decrypt_crt(uint32_t ciphertext, rsa_key_t *key)
{
    // Compute modulo p
    uint32_t m1 = mod_exp(ciphertext, key->dp, key->p);

    // Compute modulo q (← inject fault here!)
    uint32_t m2 = mod_exp(ciphertext, key->dq, key->q);

    // Combine using CRT
    uint32_t h = (key->qinv * (m1 - m2)) % key->p;
    uint32_t m = m2 + h * key->q;

    return m;
}
```

**Attack:**
1. **Inject fault** during computation of `m2`
2. **Get faulty signature:** `s_faulty`
3. **Get correct signature:** `s_correct`
4. **Compute:** `gcd(s_faulty - s_correct, N)`
5. **Result:** Recovers prime factor `q`!
6. **Break RSA:** With `q`, compute `p = N/q` → Full key recovery

**Impact:** Single fault breaks RSA completely!

### 6.2 AES Differential Fault Analysis (DFA)

**Attack Overview:**
```python
# Simplified DFA attack on AES

def extract_aes_key_dfa(oracle_encrypt, num_faults=200):
    """
    Extract AES key using Differential Fault Analysis

    oracle_encrypt: Function that encrypts with optional fault injection
    num_faults: Number of faulted encryptions (typically 200-300)
    """
    plaintext = b'\x00' * 16
    correct_ciphertext = oracle_encrypt(plaintext, fault=False)

    fault_pairs = []
    for _ in range(num_faults):
        # Inject fault in round 9
        faulted_ciphertext = oracle_encrypt(plaintext, fault=True, round=9)
        fault_pairs.append((correct_ciphertext, faulted_ciphertext))

    # Differential analysis (simplified)
    # For each byte of last round key:
    key_candidates = []
    for byte_pos in range(16):
        candidates = []

        for key_guess in range(256):
            # Check if key_guess is consistent with all fault pairs
            consistent = all(
                is_consistent(pair, byte_pos, key_guess)
                for pair in fault_pairs
            )

            if consistent:
                candidates.append(key_guess)

        # Should narrow down to 1-2 candidates per byte
        key_candidates.append(candidates)

    # Recover full key from round 10 key
    return reverse_key_schedule(key_candidates)
```

### 6.3 Secure Boot Bypass

**Classic Attack:**
```c
void boot_firmware(void)
{
    uint8_t *firmware = (uint8_t *)FIRMWARE_START;
    size_t firmware_len = FIRMWARE_SIZE;
    uint8_t *signature = (uint8_t *)SIGNATURE_ADDR;

    bool valid = verify_signature(firmware, firmware_len, signature);

    if (valid) {  // ← Target this check!
        jump_to_firmware(FIRMWARE_START);
    } else {
        boot_recovery_mode();
    }
}
```

**Fault Injection Targets:**

1. **Skip verification entirely:**
   ```
   Glitch at verify_signature() call
   → Function never executes
   → `valid` contains random/old value
   ```

2. **Corrupt comparison:**
   ```
   Glitch at `if (valid)` comparison
   → Branch always taken
   ```

3. **Modify return value:**
   ```
   Glitch when verify_signature() returns
   → Force return value to true
   ```

---

## 7. Countermeasures

### 7.1 Redundant Checks

**Principle:** Perform security check multiple times

**Implementation:**
```c
// ❌ Single check (vulnerable)
if (signature_valid) {
    allow_access();
}

// ✓ Double check
bool check1 = verify_signature(...);
bool check2 = verify_signature(...);

if (check1 && check2) {
    allow_access();
}

// ✓✓ Even better: Inverse check
bool check_positive = verify_signature(...);
bool check_negative = !verify_signature_inverse(...);  // Inverse logic

if (check_positive && check_negative) {
    allow_access();
}
```

**Why Inverse Check?**
- Fault must corrupt BOTH checks
- Checks use different logic → harder to glitch both
- If only one corrupted, mismatch detected

### 7.2 Loop Counters

**Problem:** Fault can corrupt loop counter

**Solution:**
```c
// ❌ Vulnerable
for (int i = 0; i < 3; i++) {
    // Try PIN verification
}

// ✓ Protected
int i = 0;
int i_inverse = 0xFF;  // Complementary counter

while (i < 3) {
    if ((i + i_inverse) != 0xFF) {
        // Fault detected! Counters don't match
        security_halt();
    }

    // Try PIN verification

    i++;
    i_inverse--;
}
```

### 7.3 Canary Values

**Stack Canaries:** Detect stack corruption
```c
#define CANARY_VALUE 0xDEADBEEF

void secure_function(void)
{
    volatile uint32_t canary = CANARY_VALUE;

    // Perform operation

    if (canary != CANARY_VALUE) {
        // Stack corrupted or fault injected!
        security_halt();
    }
}
```

**Data Canaries:** Protect critical variables
```c
typedef struct {
    uint32_t value;
    uint32_t canary_before;
    uint32_t canary_after;
} protected_value_t;

void write_protected(protected_value_t *p, uint32_t new_value)
{
    p->canary_before = CANARY_VALUE;
    p->value = new_value;
    p->canary_after = CANARY_VALUE;
}

bool verify_protected(protected_value_t *p)
{
    return (p->canary_before == CANARY_VALUE) &&
           (p->canary_after == CANARY_VALUE);
}
```

### 7.4 Random Delays

**Principle:** Make timing unpredictable

```c
void secure_operation_with_jitter(void)
{
    // Add random delay before critical operation
    uint32_t delay = get_true_random() % 1000;
    for (volatile uint32_t i = 0; i < delay; i++) {
        __NOP();
    }

    // Perform security check
    bool valid = verify_signature(...);

    // Add random delay after
    delay = get_true_random() % 1000;
    for (volatile uint32_t i = 0; i < delay; i++) {
        __NOP();
    }

    if (valid) {
        allow_access();
    }
}
```

**Effectiveness:**
- Attacker must search larger parameter space
- Success rate decreases
- Not foolproof (patient attacker can still succeed)

### 7.5 Fault Detection in Hardware

**STM32U5 Security Features:**

1. **Voltage Monitoring (PVD - Programmable Voltage Detector):**
```c
// Configure PVD to detect voltage glitches
void configure_pvd_protection(void)
{
    PWR_PVDTypeDef pVD_Config;

    // Detect when VDD < 2.8V or > 3.6V
    pVD_Config.PVDLevel = PWR_PVDLEVEL_7;  // 2.8V threshold
    pVD_Config.Mode = PWR_PVD_MODE_IT_RISING_FALLING;

    HAL_PWR_ConfigPVD(&pVD_Config);
    HAL_PWR_EnablePVD();

    // Enable interrupt
    HAL_NVIC_SetPriority(PVD_AVD_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(PVD_AVD_IRQn);
}

// PVD interrupt handler
void PVD_AVD_IRQHandler(void)
{
    // Voltage glitch detected!
    erase_all_keys();
    system_lockdown();
    NVIC_SystemReset();
}
```

2. **Clock Security System (CSS):**
```c
// Detect clock glitches
void configure_css_protection(void)
{
    // Enable Clock Security System
    HAL_RCC_EnableCSS();

    // CSS will trigger NMI if HSE clock fails
}

void NMI_Handler(void)
{
    if (__HAL_RCC_GET_IT(RCC_IT_CSS)) {
        // Clock failure detected (possible glitch)
        __HAL_RCC_CLEAR_IT(RCC_IT_CSS);

        erase_all_keys();
        security_halt();
    }
}
```

3. **Memory Protection (MPU):**
```c
// Detect unexpected code execution
void configure_mpu_protection(void)
{
    MPU_Region_InitTypeDef MPU_Config;

    // Make bootloader region execute-only
    MPU_Config.Number = MPU_REGION_NUMBER0;
    MPU_Config.BaseAddress = BOOTLOADER_START;
    MPU_Config.Size = MPU_REGION_SIZE_32KB;
    MPU_Config.AccessPermission = MPU_REGION_PRIV_RO;  // Read-only
    MPU_Config.IsExecute = MPU_INSTRUCTION_ACCESS_ENABLE;
    MPU_Config.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_Config.IsCacheable = MPU_ACCESS_CACHEABLE;
    MPU_Config.IsShareable = MPU_ACCESS_NOT_SHAREABLE;

    HAL_MPU_ConfigRegion(&MPU_Config);
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}
```

### 7.6 TF-M Fault Resistance

**TF-M Built-in Protections:**

1. **Double Signature Verification:**
```c
// TF-M bootloader (BL2) verifies images twice
psa_status_t boot_platform_post_load(uint32_t image_id)
{
    // First verification
    if (boot_verify_image(image_id) != 0) {
        return PSA_ERROR_INVALID_SIGNATURE;
    }

    // Second verification (redundant check)
    if (boot_verify_image(image_id) != 0) {
        return PSA_ERROR_INVALID_SIGNATURE;
    }

    // Additional CRC check
    if (boot_check_crc(image_id) != 0) {
        return PSA_ERROR_CORRUPTION_DETECTED;
    }

    return PSA_SUCCESS;
}
```

2. **Watchdog Timers:**
```c
// TF-M uses watchdog to detect hung operations
void tfm_hal_platform_init(void)
{
    // Configure watchdog
    watchdog_init();
    watchdog_set_timeout(1000);  // 1 second

    // Kick in main loop
    while (1) {
        watchdog_kick();
        tfm_core_process();
    }
}
```

3. **Memory Scrubbing:**
```c
// TF-M scrubs sensitive memory after use
void tfm_crypto_clear_temp_buffers(void)
{
    // Overwrite with random data (prevents residual data attacks)
    uint8_t random[64];
    psa_generate_random(random, sizeof(random));

    memcpy(temp_buffer, random, sizeof(temp_buffer));

    // Then zero
    memset(temp_buffer, 0, sizeof(temp_buffer));
}
```

---

## 8. Practical Implementation

### 8.1 Complete Example: Fault-Resistant PIN Verification

```c
/**
 * Fault-resistant PIN verification
 * Combines multiple countermeasures
 */

#define MAX_PIN_ATTEMPTS 3
#define PIN_LENGTH 4
#define CANARY_VALUE 0x5A5A5A5A

typedef struct {
    uint32_t canary_start;
    uint8_t pin[PIN_LENGTH];
    uint32_t canary_end;
    uint32_t attempt_count;
    uint32_t attempt_count_inv;  // Complementary counter
} pin_state_t;

static pin_state_t g_pin_state = {
    .canary_start = CANARY_VALUE,
    .pin = {1, 2, 3, 4},
    .canary_end = CANARY_VALUE,
    .attempt_count = 0,
    .attempt_count_inv = ~0U
};

/**
 * Check canaries and counter integrity
 */
static bool verify_pin_state_integrity(void)
{
    // Check canaries
    if (g_pin_state.canary_start != CANARY_VALUE) {
        return false;
    }
    if (g_pin_state.canary_end != CANARY_VALUE) {
        return false;
    }

    // Check complementary counters
    if ((g_pin_state.attempt_count + g_pin_state.attempt_count_inv) != ~0U) {
        return false;
    }

    return true;
}

/**
 * Constant-time PIN comparison
 */
static bool compare_pin_ct(const uint8_t *entered, const uint8_t *stored)
{
    volatile uint8_t diff = 0;

    for (int i = 0; i < PIN_LENGTH; i++) {
        diff |= (entered[i] ^ stored[i]);
    }

    return (diff == 0);
}

/**
 * Increment attempt counter with integrity checks
 */
static void increment_attempts(void)
{
    // Add random delay (anti-glitch)
    uint32_t delay = get_random() % 100;
    for (volatile uint32_t i = 0; i < delay; i++) __NOP();

    // Increment
    g_pin_state.attempt_count++;
    g_pin_state.attempt_count_inv--;

    // Verify integrity immediately
    if (!verify_pin_state_integrity()) {
        security_halt();
    }
}

/**
 * Main PIN verification function
 */
bool verify_pin(const uint8_t *entered_pin)
{
    // Check integrity before starting
    if (!verify_pin_state_integrity()) {
        security_halt();
        return false;
    }

    // Check attempt limit (with redundancy)
    bool limit_check1 = (g_pin_state.attempt_count >= MAX_PIN_ATTEMPTS);
    bool limit_check2 = (g_pin_state.attempt_count >= MAX_PIN_ATTEMPTS);

    if (limit_check1 || limit_check2) {
        security_lockout();
        return false;
    }

    // Add random delay before comparison
    uint32_t delay = get_random() % 200;
    for (volatile uint32_t i = 0; i < delay; i++) __NOP();

    // Compare PIN (constant-time)
    bool match1 = compare_pin_ct(entered_pin, g_pin_state.pin);

    // Second comparison (redundant check)
    delay = get_random() % 100;
    for (volatile uint32_t i = 0; i < delay; i++) __NOP();

    bool match2 = compare_pin_ct(entered_pin, g_pin_state.pin);

    // Both must match
    if (match1 && match2 && (match1 == match2)) {
        // Success - reset attempt counter
        g_pin_state.attempt_count = 0;
        g_pin_state.attempt_count_inv = ~0U;
        return true;
    }

    // Failed attempt - increment counter
    increment_attempts();

    // Check integrity after incrementing
    if (!verify_pin_state_integrity()) {
        security_halt();
        return false;
    }

    return false;
}

/**
 * Security halt (called on fault detection)
 */
static void security_halt(void)
{
    // Erase sensitive data
    memset(&g_pin_state, 0, sizeof(g_pin_state));

    // Trigger permanent lockdown
    set_tamper_flag();

    // Reset system
    NVIC_SystemReset();

    // Infinite loop (should never reach due to reset)
    while (1) {
        __WFI();
    }
}
```

### 8.2 Hardware Configuration (STM32U5)

```c
/**
 * Configure all hardware fault detection features
 */
void configure_fault_detection(void)
{
    // 1. Programmable Voltage Detector (PVD)
    PWR_PVDTypeDef pVD_Config = {
        .PVDLevel = PWR_PVDLEVEL_7,  // 2.8V
        .Mode = PWR_PVD_MODE_IT_RISING_FALLING
    };
    HAL_PWR_ConfigPVD(&pVD_Config);
    HAL_PWR_EnablePVD();

    // 2. Clock Security System (CSS)
    HAL_RCC_EnableCSS();

    // 3. Watchdog Timer
    IWDG_HandleTypeDef hiwdg = {
        .Instance = IWDG,
        .Init.Prescaler = IWDG_PRESCALER_64,
        .Init.Reload = 4095,  // ~1 second timeout
        .Init.Window = IWDG_WINDOW_DISABLE
    };
    HAL_IWDG_Init(&hiwdg);

    // 4. Tamper Detection
    RTC_TamperTypeDef sTamper = {
        .Tamper = RTC_TAMPER_1,
        .Trigger = RTC_TAMPERTRIGGER_RISINGEDGE,
        .NoErase = RTC_TAMPER_ERASE_BACKUP_ENABLE
    };
    HAL_RTCEx_SetTamper(&hrtc, &sTamper);

    // 5. Memory Protection Unit (MPU)
    configure_mpu_protection();

    // Enable fault interrupts
    HAL_NVIC_SetPriority(PVD_AVD_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(PVD_AVD_IRQn);
    HAL_NVIC_SetPriority(RCC_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RCC_IRQn);
}
```

---

## 📊 Summary

### Key Takeaways

1. **Fault Injection is Powerful**
   - Can bypass security checks with single fault
   - Breaks RSA, AES, secure boot
   - Relatively affordable equipment

2. **Multiple Attack Techniques**
   - Voltage glitching (cheap, effective)
   - Clock glitching (very effective)
   - EM fault injection (non-contact)
   - Laser fault injection (precise, expensive)

3. **Defense in Depth Required**
   - No single countermeasure is perfect
   - Combine multiple techniques:
     * Redundant checks
     * Complementary counters
     * Random delays
     * Hardware monitoring

4. **Hardware Features are Essential**
   - STM32U5 PVD, CSS, watchdog
   - Tamper detection
   - Memory protection (MPU)

5. **TF-M Provides Good Baseline**
   - Redundant checks in bootloader
   - Watchdog protection
   - Memory scrubbing
   - But application must add own checks!

### Countermeasure Summary Table

| Countermeasure | Overhead | Effectiveness | When to Use |
|----------------|----------|---------------|-------------|
| Redundant checks | Low | High | Always |
| Inverse logic | Low | Very High | Critical checks |
| Complementary counters | Low | High | Loops |
| Canaries | Low | Medium | Memory protection |
| Random delays | Medium | Medium | Timing uncertainty |
| PVD (voltage monitor) | None | High | Always enable |
| CSS (clock monitor) | None | High | Always enable |
| Watchdog | Low | Medium | Long operations |

### Next Module

**Module 04: Physical Attack Mitigation**
- Tamper detection and response
- Secure enclosures
- Debug port lockdown
- Flash readout protection
- Key extraction prevention

**Lab 27: Fault Injection Defense** (Section 6)
- Implement redundant security checks
- Configure STM32U5 fault detection (PVD, CSS)
- Test with simulated faults
- Measure effectiveness

---

## 📚 References

1. **"Fault Attacks on Cryptographic Devices"** - Tunstall, Mukhopadhyay, Ali (Springer)
2. **"Differential Fault Analysis of AES"** - Piret, Quisquater
3. **"Fault Attacks on RSA-CRT"** - Boneh, DeMillo, Lipton (Bellcore Attack)
4. **ChipWhisperer Documentation** - NewAE Technology
5. **ARM TrustZone Security Whitepaper**
6. **STM32U5 Reference Manual** - Security features chapter
7. **PSA Certified Level 2 Requirements** - Fault resistance

---

**End of Module 03**

Continue to [Module 04: Physical Attack Mitigation](04_theory_physical_attacks.md) →

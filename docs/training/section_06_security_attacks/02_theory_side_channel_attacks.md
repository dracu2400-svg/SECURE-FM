# Section 6: Security & Attack Resistance

## Module 02: Side-Channel Attacks

**Learning Objectives:**
- Understand principles of side-channel analysis
- Learn about power analysis attacks (SPA, DPA, CPA)
- Understand timing attacks and countermeasures
- Learn electromagnetic analysis techniques
- Implement constant-time cryptographic code
- Apply side-channel countermeasures in TF-M

---

## 📋 Table of Contents

1. [Introduction to Side-Channel Attacks](#1-introduction)
2. [Power Analysis Attacks](#2-power-analysis-attacks)
3. [Timing Attacks](#3-timing-attacks)
4. [Electromagnetic (EM) Analysis](#4-electromagnetic-analysis)
5. [Cache Timing Attacks](#5-cache-timing-attacks)
6. [Countermeasures](#6-countermeasures)
7. [Constant-Time Programming](#7-constant-time-programming)
8. [Practical Examples](#8-practical-examples)

---

## 1. Introduction to Side-Channel Attacks

### 1.1 What Are Side-Channel Attacks?

**Definition:** Side-channel attacks exploit unintentional information leakage from physical implementation of cryptographic algorithms, rather than weaknesses in the algorithms themselves.

**Key Concept:**
```
Traditional Cryptanalysis          Side-Channel Analysis
─────────────────────────────────────────────────────────
Attack the math                    Attack the implementation
Analyze ciphertext                 Measure physical properties
Pure software analysis             Requires hardware access
Theoretical                        Practical
```

**Why Side-Channels Exist:**

Even though an algorithm like AES-256 is mathematically secure, its physical implementation leaks information:

```
┌─────────────────────────────────────────────────┐
│         Cryptographic Algorithm (AES)            │
│                                                  │
│   Mathematically Secure ✓                       │
│   - No known analytical attacks                 │
│   - 256-bit key = 2^256 possibilities          │
│   - Brute force impractical                     │
└─────────────────────────────────────────────────┘
              ↓ Implementation
┌─────────────────────────────────────────────────┐
│         Physical Implementation                  │
│                                                  │
│   ❌ Leaks information via:                     │
│   - Power consumption                           │
│   - Execution time                              │
│   - Electromagnetic emissions                   │
│   - Cache access patterns                       │
│   - Acoustic noise                              │
│   - Heat dissipation                            │
└─────────────────────────────────────────────────┘
```

### 1.2 Types of Side-Channel Attacks

| Attack Type | Observable | Equipment | Difficulty | Effectiveness |
|-------------|-----------|-----------|------------|---------------|
| **Simple Power Analysis (SPA)** | Power waveform | Oscilloscope | Low | Medium |
| **Differential Power Analysis (DPA)** | Statistical power | Oscilloscope + PC | Medium | High |
| **Timing Attack** | Execution time | Clock/Timer | Very Low | High |
| **EM Analysis** | EM radiation | EM probe | Medium | High |
| **Cache Timing** | Cache hits/misses | Software timing | Low | Medium |
| **Acoustic** | Sound emissions | Microphone | High | Low |

### 1.3 Attack Workflow

```
1. Capture Phase
   ↓
   Trigger device to perform crypto operation
   Record side-channel measurements (power, time, EM)
   Repeat many times (10,000+ traces typical)

2. Analysis Phase
   ↓
   Statistical analysis of traces
   Correlate measurements with key hypotheses
   Extract secret key

3. Verification Phase
   ↓
   Test extracted key
   Decrypt known ciphertext
   Success!
```

---

## 2. Power Analysis Attacks

### 2.1 Why Power Consumption Leaks Information

**CMOS Power Consumption:**

Digital circuits (CMOS) consume power when:
1. Switching states (0→1 or 1→0)
2. Charging/discharging capacitances
3. Short-circuit current during transitions

**Power = f(data being processed)**

```c
// Example: Processing different data values

// Case 1: Data = 0x00
XOR R0, R1  // Few transistors switch → Low power

// Case 2: Data = 0xFF
XOR R0, R1  // Many transistors switch → High power
```

**Hamming Weight Dependency:**

Power consumption is proportional to the **Hamming weight** (number of '1' bits) being processed.

```
Data Value    Binary        Hamming Weight    Power Consumption
───────────────────────────────────────────────────────────────
0x00          00000000      0                 Low
0x01          00000001      1                 ↑
0x03          00000011      2                 ↑
0x0F          00001111      4                 ↑
0xFF          11111111      8                 High
```

### 2.2 Simple Power Analysis (SPA)

**Principle:** Visual inspection of power traces reveals secret-dependent operations.

**Example: RSA Square-and-Multiply**

```c
// Insecure RSA exponentiation
uint32_t rsa_exp(uint32_t base, uint32_t exponent, uint32_t modulus)
{
    uint32_t result = 1;
    for (int i = 31; i >= 0; i--) {
        result = (result * result) % modulus;  // Square (always)

        if (exponent & (1 << i)) {
            result = (result * base) % modulus;  // Multiply (if bit = 1)
        }
    }
    return result;
}
```

**Power Trace:**

```
Power
  ↑
  │  S   SM   S   SM   S   S   SM   S     (S=Square, M=Multiply)
  │ ┌┐  ┌┬┐  ┌┐  ┌┬┐  ┌┐  ┌┐  ┌┬┐  ┌┐
  │ ││  │││  ││  │││  ││  ││  │││  ││
──┴─┴┴──┴┴┴──┴┴──┴┴┴──┴┴──┴┴──┴┴┴──┴┴────→ Time

  Exponent bits: 1  0  1  0  0  1  0
```

**Attack:** Directly read the exponent (private key) from power trace!

### 2.3 Differential Power Analysis (DPA)

**Principle:** Statistical analysis of many power traces to extract secrets.

**Attack Steps:**

1. **Capture Phase:**
   - Encrypt many plaintexts (e.g., 10,000)
   - Record power trace for each encryption
   - Each trace is 1000+ sample points

2. **Hypothesis Phase:**
   - For each possible key byte (0x00 to 0xFF):
     - Compute hypothetical intermediate value
     - Predict power consumption (Hamming weight)

3. **Correlation Phase:**
   - For each key hypothesis:
     - Calculate correlation between predicted and actual power
     - Key with highest correlation is the correct key

**Example: AES First Round Attack**

```python
# DPA attack on AES (simplified)

def dpa_attack_aes_byte(traces, plaintexts, byte_position):
    """
    Attack one byte of the AES key

    traces: Power traces (10000 x 1000 samples)
    plaintexts: Corresponding plaintexts
    byte_position: Which key byte to attack (0-15)
    """
    max_correlation = 0
    best_key_guess = 0

    # Try all 256 possible key bytes
    for key_guess in range(256):
        # Compute hypothetical intermediate values
        hypothetical_values = []
        for pt in plaintexts:
            # AES first round: SubBytes(plaintext XOR key)
            intermediate = AES_SBOX[pt[byte_position] ^ key_guess]
            hypothetical_values.append(intermediate)

        # Predict power consumption (Hamming weight model)
        predicted_power = [hamming_weight(v) for v in hypothetical_values]

        # Correlate with actual power traces at each time point
        for time_point in range(1000):
            actual_power = [trace[time_point] for trace in traces]
            corr = correlation(predicted_power, actual_power)

            if abs(corr) > max_correlation:
                max_correlation = abs(corr)
                best_key_guess = key_guess

    return best_key_guess
```

**Effectiveness:**
- Can break AES-128 with ~10,000 traces
- Can break AES-256 with ~100,000 traces
- Works even with noisy measurements
- Very practical attack!

### 2.4 Correlation Power Analysis (CPA)

**Improvement over DPA:** Uses Pearson correlation coefficient for more robust analysis.

**Correlation Coefficient:**
```
        Σ[(predicted[i] - mean_predicted) * (measured[i] - mean_measured)]
r = ────────────────────────────────────────────────────────────────────────
    sqrt(Σ(predicted[i] - mean_predicted)²) * sqrt(Σ(measured[i] - mean_measured)²)
```

**Interpretation:**
- `r = 1.0`: Perfect positive correlation (correct key!)
- `r = 0.0`: No correlation
- `r = -1.0`: Perfect negative correlation

**Example Result:**
```
Key Guess    Correlation Coefficient
─────────────────────────────────────
0x00         0.05   │░
0x01         0.03   │░
...
0x3A         0.87   │████████████████████▌  ← Correct key!
0x3B         0.04   │░
...
0xFF         0.02   │░
```

---

## 3. Timing Attacks

### 3.1 Principle

**Key Idea:** Execution time depends on secret data.

**Why Timing Varies:**
1. Secret-dependent conditional branches
2. Secret-dependent loop iterations
3. Secret-dependent memory access patterns
4. Data-dependent CPU instruction timing

### 3.2 Classic Example: Modular Exponentiation

```c
// Insecure: Timing leaks key bits
uint32_t mod_exp(uint32_t base, uint32_t exp, uint32_t mod)
{
    uint32_t result = 1;
    while (exp > 0) {
        if (exp & 1) {
            result = (result * base) % mod;  // Slow operation
        }
        base = (base * base) % mod;
        exp >>= 1;
    }
    return result;
}
```

**Timing Profile:**
```
Exponent bit = 0: ~100 cycles (no multiplication)
Exponent bit = 1: ~150 cycles (with multiplication)

Attacker measures total time → Determines number of 1-bits → Narrows key space
```

### 3.3 Real-World Timing Attack: RSA

**OpenSSL RSA Timing Attack (Brumley & Boneh, 2003):**

- Targeted OpenSSL's RSA implementation
- Used Chinese Remainder Theorem (CRT) optimization
- Exploited cache timing differences
- Extracted 1024-bit RSA private key over network!
- Required ~1 million timing measurements

**Attack Scenario:**
```
Attacker                          Server (OpenSSL)
   │                                    │
   ├───────── Ciphertext C ────────────→│
   │                                    │ RSA_decrypt(C, private_key)
   │                                    │ Time: T
   │←────── Response + Time T ──────────┤
   │                                    │
   │ (Repeat 1M times with different C) │
   │                                    │
   └──→ Statistical analysis → Extract private key
```

### 3.4 Password Comparison Timing Attack

```c
// Vulnerable: Early exit reveals position
bool check_password(const char *input, const char *correct)
{
    for (int i = 0; i < PASSWORD_LEN; i++) {
        if (input[i] != correct[i]) {
            return false;  // ❌ Timing reveals first wrong byte!
        }
    }
    return true;
}
```

**Attack:**
```python
def timing_attack_password():
    password = bytearray(16)

    for pos in range(16):
        best_time = 0
        best_char = 0

        for guess in range(256):
            password[pos] = guess
            time = measure_check_time(password)

            if time > best_time:  # Longer time = went further in loop
                best_time = time
                best_char = guess

        password[pos] = best_char  # Lock in this byte

    return password
```

**Result:** Password cracked in `16 * 256 = 4096` attempts instead of `256^16` brute force!

### 3.5 Network Timing Attacks

Even over network, timing attacks can work:

**Challenge:** Network jitter adds noise
**Solution:** Take many measurements and use statistical methods

**Example: Remote timing attack on AES**

```python
import statistics

def network_timing_attack(host, port):
    timings = []

    # Collect many samples
    for _ in range(10000):
        start = time.time()
        send_request(host, port, plaintext)
        end = time.time()
        timings.append(end - start)

    # Use median (robust to outliers)
    return statistics.median(timings)
```

---

## 4. Electromagnetic (EM) Analysis

### 4.1 Principle

**Physics:** Current flow in conductors generates electromagnetic fields.

**EM Radiation Sources:**
- CPU data bus
- Address bus
- Crypto accelerator
- Power supply lines

**Advantage:** Non-invasive, can measure without physical contact.

### 4.2 EM Measurement Setup

```
┌──────────────────────────────────────────────────┐
│                                                   │
│   ┌────────────┐        ┌───────────────┐       │
│   │ EM Probe   │───────→│ Low-Noise Amp │       │
│   │ (H-field)  │        │  (40 dB gain) │       │
│   └─────┬──────┘        └───────┬───────┘       │
│         │                       │                │
│         │   ┌───────────────────┴──────┐        │
│         │   │  Oscilloscope (1 GS/s)   │        │
│         │   │  - Trigger on crypto op  │        │
│         │   │  - Record EM waveform    │        │
│         │   └──────────────────────────┘        │
│         │                                        │
│         ↓                                        │
│   ┌─────────────┐                               │
│   │   Target    │ STM32U545                     │
│   │   Device    │ (running AES)                 │
│   └─────────────┘                               │
│                                                   │
└──────────────────────────────────────────────────┘
```

### 4.3 EM Traces vs. Power Traces

**Similarity:** EM and power traces are highly correlated

```
Power Trace:
  ┌──┐    ┌──┐         ┌─┐
──┘  └────┘  └─────────┘ └──

EM Trace (H-field):
  ┌──┐    ┌──┐         ┌─┐
──┘  └────┘  └─────────┘ └──
(Nearly identical!)
```

**Advantage of EM:**
- Spatially localized (can target specific chip areas)
- Can measure through plastic enclosures
- Less affected by power supply noise

### 4.4 Localized EM Probing

```
        EM Probe
           │
           ↓
    ┌──────────────┐
    │ □□□□□□□□□□□□ │  MCU Die
    │ □□[AES]□□□□□ │  (can target specific blocks)
    │ □□□□□□[CPU]□ │
    │ □□□[RAM]□□□□ │
    └──────────────┘
```

**Attack Strategy:**
1. Position probe over AES hardware accelerator
2. Filter out other noise sources
3. Higher signal-to-noise ratio
4. Fewer traces needed for attack

---

## 5. Cache Timing Attacks

### 5.1 Cache Basics

**Cache Memory:**
- Small, fast memory close to CPU
- Stores frequently accessed data
- Cache hit: Data found (fast, ~1 cycle)
- Cache miss: Data not found (slow, ~100 cycles)

**Timing Difference:**
```
Access cached data:     ~10 ns
Access uncached data:   ~100 ns  ← 10x difference!
```

### 5.2 AES T-Table Cache Attack

**AES T-Table Implementation:**

```c
// Common AES implementation using lookup tables
uint32_t T0[256], T1[256], T2[256], T3[256];  // 4 KB total

void aes_encrypt_round(uint8_t *state, uint32_t *round_key)
{
    uint32_t s0, s1, s2, s3;

    // Table lookups indexed by state bytes
    s0 = T0[state[0]] ^ T1[state[5]] ^ T2[state[10]] ^ T3[state[15]] ^ round_key[0];
    s1 = T0[state[4]] ^ T1[state[9]] ^ T2[state[14]] ^ T3[state[3]]  ^ round_key[1];
    // ...
}
```

**Attack Idea:**
1. Table index depends on secret key
2. Index determines which cache line is loaded
3. Measure access time to infer which cache lines used
4. Deduce secret key from cache access pattern

**Attack Types:**

**Prime + Probe:**
1. Attacker fills cache with known data (Prime)
2. Victim performs AES encryption
3. Attacker checks which cache lines were evicted (Probe)
4. Evicted lines reveal accessed table entries → Key

**Flush + Reload:**
1. Attacker flushes T-table from cache
2. Victim performs AES encryption (loads some T-table entries)
3. Attacker times access to all T-table entries
4. Fast access = was loaded by victim → Reveals index → Key

### 5.3 Spectre/Meltdown (Bonus)

**Spectre/Meltdown Concept:**
- Exploit speculative execution
- CPU speculatively executes code before checking permissions
- Speculative execution leaves traces in cache
- Attacker uses cache timing to extract data

**Embedded Impact:**
- Less relevant for Cortex-M (in-order execution, no speculation)
- Relevant for Cortex-A (out-of-order, speculative)

---

## 6. Countermeasures

### 6.1 Countermeasure Categories

| Category | Technique | Overhead | Effectiveness |
|----------|-----------|----------|---------------|
| **Hiding** | Noise generation, random delays | Medium | Low-Medium |
| **Masking** | Randomize intermediate values | High | High |
| **Algorithm Design** | Constant-time operations | Low-Medium | Very High |
| **Hardware** | Dual-rail logic, shielding | Very High | Very High |

### 6.2 Hiding Techniques

#### 6.2.1 Random Delays

```c
// Add random delays to make timing unpredictable
void crypto_with_random_delay(void)
{
    uint32_t delay = get_random() % 1000;  // 0-999 cycles
    for (volatile uint32_t i = 0; i < delay; i++) {
        __NOP();
    }

    perform_crypto_operation();
}
```

**Effectiveness:** Low to medium (increases measurement noise)

#### 6.2.2 Power Noise Generation

```c
// Generate power consumption noise during crypto
void crypto_with_noise(void)
{
    // Start dummy operations on another peripheral
    start_dummy_adc_conversions();
    start_dummy_timer_pwm();

    perform_crypto_operation();

    stop_dummy_operations();
}
```

**Effectiveness:** Medium (requires more traces for DPA)

### 6.3 Masking

**Principle:** Randomize all intermediate values so they're uncorrelated with secrets.

**Boolean Masking:**
```c
// Instead of: y = AES_SBOX[x ^ key]
// Use masked version:

uint8_t mask = get_random();
uint8_t x_masked = x ^ mask;
uint8_t key_masked = key ^ mask;  // Same mask!
uint8_t y = AES_SBOX[x_masked ^ key_masked];
// Result is correct, but intermediate values are random!
```

**First-Order Masking:**
```c
// Masked AES S-Box lookup
uint8_t masked_sbox(uint8_t x, uint8_t mask_in, uint8_t *mask_out)
{
    uint8_t y_masked = AES_SBOX[x];  // Output is masked
    *mask_out = AES_SBOX[mask_in];   // Compute output mask
    return y_masked;
}
```

**Overhead:** 2-5x slower, more complex

### 6.4 Constant-Time Programming

**Principle:** Execution time independent of secret data.

**Rules:**
1. No secret-dependent branches
2. No secret-dependent memory access
3. No secret-dependent loop iterations
4. Use bitwise operations instead of conditionals

**Example: Constant-Time Comparison**

```c
// ❌ NOT constant time
bool compare_ct_wrong(const uint8_t *a, const uint8_t *b, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (a[i] != b[i]) {
            return false;  // Early exit! Timing leaks position
        }
    }
    return true;
}

// ✓ Constant time
bool compare_ct(const uint8_t *a, const uint8_t *b, size_t len)
{
    volatile uint8_t diff = 0;
    for (size_t i = 0; i < len; i++) {
        diff |= (a[i] ^ b[i]);  // No early exit, always processes all bytes
    }
    return (diff == 0);
}
```

**Example: Constant-Time Conditional Move**

```c
// ❌ NOT constant time
void conditional_move_wrong(uint32_t *dst, uint32_t *src, bool condition)
{
    if (condition) {
        *dst = *src;  // Branch! Timing leaks condition
    }
}

// ✓ Constant time (using bitwise operations)
void conditional_move_ct(uint32_t *dst, uint32_t *src, bool condition)
{
    uint32_t mask = -(uint32_t)condition;  // 0x00000000 or 0xFFFFFFFF
    *dst = (*dst & ~mask) | (*src & mask);  // No branch
}
```

### 6.5 Hardware Countermeasures

**STM32U5 Security Features:**

1. **Tamper Detection:**
   - External tamper pins
   - Temperature monitoring
   - Voltage monitoring
   - Action: Erase keys on tamper event

2. **True Random Number Generator (TRNG):**
   - Hardware entropy source
   - For masking and random delays

3. **AES Hardware Accelerator:**
   - Resistant to timing attacks (fixed latency)
   - Can include countermeasures (masking)

4. **Secure Boot and RDP:**
   - Read Protection Level 2 prevents debug access
   - Secure boot prevents firmware modification

---

## 7. Constant-Time Programming

### 7.1 Constant-Time Principles

**Goal:** Execution time depends only on data size, not data values.

**Key Techniques:**

1. **No Data-Dependent Branches**
```c
// ❌ BAD
if (secret_key & 0x01) {
    do_operation_a();
} else {
    do_operation_b();
}

// ✓ GOOD
bool bit = secret_key & 0x01;
uint32_t mask = -(uint32_t)bit;
result = (operation_a() & mask) | (operation_b() & ~mask);
```

2. **No Data-Dependent Memory Access**
```c
// ❌ BAD
uint8_t result = lookup_table[secret_index];  // Cache timing leak!

// ✓ GOOD (if table is small enough)
uint8_t result = 0;
for (int i = 0; i < TABLE_SIZE; i++) {
    uint8_t mask = -(uint8_t)(i == secret_index);
    result |= (lookup_table[i] & mask);
}
// Always accesses all table entries (no cache leak)
```

3. **Use volatile to Prevent Compiler Optimization**
```c
volatile uint8_t diff = 0;  // Prevent compiler from optimizing away
```

### 7.2 Example: Constant-Time AES S-Box

```c
/**
 * Constant-time AES S-Box using bitslicing
 * (No table lookups, no branches)
 */
uint8_t aes_sbox_ct(uint8_t x)
{
    // This is a complex bitsliced implementation
    // Each operation is constant-time (no lookups, no branches)

    // Galois field inversion in GF(2^8)
    uint8_t y = x;
    y = gf_mul(y, y);   // y^2
    uint8_t z = gf_mul(y, y);   // y^4
    z = gf_mul(z, z);   // y^8
    z = gf_mul(z, y);   // y^10
    z = gf_mul(z, z);   // y^20
    z = gf_mul(z, z);   // y^40
    z = gf_mul(z, y);   // y^42
    z = gf_mul(z, z);   // y^84
    z = gf_mul(z, y);   // y^86
    z = gf_mul(z, z);   // y^172
    z = gf_mul(z, y);   // y^174
    z = gf_mul(z, z);   // y^254 (multiplicative inverse)

    // Affine transformation
    uint8_t result = z ^ rotate_left(z, 1) ^ rotate_left(z, 2) ^
                     rotate_left(z, 3) ^ rotate_left(z, 4) ^ 0x63;

    return result;
}
```

**Performance:** ~10x slower than table-based, but constant time!

### 7.3 Verifying Constant-Time Code

**Tools:**

1. **ctgrind (Valgrind extension):**
   - Tracks which variables are secret
   - Warns about secret-dependent branches

2. **dudect (Constant-Time Testing):**
   - Statistical timing measurement
   - Compares execution time distributions

**Example: dudect usage**

```c
#include "dudect.h"

// Function to test
uint8_t compare_secret(uint8_t *a, uint8_t *b)
{
    return compare_ct(a, b, 16);
}

int main()
{
    dudect_ctx_t ctx;
    dudect_init(&ctx, compare_secret);

    // Run many iterations
    for (int i = 0; i < 1000000; i++) {
        dudect_run_test(&ctx);
    }

    // Check if timing is constant
    if (dudect_is_constant_time(&ctx)) {
        printf("✓ Function is constant-time\n");
    } else {
        printf("❌ Function leaks timing information!\n");
    }
}
```

---

## 8. Practical Examples

### 8.1 Secure Password Verification

```c
/**
 * Constant-time password/PIN verification
 *
 * Prevents timing attacks that could reveal correct password length
 * or character positions.
 */
bool verify_password_secure(const uint8_t *input, const uint8_t *stored,
                             size_t length)
{
    volatile uint8_t diff = 0;

    // Always compare full length
    for (size_t i = 0; i < length; i++) {
        diff |= (input[i] ^ stored[i]);
    }

    // Constant-time check if diff == 0
    // (0 - 0) = 0x00000000
    // (0 - 1) = 0xFFFFFFFF
    uint32_t is_zero = ((uint32_t)diff - 1) >> 31;

    return (bool)is_zero;
}
```

### 8.2 Secure HMAC Comparison

```c
/**
 * Constant-time HMAC comparison for preventing timing attacks
 * on MAC verification
 */
bool hmac_verify_secure(const uint8_t *computed_mac,
                        const uint8_t *received_mac,
                        size_t mac_len)
{
    if (mac_len > 64) return false;  // Sanity check

    volatile uint8_t diff = 0;

    // Compare all bytes
    for (size_t i = 0; i < mac_len; i++) {
        diff |= (computed_mac[i] ^ received_mac[i]);
    }

    // Add noise to make analysis harder (optional)
    uint32_t random_delay = get_random() & 0x1F;  // 0-31 cycles
    for (volatile uint32_t i = 0; i < random_delay; i++) {
        __NOP();
    }

    return (diff == 0);
}
```

### 8.3 Secure Key Comparison (TF-M Usage)

```c
/**
 * Compare two keys in constant time
 * Used in TF-M for key handle validation
 */
int32_t tfm_crypto_key_compare(const uint8_t *key1,
                                const uint8_t *key2,
                                size_t key_len)
{
    volatile uint8_t diff = 0;

    for (size_t i = 0; i < key_len; i++) {
        diff |= (key1[i] ^ key2[i]);
    }

    // Return 0 if equal, non-zero otherwise
    return diff;
}
```

---

## 📊 Summary

### Key Takeaways

1. **Side-Channel Attacks Are Practical**
   - Don't require expensive equipment
   - Can break strong cryptography (AES, RSA)
   - Must be considered in secure systems

2. **Power Analysis Is Very Effective**
   - DPA can break AES with ~10,000 traces
   - SPA can extract RSA keys directly
   - Requires oscilloscope + statistical analysis

3. **Timing Attacks Are Easy to Mount**
   - Can be done remotely over network
   - Exploit secret-dependent execution time
   - Defeated by constant-time programming

4. **Constant-Time Programming Is Essential**
   - No secret-dependent branches
   - No secret-dependent memory access
   - Use bitwise operations, not conditionals

5. **Defense in Depth**
   - Combine multiple countermeasures
   - Algorithm-level (constant-time code)
   - Hardware-level (TRNG, tamper detection)
   - System-level (TrustZone isolation)

### TF-M Protections

| Attack Type | TF-M Countermeasure |
|-------------|---------------------|
| Power Analysis | Hardware crypto accelerator (constant time) |
| Timing Attack | Constant-time PSA Crypto implementation |
| EM Analysis | Secure enclosure (system-level) |
| Cache Timing | Cortex-M has no data cache (immune) |
| Debug Access | RDP Level 2, secure debug |

### Next Module

**Module 03: Fault Injection Attacks**
- Voltage/clock glitching techniques
- Laser and EM fault injection
- Redundant check countermeasures
- Glitch detection circuits

**Lab 26: Side-Channel Resistance** (Section 6)
- Implement constant-time crypto functions
- Measure power consumption with oscilloscope
- Verify constant-time behavior
- Add random delays and masking

---

## 📚 References

1. **"Power Analysis Attacks"** - Mangard, Oswald, Popp (Springer)
2. **"Timing Analysis of Keystrokes and Timing Attacks on SSH"** - Song, Wagner, Tian
3. **"Remote Timing Attacks Are Practical"** - Brumley, Boneh (USENIX Security 2003)
4. **"AES Cache-Timing Attacks in Virtualized Environments"** - Gullasch et al.
5. **PSA Certified Level 2 Requirements** - Side-channel resistance guidelines
6. **ARM TrustZone for Armv8-M Security Whitepaper**
7. **"Constant-Time Cryptography"** - https://bearssl.org/ctmul.html

---

**End of Module 02**

Continue to [Module 03: Fault Injection Attacks](03_theory_fault_injection.md) →

# Section 6: Security & Attack Resistance

## Module 01: Attack Vectors and Threat Modeling

**Learning Objectives:**
- Understand common attack vectors against embedded systems
- Learn threat modeling for IoT devices
- Identify attack surfaces in TF-M-based systems
- Understand attacker capabilities and motivations

---

## 📋 Table of Contents

1. [Introduction to Embedded Security Threats](#1-introduction)
2. [Attack Surface Analysis](#2-attack-surface-analysis)
3. [Common Attack Vectors](#3-common-attack-vectors)
4. [Threat Modeling](#4-threat-modeling)
5. [Attacker Profiles](#5-attacker-profiles)
6. [TF-M Attack Surface](#6-tfm-attack-surface)
7. [Real-World Examples](#7-real-world-examples)

---

## 1. Introduction to Embedded Security Threats

### 1.1 Why Embedded Devices Are Targeted

Embedded devices and IoT systems face unique security challenges:

**Physical Access:**
- Attackers can physically access deployed devices
- Devices may be unattended for long periods
- Tamper-evident packaging is often impractical

**Resource Constraints:**
- Limited CPU, memory, power
- Difficult to implement complex security
- Cannot always apply patches/updates

**Long Lifecycle:**
- Devices deployed for 10+ years
- Crypto algorithms may become obsolete
- Cannot easily recall deployed devices

**High-Value Targets:**
- Medical devices (pacemakers, insulin pumps)
- Industrial control systems
- Automotive systems
- Smart home devices with cameras/microphones

### 1.2 Security Threat Landscape

```
┌─────────────────────────────────────────────────────────────┐
│                  Embedded Security Threats                   │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  Software Attacks          Hardware Attacks    Side-Channel │
│  ┌─────────────────┐      ┌──────────────┐    ┌──────────┐ │
│  │ Buffer overflow │      │ Fault inject.│    │ Power    │ │
│  │ Code injection  │      │ Probing      │    │ Timing   │ │
│  │ Privilege esc.  │      │ Glitching    │    │ EM       │ │
│  │ Replay attacks  │      │ Decapping    │    │ Cache    │ │
│  └─────────────────┘      └──────────────┘    └──────────┘ │
│                                                              │
│  Network Attacks          Physical Access     Supply Chain  │
│  ┌─────────────────┐      ┌──────────────┐    ┌──────────┐ │
│  │ MITM            │      │ Debug ports  │    │ Malware  │ │
│  │ DoS/DDoS        │      │ JTAG/SWD     │    │ Backdoors│ │
│  │ Eavesdropping   │      │ Flash dump   │    │ Trojans  │ │
│  │ Spoofing        │      │ Theft        │    │ Counter. │ │
│  └─────────────────┘      └──────────────┘    └──────────┘ │
└─────────────────────────────────────────────────────────────┘
```

---

## 2. Attack Surface Analysis

### 2.1 What is Attack Surface?

The **attack surface** is the sum of all possible entry points where an attacker can attempt to gain unauthorized access or cause damage.

**Components of Attack Surface:**

1. **Input Vectors** - Where data enters the system
   - UART, SPI, I2C, USB, Ethernet, WiFi, BLE
   - Sensor inputs
   - Debug interfaces

2. **Output Vectors** - Where data leaves the system
   - Display, LEDs, speakers
   - Network transmissions
   - Debug output

3. **Stored Data** - Persistent data at rest
   - Flash memory
   - EEPROM
   - SD cards
   - Cloud storage

4. **Code Execution** - Where code runs
   - Bootloader
   - RTOS/OS
   - Applications
   - Libraries

5. **Interfaces** - Communication channels
   - APIs
   - Command interfaces
   - Network protocols

### 2.2 Attack Surface Reduction

**Principle: Minimize the attack surface**

```c
// BAD: Debug UART left enabled in production
void init_system(void)
{
    init_debug_uart();  // ❌ Attack vector!
    enable_jtag();      // ❌ Attack vector!
    // ...
}

// GOOD: Disable debug interfaces in production
void init_system(void)
{
#ifdef DEBUG_BUILD
    init_debug_uart();  // Only in debug builds
    enable_jtag();
#else
    disable_debug_ports();  // ✓ Reduced attack surface
#endif
    // ...
}
```

### 2.3 Attack Surface in Layered Architecture

```
┌────────────────────────────────────────────────┐
│          Application Layer                     │
│  Attack Surface: Network APIs, File I/O       │ ← High
├────────────────────────────────────────────────┤
│          System Services                       │
│  Attack Surface: Service APIs, IPC            │ ← Medium
├────────────────────────────────────────────────┤
│          TF-M Secure Partition Manager         │
│  Attack Surface: PSA APIs only                │ ← Low
├────────────────────────────────────────────────┤
│          Hardware Abstraction Layer            │
│  Attack Surface: Hardware interfaces          │ ← Minimal
└────────────────────────────────────────────────┘
```

**TrustZone Benefit:** Isolates high-value assets in Secure world with minimal attack surface.

---

## 3. Common Attack Vectors

### 3.1 Software Attacks

#### 3.1.1 Buffer Overflow

**Description:** Writing beyond allocated buffer boundaries.

**Example Vulnerability:**
```c
// Vulnerable code
void process_command(char *input)
{
    char buffer[64];
    strcpy(buffer, input);  // ❌ No bounds checking!
    // Attacker sends 100-byte input → buffer overflow
}
```

**Attack Scenario:**
1. Attacker sends oversized input
2. Buffer overflow overwrites stack
3. Return address overwritten → code execution
4. Attacker gains control

**TF-M Mitigation:**
- Stack canaries (compiler option `-fstack-protector`)
- MPU enforces stack boundaries
- Address Space Layout Randomization (ASLR)

#### 3.1.2 Integer Overflow

**Vulnerability:**
```c
// Vulnerable: Integer overflow in size calculation
void allocate_buffer(uint32_t count, uint32_t size)
{
    uint32_t total = count * size;  // ❌ May overflow!
    uint8_t *buf = malloc(total);
    // If overflow: allocates small buffer but writes large data
}
```

**Attack:** Attacker provides `count=0x10000` and `size=0x10000`:
- `total = 0x10000 * 0x10000 = 0x100000000` → Overflows to `0`
- `malloc(0)` may succeed with tiny allocation
- Subsequent writes cause heap overflow

**Mitigation:**
```c
// Safe version
void allocate_buffer(uint32_t count, uint32_t size)
{
    if (count > 0 && size > (UINT32_MAX / count)) {
        return ERROR_OVERFLOW;  // ✓ Detect overflow
    }
    uint32_t total = count * size;
    // ...
}
```

#### 3.1.3 Use-After-Free

**Vulnerability:**
```c
void *ptr = malloc(100);
free(ptr);
// ... later ...
memcpy(ptr, data, 100);  // ❌ Use after free!
```

**Attack:** Attacker triggers reallocation of freed memory with controlled data.

**TF-M Mitigation:**
- Secure heap with guards
- Memory poisoning on free
- Pointer nullification

#### 3.1.4 Time-of-Check to Time-of-Use (TOCTOU)

**Vulnerability:**
```c
// Check
if (is_authorized(user_id)) {
    // ← Race condition window!
    // Use
    perform_privileged_operation(user_id);
}
```

**Attack:** Attacker changes `user_id` between check and use.

**Mitigation:**
```c
// Atomic check-and-use
uint32_t user_id_copy = user_id;
if (is_authorized(user_id_copy)) {
    perform_privileged_operation(user_id_copy);  // ✓ Use copy
}
```

### 3.2 Hardware Attacks

#### 3.2.1 Fault Injection Attacks

**Goal:** Cause controlled glitches to skip security checks.

**Techniques:**
1. **Voltage Glitching:** Briefly drop or spike VDD
2. **Clock Glitching:** Inject extra clock edges
3. **EM Fault Injection:** Use EM pulse to flip bits
4. **Laser Fault Injection:** Focused laser on die

**Example Attack:**
```c
// Security check
if (signature_valid) {
    // Attacker injects fault here to skip check
    allow_access();
}
```

**Fault Effect:**
```assembly
; Original code
CMP  R0, #1      ; Compare signature_valid
BEQ  allow_access ; Branch if equal

; With fault injection
CMP  R0, #1
NOP              ; Glitch replaces BEQ with NOP!
; Falls through to allow_access() even if R0 != 1
```

**Countermeasures:**
- Redundant checks (see Lab 27)
- Random delays
- Loop counters
- Double checks with inverse logic

#### 3.2.2 Debug Port Exploitation

**Attack Vector:** JTAG, SWD, or serial debug interfaces

**Attacker Actions:**
1. Connect to SWD/JTAG port
2. Halt CPU
3. Dump flash memory → Extract firmware and secrets
4. Modify memory → Bypass security
5. Single-step debug → Reverse engineer algorithms

**Mitigation:**
```c
// Lock debug access in production (STM32 example)
void lock_debug_access(void)
{
    // Set Read Protection Level 2 (permanent)
    FLASH_OBProgramInitTypeDef ob_config;
    ob_config.RDPLevel = OB_RDP_LEVEL_2;  // Irreversible!
    HAL_FLASHEx_OBProgram(&ob_config);

    // Debug ports are now permanently disabled
}
```

**Warning:** RDP Level 2 is **irreversible** - use carefully!

#### 3.2.3 Flash Memory Readout

**Attack:**
1. Desolder flash chip from PCB
2. Read with external programmer
3. Extract firmware and keys

**Mitigation:**
- On-chip flash only (no external SPI flash for secrets)
- Flash readout protection (RDP)
- Encrypted flash contents
- Key diversification (unique per device)

### 3.3 Side-Channel Attacks

#### 3.3.1 Power Analysis

**Simple Power Analysis (SPA):**
- Observe power consumption during crypto operations
- Different operations have different power signatures

**Example:**
```c
// Vulnerable: Secret-dependent conditional
if (secret_key_bit == 1) {
    do_multiplication();  // High power consumption
} else {
    do_addition();        // Low power consumption
}
// Attacker measures power → Learns secret key bit!
```

**Differential Power Analysis (DPA):**
- Statistical analysis of many power traces
- Correlate power with key hypotheses
- Extremely powerful attack

**Countermeasures:**
- Constant-time algorithms (Lab 26)
- Random masking
- Noise generation
- Dual-rail logic

#### 3.3.2 Timing Attacks

**Vulnerability:**
```c
// Insecure: Timing reveals secret
bool verify_password(uint8_t *input, uint8_t *secret, size_t len)
{
    for (int i = 0; i < len; i++) {
        if (input[i] != secret[i]) {
            return false;  // ❌ Early return reveals position!
        }
    }
    return true;
}
```

**Attack:** Measure execution time:
- Wrong at position 0: ~10 µs
- Wrong at position 1: ~20 µs
- Wrong at position 2: ~30 µs
- All correct: ~100 µs

→ Attacker can brute-force byte-by-byte!

**Constant-Time Fix:**
```c
// Secure: Constant time
bool verify_password_ct(uint8_t *input, uint8_t *secret, size_t len)
{
    volatile uint8_t diff = 0;
    for (int i = 0; i < len; i++) {
        diff |= (input[i] ^ secret[i]);  // ✓ No early exit
    }
    return (diff == 0);
}
```

#### 3.3.3 Electromagnetic (EM) Analysis

**Attack:**
- Place EM probe near CPU
- Capture EM emissions during crypto
- Similar to power analysis

**Countermeasures:**
- EM shielding (metal enclosure)
- Random operation ordering
- Balanced logic

---

## 4. Threat Modeling

### 4.1 Threat Modeling Process

**STRIDE Model** (Microsoft):

| Threat Type | Description | Example |
|-------------|-------------|---------|
| **S**poofing | Impersonating something/someone | Fake sensor data, replay attacks |
| **T**ampering | Modifying data or code | Flash modification, MITM |
| **R**epudiation | Claiming not to have done something | Missing audit logs |
| **I**nformation Disclosure | Exposing protected information | Memory dumps, side-channels |
| **D**enial of Service | Make system unavailable | Resource exhaustion, jamming |
| **E**levation of Privilege | Gain unauthorized capabilities | Exploit to gain root access |

### 4.2 Threat Modeling for TF-M Device

**Example: Secure GPS Tracker (Project 1)**

#### Step 1: Identify Assets

**Critical Assets:**
1. GPS location data
2. Device credentials (certificates, keys)
3. Firmware code
4. User data in storage

#### Step 2: Identify Entry Points

**Attack Entry Points:**
1. UART (AT commands to modem)
2. 4G network connection
3. GPS receiver
4. Debug port (SWD)
5. Firmware update mechanism

#### Step 3: Identify Threats (STRIDE)

| Component | Threat Type | Threat | Impact | Likelihood |
|-----------|-------------|--------|---------|------------|
| GPS | Spoofing | Fake GPS signals | High | Medium |
| 4G Modem | Tampering | MITM attack | High | Medium |
| Flash | Information Disclosure | Flash readout | Critical | Low |
| Firmware Update | Tampering | Malicious firmware | Critical | Low |
| Debug Port | Elevation of Privilege | Debug access | Critical | Medium |

#### Step 4: Countermeasures

| Threat | Countermeasure | TF-M Feature |
|--------|----------------|--------------|
| GPS spoofing | Multi-source validation | Attestation + sensor fusion |
| MITM | TLS 1.3 with mutual auth | PSA Crypto (TLS) |
| Flash readout | Read protection | RDP Level 2, encrypted storage |
| Malicious firmware | Signature verification | MCUboot + attestation |
| Debug access | Lock debug ports | Secure debug, RDP |

### 4.3 Attack Trees

**Example: Extracting Device Keys**

```
Goal: Extract Device Private Keys
│
├─── [OR] Physical Access
│    ├─── [AND] Open Device
│    │    ├─── Remove enclosure
│    │    └─── Access PCB
│    ├─── [OR] Read Flash
│    │    ├─── Debug port (SWD)   ← Countermeasure: Disable SWD
│    │    ├─── Desolder flash     ← Countermeasure: On-chip only
│    │    └─── Glitch attack      ← Countermeasure: Redundant checks
│    └─── [OR] Side-Channel
│         ├─── Power analysis     ← Countermeasure: Constant-time
│         └─── EM analysis        ← Countermeasure: Shielding
│
└─── [OR] Remote Access
     ├─── Exploit firmware bug    ← Countermeasure: Secure coding
     ├─── MITM attack             ← Countermeasure: TLS + certs
     └─── Supply chain attack     ← Countermeasure: Secure boot
```

---

## 5. Attacker Profiles

### 5.1 Attacker Capability Levels

**Level 1: Script Kiddie**
- Tools: Public exploits, automated scanners
- Skills: Low
- Motivation: Curiosity, vandalism
- Budget: $0 - $100
- Threat: Low to medium

**Level 2: Skilled Individual**
- Tools: Custom tools, debuggers, oscilloscopes
- Skills: Medium to high
- Motivation: Financial gain, espionage
- Budget: $1,000 - $10,000
- Threat: Medium to high

**Level 3: Professional Team**
- Tools: Expensive equipment (FIB, laser station, EM probes)
- Skills: Expert
- Motivation: Corporate espionage, nation-state
- Budget: $100,000+
- Threat: Very high

**Level 4: Nation-State**
- Tools: Unlimited resources, zero-day exploits
- Skills: World-class experts
- Motivation: Strategic intelligence, warfare
- Budget: Millions
- Threat: Extreme

### 5.2 Attack Cost vs. Asset Value

```
Asset Value                 Defense Requirement
─────────────────────────────────────────────────────
Low ($0-$1000)        →    Basic defenses
Medium ($1K-$100K)    →    Standard TF-M + RDP
High ($100K-$1M)      →    TF-M + tamper detection
Critical ($1M+)       →    TF-M + physical security + monitoring
```

**Principle:** Defense cost should be proportional to asset value.

---

## 6. TF-M Attack Surface

### 6.1 TF-M Architecture from Security Perspective

```
┌──────────────────────────────────────────────────────────┐
│                   Non-Secure World                        │
│   Attack Surface: Large (network, peripherals, apps)     │
│                                                           │
│   ┌──────────────────────────────────────────┐           │
│   │ Non-Secure Application Code              │           │
│   │ - High attack surface                    │           │
│   │ - Can be compromised                     │           │
│   └──────────────────────────────────────────┘           │
│           ↓ (Limited PSA API calls only)                 │
└───────────┼──────────────────────────────────────────────┘
            ↓
┌───────────┼──────────────────────────────────────────────┐
│           ↓   Secure World (TrustZone Protected)         │
│   Attack Surface: Minimal (only PSA APIs)                │
│                                                           │
│   ┌──────────────────────────────────────────┐           │
│   │ Secure Partition Manager (SPM)           │           │
│   │ - Validates all NS requests              │           │
│   │ - Enforces isolation                     │           │
│   └──────────────────────────────────────────┘           │
│   ┌──────────────────────────────────────────┐           │
│   │ Crypto Service │ Storage │ Attestation   │           │
│   │ - Sandboxed partitions                   │           │
│   │ - Cannot directly access each other      │           │
│   └──────────────────────────────────────────┘           │
│           ↓ (Hardware abstraction only)                  │
└───────────┼──────────────────────────────────────────────┘
            ↓
      [Hardware: Flash, Crypto, Keys]
```

### 6.2 TF-M Security Boundaries

**1. TrustZone Boundary (HW enforced)**
- SAU/IDAU prevent NS from accessing Secure memory
- MPU enforces additional boundaries
- NVIC routes secure interrupts

**2. Partition Boundaries (SW/HW enforced)**
- Each secure partition isolated
- IPC communication only
- SPM mediates all access

**3. API Surface**
- PSA APIs are the **only** entry points
- Well-defined, reviewed interfaces
- Input validation at boundary

### 6.3 Attack Paths in TF-M System

**Blocked Attack Paths:**
```
NS App → Direct access to Secure RAM       ❌ Blocked by SAU
NS App → Direct call to crypto function    ❌ Blocked by TrustZone
NS App → Access another partition's data   ❌ Blocked by SPM
```

**Allowed Attack Paths (must be defended):**
```
NS App → PSA Crypto API                    ✓ Allowed (validated)
NS App → PSA Storage API                   ✓ Allowed (validated)
NS App → Network (external)                ✓ Allowed (app responsibility)
```

---

## 7. Real-World Examples

### 7.1 Case Study: Automotive ECU Attack

**Target:** Engine Control Unit (ECU)

**Attack:**
1. Attacker connects to OBD-II port
2. Sends crafted CAN messages
3. Exploits buffer overflow in message handler
4. Gains code execution
5. Modifies engine parameters or disables safety features

**TF-M Defense:**
- CAN handler in Non-Secure world
- Critical engine parameters in Secure storage
- Attestation verifies ECU integrity
- Even if NS compromised, cannot modify Secure data

### 7.2 Case Study: Smart Meter Key Extraction

**Target:** Smart electricity meter

**Attack:**
1. Physical access to meter
2. Connect to debug port (not disabled)
3. Dump flash memory
4. Extract AES key used for billing encryption
5. Clone meter to avoid charges

**TF-M Defense:**
- Debug ports locked (RDP Level 2)
- AES key in Secure storage (ITS)
- Key is hardware-bound (cannot extract)
- Even with debug access, cannot read Secure flash

### 7.3 Case Study: Medical Device Firmware Modification

**Target:** Insulin pump

**Attack:**
1. Wireless connection to pump
2. Exploit vulnerability in update mechanism
3. Install malicious firmware
4. Deliver lethal insulin dose remotely

**TF-M Defense:**
- MCUboot verifies firmware signatures
- Only signed firmware from manufacturer accepted
- Attestation proves firmware authenticity
- Cannot install unsigned firmware

---

## 📊 Summary

### Key Takeaways

1. **Attack Surface Reduction:** Minimize entry points to minimize risk
2. **Defense in Depth:** Multiple layers of security
3. **Threat Modeling:** Systematic identification of threats
4. **TrustZone Isolation:** Critical assets protected even if NS compromised
5. **Physical Security:** Must be considered for high-value assets

### TF-M Security Principles

| Principle | Description | TF-M Implementation |
|-----------|-------------|---------------------|
| Least Privilege | Minimal permissions | Isolation Level 2/3, MPU |
| Compartmentalization | Separate components | Secure partitions |
| Secure by Default | Secure configuration out-of-box | Profile_medium+, RDP |
| Defense in Depth | Multiple security layers | TrustZone + MPU + crypto |
| Fail Securely | Errors don't compromise security | Secure error handling |

### Next Steps

- **Module 02:** Side-Channel Attacks in detail
- **Module 03:** Fault Injection techniques and countermeasures
- **Module 04:** Physical attack mitigations
- **Module 05:** TF-M specific countermeasures

- **Lab 26:** Implement side-channel resistant crypto
- **Lab 27:** Add fault injection countermeasures
- **Lab 28:** Configure physical attack mitigations
- **Lab 29:** Secure debug implementation
- **Lab 30:** Complete security audit

---

## 📚 References

1. **OWASP IoT Top 10:** https://owasp.org/www-project-internet-of-things/
2. **STRIDE Threat Modeling:** Microsoft Security Development Lifecycle
3. **Common Weakness Enumeration (CWE):** https://cwe.mitre.org/
4. **PSA Certified Security Model:** https://www.psacertified.org/
5. **ARM TrustZone Security Whitepaper**
6. **NIST Cybersecurity Framework**

---

**End of Module 01**

Continue to [Module 02: Side-Channel Attacks](02_theory_side_channel_attacks.md) →

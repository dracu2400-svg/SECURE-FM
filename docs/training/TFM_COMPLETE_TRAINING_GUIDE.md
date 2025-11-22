# Trusted Firmware-M (TF-M) Complete Training Guide

## 📚 Training Overview

This comprehensive training guide takes you from TF-M basics to building real-world secure IoT products. The training is organized in a progressive manner, starting with fundamental concepts and building up to complex projects.

**Training Duration:** 5-7 days (40-50 hours)
**Target Audience:** Embedded engineers, security engineers, IoT developers
**Prerequisites:**
- Basic C programming
- Understanding of embedded systems
- Familiarity with ARM Cortex-M processors (helpful but not required)

---

## 📖 Table of Contents

### Part 1: Foundations (Day 1)
1. [Introduction to Secure IoT](#module-1-introduction-to-secure-iot)
2. [ARM TrustZone Technology](#module-2-arm-trustzone-technology)
3. [PSA (Platform Security Architecture)](#module-3-psa-platform-security-architecture)
4. [TF-M Architecture Overview](#module-4-tf-m-architecture-overview)
5. [Development Environment Setup](#module-5-development-environment-setup)

### Part 2: Core Concepts (Day 2)
6. [Secure Partition Manager (SPM)](#module-6-secure-partition-manager-spm)
7. [Isolation Levels and Security Models](#module-7-isolation-levels-and-security-models)
8. [PSA APIs - Client and Service](#module-8-psa-apis-client-and-service)
9. [Build System and Configuration](#module-9-build-system-and-configuration)
10. [First TF-M Application](#module-10-first-tf-m-application)

### Part 3: Secure Services (Day 3)
11. [Cryptographic Services](#module-11-cryptographic-services)
12. [Secure Storage - ITS and PS](#module-12-secure-storage-its-and-ps)
13. [Initial Attestation](#module-13-initial-attestation)
14. [Platform Services](#module-14-platform-services)
15. [Firmware Update Service](#module-15-firmware-update-service)

### Part 4: Secure Boot and MCUboot (Day 4)
16. [Secure Boot Architecture](#module-16-secure-boot-architecture)
17. [MCUboot Deep Dive](#module-17-mcuboot-deep-dive)
18. [Image Signing and Verification](#module-18-image-signing-and-verification)
19. [Rollback Protection](#module-19-rollback-protection)
20. [Encrypted Firmware Images](#module-20-encrypted-firmware-images)

### Part 5: Advanced Topics (Day 5)
21. [Porting TF-M to New Hardware](#module-21-porting-tf-m-to-new-hardware)
22. [Creating Custom Secure Partitions](#module-22-creating-custom-secure-partitions)
23. [Multi-Core Configurations](#module-23-multi-core-configurations)
24. [Physical Attack Mitigation](#module-24-physical-attack-mitigation)
25. [Performance Optimization](#module-25-performance-optimization)

### Part 6: Real-World Projects (Days 6-7)
26. [Project 1: STM32U5 Secure Tracker](#project-1-stm32u5-secure-tracker)
27. [Project 2: NRF52840 Secure Tracker](#project-2-nrf52840-secure-tracker)

---

# PART 1: FOUNDATIONS

## Module 1: Introduction to Secure IoT

### 1.1 Why Security Matters in IoT

**The IoT Security Challenge:**
- Billions of connected devices
- Long device lifetimes (10+ years)
- Physical access by attackers
- Remote attacks over networks
- Supply chain vulnerabilities

**Real-World Security Breaches:**
```
Example: Mirai Botnet (2016)
- Compromised 600,000+ IoT devices
- Used default passwords
- Caused major internet outages
- Cost: $110+ million

Example: Medical Device Vulnerabilities
- Insulin pumps remotely controlled
- Pacemakers vulnerable to attacks
- Patient safety at risk
```

**Cost of Insecurity:**
- Device recalls
- Brand damage
- Legal liability
- Loss of customer trust
- Regulatory fines (GDPR, etc.)

### 1.2 Security Requirements for IoT Devices

**Core Security Principles:**

1. **Secure Boot**
   - Verify firmware authenticity
   - Prevent unauthorized code execution
   - Establish root of trust

2. **Secure Storage**
   - Protect cryptographic keys
   - Store sensitive data encrypted
   - Prevent unauthorized access

3. **Cryptography**
   - Authenticate communications
   - Encrypt data in transit
   - Generate random numbers securely

4. **Attestation**
   - Prove device identity
   - Report security state
   - Enable remote verification

5. **Secure Update**
   - Deliver firmware updates safely
   - Verify update authenticity
   - Prevent rollback attacks

### 1.3 Introduction to TF-M

**What is TF-M?**

Trusted Firmware-M (TF-M) is an open-source reference implementation of the Platform Security Architecture (PSA) for ARM Cortex-M devices.

```
┌─────────────────────────────────────────┐
│         Non-Secure World (NSPE)         │
│  ┌──────────────────────────────────┐   │
│  │    Application Code              │   │
│  │  - Business logic                │   │
│  │  - Communication protocols       │   │
│  │  - User interfaces               │   │
│  └──────────────────────────────────┘   │
├─────────────────────────────────────────┤  ← TrustZone Boundary
│         Secure World (SPE)               │
│  ┌──────────────────────────────────┐   │
│  │    TF-M Core (SPM)               │   │
│  ├──────────────────────────────────┤   │
│  │  Secure Services (Partitions)    │   │
│  │  • Crypto                        │   │
│  │  • Secure Storage                │   │
│  │  • Attestation                   │   │
│  │  • Firmware Update               │   │
│  └──────────────────────────────────┘   │
└─────────────────────────────────────────┘
```

**Key Features:**
- ✅ PSA Certified Level 1, 2, 3 ready
- ✅ Small footprint (starts from ~40KB)
- ✅ Multiple isolation levels
- ✅ Portable across ARM Cortex-M devices
- ✅ Production-ready code
- ✅ Open source (BSD-3-Clause)

### 1.4 TF-M vs Other Security Solutions

| Feature | TF-M | Bare Metal | RTOS Only | TEE Solutions |
|---------|------|------------|-----------|---------------|
| Standard API | ✅ PSA | ❌ Custom | ⚠️ Varies | ⚠️ Varies |
| Certification | ✅ PSA Cert | ❌ No | ❌ No | ⚠️ Some |
| Footprint | ✅ Small | ✅ Smallest | ⚠️ Medium | ❌ Large |
| Isolation | ✅ Hardware | ❌ None | ⚠️ Software | ✅ Hardware |
| Portability | ✅ High | ❌ Low | ⚠️ Medium | ⚠️ Medium |
| Maintenance | ✅ Community | ❌ DIY | ⚠️ Vendor | ⚠️ Vendor |

**When to Use TF-M:**
- ✅ Need PSA certification
- ✅ Regulatory compliance required
- ✅ Long device lifetime
- ✅ High security requirements
- ✅ Resource-constrained devices

---

## Module 2: ARM TrustZone Technology

### 2.1 What is TrustZone?

ARM TrustZone is a hardware security technology that creates two virtual processors from a single physical core:
- **Secure World**: Trusted execution environment
- **Non-Secure World**: Normal application environment

**Simple Analogy:**
```
Think of TrustZone like a bank vault inside a building:

Building (Processor):
├── Public Area (Non-Secure World)
│   - Customer service
│   - Open to everyone
│   - Normal operations
│
└── Vault (Secure World)
    - Restricted access
    - Stores valuables (keys, secrets)
    - Protected by hardware
```

### 2.2 TrustZone for Cortex-M (v8-M Architecture)

**Hardware Components:**

1. **Security Attribution Unit (SAU)**
   ```
   Memory Map:

   0xFFFFFFFF  ┌─────────────────┐
               │  Non-Secure     │
   0x20000000  ├─────────────────┤ ← SAU Configuration
               │  Secure         │
   0x10000000  ├─────────────────┤
               │  Non-Secure     │
   0x00000000  └─────────────────┘
   ```

2. **Implementation Defined Attribution Unit (IDAU)**
   - Vendor-specific security configuration
   - Works with SAU
   - Cannot be overridden

3. **Memory Protection Unit (MPU)**
   - Separate MPUs for Secure and Non-Secure
   - Fine-grained memory protection
   - Access permissions per region

4. **Secure/Non-Secure Interrupts**
   - Interrupts can target either world
   - Secure interrupts have priority
   - NVIC security extension

### 2.3 Secure vs Non-Secure World

**Execution States:**

```c
// Non-Secure State (NS=1)
void non_secure_function(void) {
    // Can access:
    // ✅ Non-Secure memory
    // ✅ Non-Secure peripherals
    // ✅ Call Secure functions (via NSC)

    // Cannot access:
    // ❌ Secure memory
    // ❌ Secure peripherals
    // ❌ Secure code directly
}

// Secure State (NS=0)
void secure_function(void) {
    // Can access:
    // ✅ Secure memory
    // ✅ Secure peripherals
    // ✅ Non-Secure memory (if needed)
    // ✅ Non-Secure peripherals

    // Full system access
}
```

**Transition Rules:**
```
Non-Secure → Secure:
    Only through Secure Gateway (SG instruction)
    Entry points controlled by secure code

Secure → Non-Secure:
    Via BXNS/BLXNS instructions
    Return from secure function
    Secure handler calling NS callback
```

### 2.4 Secure Gateway and Veneers

**What is a Veneer?**

A veneer is a secure entry point that allows non-secure code to call secure functions safely.

```c
// Secure Code
// crypto.c (Secure)
__attribute__((cmse_nonsecure_entry))
int secure_crypto_operation(uint32_t* data, size_t len) {
    // This function can be called from Non-Secure world
    // Compiler generates SG instruction at entry

    // Validate NS pointers
    if (cmse_check_address_range(data, len, CMSE_NONSECURE) == NULL) {
        return -1;  // Invalid NS address
    }

    // Perform secure operation
    return perform_crypto(data, len);
}
```

```c
// Non-Secure Code
// app.c (Non-Secure)
extern int secure_crypto_operation(uint32_t* data, size_t len);

void application_code(void) {
    uint32_t data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // Call into secure world
    int result = secure_crypto_operation(data, 10);

    // Execution returns here
}
```

**Veneer Execution Flow:**
```
1. NS code calls secure_crypto_operation
2. Branch to veneer in NSC region
3. SG instruction validates secure entry
4. Secure function executes
5. BXNS returns to Non-Secure
6. NS code continues
```

### 2.5 Memory Layout Example

**Typical TF-M Memory Configuration:**

```
Flash Memory (Code):
0x10100000  ┌──────────────────────┐
            │  Non-Secure Code     │  512 KB
            │  (Application)       │
0x10080000  ├──────────────────────┤
            │  Non-Secure Callable │  4 KB (Veneers)
            │  (NSC Region)        │
0x1007F000  ├──────────────────────┤
            │  Secure Code         │  508 KB
            │  (TF-M)              │
0x10000000  └──────────────────────┘

RAM Memory (Data):
0x20040000  ┌──────────────────────┐
            │  Non-Secure RAM      │  128 KB
            │  (App data/stack)    │
0x20020000  ├──────────────────────┤
            │  Secure RAM          │  128 KB
            │  (TF-M data/stack)   │
0x20000000  └──────────────────────┘
```

### 2.6 TrustZone Best Practices

**Security Principles:**

1. **Minimize Secure World Complexity**
   ```
   Good: Small, focused secure functions
   Bad:  Complex business logic in secure world
   ```

2. **Validate All Non-Secure Inputs**
   ```c
   // Always check NS pointers
   if (cmse_check_address_range(ns_ptr, size,
                                 CMSE_NONSECURE | CMSE_MPU_READ) == NULL) {
       return ERROR_INVALID_PARAMETER;
   }
   ```

3. **Clear Secrets on Exit**
   ```c
   // Clear secure registers when returning to NS
   __asm volatile(
       "mov r0, #0\n"
       "mov r1, #0\n"
       "mov r2, #0\n"
       "mov r3, #0\n"
   );
   ```

4. **Minimize Secure Attack Surface**
   ```
   Fewer entry points = Smaller attack surface
   Each veneer must be carefully validated
   ```

---

## Module 3: PSA (Platform Security Architecture)

### 3.1 What is PSA?

**Platform Security Architecture (PSA)** is a holistic set of threat models, security analysis, hardware and firmware architecture specifications, and an open source firmware reference implementation for securing IoT devices.

**PSA Components:**

```
┌─────────────────────────────────────────────┐
│         PSA Ecosystem                        │
├─────────────────────────────────────────────┤
│  Threat Models & Security Analysis          │
│  - Common attack vectors                    │
│  - Risk assessment                          │
│  - Protection recommendations               │
├─────────────────────────────────────────────┤
│  Hardware Security Requirements             │
│  - Root of Trust                            │
│  - Isolation primitives                     │
│  - Cryptographic accelerators               │
├─────────────────────────────────────────────┤
│  Firmware Architecture                      │
│  - Secure boot                              │
│  - Secure services                          │
│  - Update mechanism                         │
├─────────────────────────────────────────────┤
│  Standard APIs                              │
│  - PSA Crypto API                           │
│  - PSA Storage API                          │
│  - PSA Attestation API                      │
│  - PSA Firmware Update API                  │
├─────────────────────────────────────────────┤
│  Certification Program                      │
│  - PSA Certified Level 1 (Software)         │
│  - PSA Certified Level 2 (SESIP)            │
│  - PSA Certified Level 3 (Attack Lab)       │
└─────────────────────────────────────────────┘
```

### 3.2 PSA Security Model

**Trust and Isolation:**

```
┌──────────────────────────────────────────────┐
│        Application Root of Trust (ARoT)      │
│  - Application-specific security services    │
│  - Isolation Level 2 or 3                    │
├──────────────────────────────────────────────┤
│        Platform Root of Trust (PRoT)         │
│  - Core security services                    │
│  - Isolation Level 3                         │
│  ┌────────────────────────────────────────┐  │
│  │  Secure Processing Environment (SPE)  │  │
│  │  - Crypto, Storage, Attestation        │  │
│  │  - Update, Platform services           │  │
│  └────────────────────────────────────────┘  │
├──────────────────────────────────────────────┤
│        Hardware Root of Trust                │
│  - Immutable boot code                       │
│  - Hardware unique key                       │
│  - Secure debug lifecycle                    │
└──────────────────────────────────────────────┘
```

**Security Lifecycle:**

```
Device States:

1. Assembly (CM_ASSEMBLY)
   - Manufacturing
   - Provisioning

2. Provisioned (CM_PROVISIONED)
   - Keys installed
   - Ready for deployment

3. Secured (CM_SECURED)
   - Production state
   - Security enabled
   - Normal operation

4. Decommissioned (CM_DECOMMISSIONED)
   - End of life
   - Keys erased
   - Debug locked
```

### 3.3 PSA Root of Trust (RoT) Services

**Core RoT Services:**

1. **Crypto Service**
   ```c
   // Standard PSA Crypto API
   psa_status_t psa_hash_compute(
       psa_algorithm_t alg,
       const uint8_t *input,
       size_t input_length,
       uint8_t *hash,
       size_t hash_size,
       size_t *hash_length
   );
   ```

2. **Internal Trusted Storage (ITS)**
   ```c
   // Store sensitive data
   psa_status_t psa_its_set(
       psa_storage_uid_t uid,
       size_t data_length,
       const void *p_data,
       psa_storage_create_flags_t create_flags
   );
   ```

3. **Initial Attestation**
   ```c
   // Get attestation token
   psa_status_t psa_initial_attest_get_token(
       const uint8_t *auth_challenge,
       size_t challenge_size,
       uint8_t *token_buf,
       size_t token_buf_size,
       size_t *token_size
   );
   ```

4. **Firmware Update**
   ```c
   // Install new firmware
   psa_status_t psa_fwu_write(
       psa_fwu_component_t component,
       size_t image_offset,
       const void *block,
       size_t block_size
   );
   ```

### 3.4 PSA APIs Overview

**API Categories:**

```c
// 1. PSA Crypto API (psa/crypto.h)
psa_crypto_init();                    // Initialize
psa_generate_random(buf, size);       // RNG
psa_hash_compute(...);                // Hashing
psa_cipher_encrypt(...);              // Symmetric encryption
psa_sign_message(...);                // Asymmetric signing
psa_key_derivation(...);              // Key derivation

// 2. PSA Storage API
// Internal Trusted Storage (psa/internal_trusted_storage.h)
psa_its_set(uid, len, data, flags);   // Store
psa_its_get(uid, offset, len, data);  // Retrieve
psa_its_remove(uid);                  // Delete

// Protected Storage (psa/protected_storage.h)
psa_ps_set(uid, len, data, flags);    // Store (encrypted)
psa_ps_get(uid, offset, len, data);   // Retrieve
psa_ps_remove(uid);                   // Delete

// 3. PSA Attestation API (psa/initial_attestation.h)
psa_initial_attest_get_token(...);    // Get token
psa_initial_attest_get_token_size(...); // Get size

// 4. PSA Firmware Update API (psa/update.h)
psa_fwu_write(...);                   // Write image
psa_fwu_install(...);                 // Install
psa_fwu_accept(...);                  // Accept update
psa_fwu_reject(...);                  // Reject update

// 5. PSA Client API (psa/client.h)
// For custom secure services
psa_connect(sid, version);            // Connect to service
psa_call(handle, type, in_vec, out_vec); // Call service
psa_close(handle);                    // Disconnect
```

### 3.5 PSA Certification Levels

**Level 1: Software Protection**
- Software-based security
- Standard PSA APIs
- Secure boot
- ~3-6 months to certify

**Level 2: Security Evaluation Standard for IoT Platforms (SESIP)**
- Hardware and software
- Side-channel protection
- Tamper resistance
- ~6-12 months to certify

**Level 3: Security Attack Lab Testing**
- Comprehensive testing
- Advanced attacks
- Highest assurance
- ~12-18 months to certify

---

## Module 4: TF-M Architecture Overview

### 4.1 High-Level Architecture

```
┌────────────────────────────────────────────────────────┐
│              Non-Secure Processing Environment         │
│                        (NSPE)                          │
│  ┌──────────────────────────────────────────────────┐  │
│  │  Non-Secure Application                          │  │
│  │  ┌────────────┬────────────┬───────────────────┐ │  │
│  │  │   RTOS     │  Protocol  │  Application      │ │  │
│  │  │ (Optional) │   Stack    │   Logic           │ │  │
│  │  └────────────┴────────────┴───────────────────┘ │  │
│  │                                                   │  │
│  │  PSA Client API Calls                           │  │
│  │  (crypto, storage, attestation, update...)      │  │
│  └──────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────┘
                          │
                          │ PSA Client API
                          │ (Veneers/NSC)
                          ▼
┌────────────────────────────────────────────────────────┐
│          Secure Processing Environment (SPE)           │
│                                                         │
│  ┌───────────────────────────────────────────────────┐ │
│  │   Secure Partition Manager (SPM)                  │ │
│  │   - Request routing                               │ │
│  │   - Partition scheduling                          │ │
│  │   - Isolation enforcement                         │ │
│  │   - Interrupt handling                            │ │
│  └───────────────────────────────────────────────────┘ │
│                          │                             │
│  ┌───────────────────────┼───────────────────────────┐ │
│  │ Secure Partitions     │                           │ │
│  │                       ▼                           │ │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────────┐   │ │
│  │  │  Crypto  │  │ Storage  │  │ Attestation  │   │ │
│  │  │ Service  │  │ (ITS/PS) │  │   Service    │   │ │
│  │  └──────────┘  └──────────┘  └──────────────┘   │ │
│  │                                                   │ │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────────┐   │ │
│  │  │ Platform │  │ Firmware │  │    Custom    │   │ │
│  │  │ Service  │  │  Update  │  │  Partitions  │   │ │
│  │  └──────────┘  └──────────┘  └──────────────┘   │ │
│  └───────────────────────────────────────────────────┘ │
│                                                         │
│  ┌───────────────────────────────────────────────────┐ │
│  │   Hardware Abstraction Layer (HAL)                │ │
│  │   - Platform-specific drivers                     │ │
│  │   - Crypto accelerators                           │ │
│  │   - Storage drivers                               │ │
│  └───────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────┘
                          │
                          ▼
┌────────────────────────────────────────────────────────┐
│                    Hardware                            │
│  - TrustZone  - Crypto  - Flash  - RAM  - Peripherals │
└────────────────────────────────────────────────────────┘
```

### 4.2 Boot Flow

**Multi-Stage Secure Boot:**

```
Power On
   │
   ▼
┌─────────────────────┐
│  BL1_1 (ROM)        │  ← Immutable, Root of Trust
│  - Verify BL1_2     │    - Hardware-based verification
│  - Jump to BL1_2    │    - Minimal code
└─────────────────────┘
   │
   ▼
┌─────────────────────┐
│  BL1_2 (Updateable) │  ← Provisioning & OTP
│  - Verify BL2       │    - Provision keys
│  - Jump to BL2      │    - Configure lifecycle
└─────────────────────┘
   │
   ▼
┌─────────────────────┐
│  BL2 (MCUboot)      │  ← Image Management
│  - Verify SPE image │    - Select boot image
│  - Verify NSPE image│    - Rollback protection
│  - Jump to SPE      │    - Encrypted image support
└─────────────────────┘
   │
   ▼
┌─────────────────────┐
│  TF-M (SPE)         │  ← Secure Runtime
│  - Initialize SPM   │    - Set up isolation
│  - Start partitions │    - Configure security
│  - Jump to NSPE     │    - Start services
└─────────────────────┘
   │
   ▼
┌─────────────────────┐
│  Application (NSPE) │  ← Normal World
│  - Run user code    │    - Business logic
└─────────────────────┘
```

### 4.3 Directory Structure

```
trusted-firmware-m/
│
├── bl1/                        # BL1 bootloader
│   ├── bl1_1/                  # Immutable first stage
│   └── bl1_2/                  # Updatable second stage
│
├── bl2/                        # BL2 bootloader (MCUboot)
│   └── ext/mcuboot/            # MCUboot source
│
├── secure_fw/                  # Secure firmware
│   ├── spm/                    # Secure Partition Manager
│   │   ├── core/               # SPM core logic
│   │   ├── include/            # SPM headers
│   │   └── ns_client_ext/      # NS client extensions
│   │
│   └── partitions/             # Secure partitions
│       ├── crypto/             # Crypto service
│       ├── internal_trusted_storage/  # ITS
│       ├── protected_storage/  # PS
│       ├── initial_attestation/# Attestation
│       ├── firmware_update/    # FWU service
│       └── platform/           # Platform service
│
├── interface/                  # PSA API headers
│   ├── include/
│   │   └── psa/
│   │       ├── crypto.h        # Crypto API
│   │       ├── internal_trusted_storage.h
│   │       ├── protected_storage.h
│   │       ├── initial_attestation.h
│   │       └── update.h        # FWU API
│   │
│   └── src/                    # PSA API implementations
│
├── platform/                   # Platform support
│   └── ext/
│       ├── target/             # Board-specific code
│       │   ├── arm/            # ARM platforms
│       │   ├── stm/            # STM32 boards
│       │   ├── nordic_nrf/     # Nordic boards
│       │   └── ...
│       │
│       └── common/             # Common platform code
│
├── lib/                        # Libraries
│   ├── ext/                    # External libraries
│   │   ├── mbedcrypto/         # Mbed TLS
│   │   ├── mcuboot/            # MCUboot
│   │   ├── qcbor/              # CBOR library
│   │   └── t_cose/             # COSE signing
│   │
│   └── fih/                    # Fault Injection Hardening
│
├── docs/                       # Documentation
│   ├── getting_started/
│   ├── design_docs/
│   ├── integration_guide/
│   └── platform/
│
├── config/                     # Configuration
│   ├── config_base.cmake       # Base configuration
│   └── profile/                # Configuration profiles
│       ├── profile_small/
│       ├── profile_medium/
│       └── profile_large/
│
├── cmake/                      # Build system
│   └── modules/                # CMake modules
│
└── tools/                      # Build tools
    ├── tf_fuzz/                # Fuzzing tools
    └── ...
```

### 4.4 Key Components Explained

**1. Secure Partition Manager (SPM)**
- Core of TF-M
- Manages secure partitions
- Routes service requests
- Enforces isolation
- Two implementations:
  - **SFN (Secure Function)**: Lightweight, less isolation
  - **IPC**: Full isolation, inter-process communication

**2. Secure Partitions**
- Isolated security services
- Run in Secure world
- Provide PSA RoT services
- Can be:
  - **Application RoT (ARoT)**: Application-specific
  - **Platform RoT (PRoT)**: Platform-level security

**3. Bootloaders**
- **BL1_1**: Immutable, ROM-based
- **BL1_2**: Updatable, provisioning
- **BL2**: MCUboot-based image manager

**4. Platform Layer**
- Hardware abstraction
- Board-specific code
- Driver interfaces
- Crypto accelerator integration

---

## Module 5: Development Environment Setup

### 5.1 Prerequisites

**Required Software:**

1. **Git**
   ```bash
   sudo apt-get install git
   git --version  # Should be 2.x or later
   ```

2. **CMake** (3.21 or later)
   ```bash
   sudo apt-get install cmake
   cmake --version
   ```

3. **Python** (3.8 or later)
   ```bash
   sudo apt-get install python3 python3-pip
   python3 --version
   ```

4. **GNU Arm Embedded Toolchain**
   ```bash
   # Download from ARM website or use package manager
   wget https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi.tar.xz

   tar xf arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi.tar.xz

   # Add to PATH
   export PATH=$PATH:/path/to/arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi/bin

   arm-none-eabi-gcc --version
   ```

5. **Python Dependencies**
   ```bash
   pip3 install imgtool cbor2 cryptography pyelftools pyyaml jinja2 click
   ```

### 5.2 Cloning TF-M Repository

```bash
# Clone TF-M repository
git clone https://git.trustedfirmware.org/TF-M/trusted-firmware-m.git
cd trusted-firmware-m

# Check current version
git describe --tags

# Install Python requirements
pip3 install -r tools/requirements.txt
```

### 5.3 First Build - ARM AN521 (MPS2+)

**Build for emulation platform:**

```bash
# Create build directory
mkdir build && cd build

# Configure build
cmake .. \
    -DTFM_PLATFORM=arm/mps2/an521 \
    -DTFM_TOOLCHAIN_FILE=../toolchain_GNUARM.cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DTEST_S=ON \
    -DTEST_NS=ON

# Build
cmake --build . -- -j$(nproc)
```

**Expected Output:**
```
Build directory structure:
build/
├── bin/
│   ├── bl2.axf              # Bootloader
│   ├── tfm_s.axf            # Secure firmware
│   └── tfm_ns.axf           # Non-secure application
│
├── image_signing/
│   ├── tfm_s_signed.bin     # Signed secure image
│   └── tfm_ns_signed.bin    # Signed NS image
│
└── install/
    └── outputs/
        └── fvp/
            └── tfm.elf      # Combined image for FVP
```

### 5.4 Running on Fixed Virtual Platform (FVP)

```bash
# Download ARM FVP
# Visit: https://developer.arm.com/tools-and-software/simulation-models/fixed-virtual-platforms

# Run TF-M on FVP
FVP_MPS2_Cortex-M33 \
    -a cpu0=build/bin/bl2.axf \
    --data build/bin/tfm_s_signed.bin@0x100000 \
    --data build/bin/tfm_ns_signed.bin@0x200000
```

**Expected Console Output:**
```
[INF] Starting bootloader
[INF] Image 0: magic=good, swap_type=0x1, copy_done=0x3, image_ok=0x3
[INF] Scratch: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
[INF] Boot source: primary slot
[INF] Swap type: none
[INF] Bootloader chainload address: 0x100000
[INF] Jumping to the first image slot
[Sec Thread] Secure image initializing!
...
```

### 5.5 Build Configuration Options

**Common Build Variables:**

```bash
# Platform selection
-DTFM_PLATFORM=<platform>
# Examples:
# arm/mps2/an521
# arm/musca_b1
# stm/nucleo_l552ze_q
# nordic_nrf/nrf5340dk_nrf5340_cpuapp

# Toolchain
-DTFM_TOOLCHAIN_FILE=../toolchain_GNUARM.cmake
# Options:
# toolchain_GNUARM.cmake      (GCC)
# toolchain_ARMCLANG.cmake    (ARM Compiler 6)
# toolchain_IARARM.cmake      (IAR)

# Build type
-DCMAKE_BUILD_TYPE=Debug
# Options: Debug, Release, RelWithDebInfo, MinSizeRel

# Configuration profile
-DTFM_PROFILE=profile_medium
# Options:
# profile_small      (Minimal footprint, SFN backend)
# profile_medium     (Balanced, IPC backend)
# profile_large      (Maximum security, IPC backend)

# Isolation level
-DTFM_ISOLATION_LEVEL=2
# Options:
# 1 - Basic isolation
# 2 - PSA RoT isolation (recommended)
# 3 - Maximum isolation

# SPM backend
-DCONFIG_TFM_SPM_BACKEND=IPC
# Options:
# SFN - Secure Function (lightweight)
# IPC - Inter-Process Communication (full isolation)

# Test configuration
-DTEST_S=ON          # Enable secure tests
-DTEST_NS=ON         # Enable non-secure tests
-DTEST_PSA_API=ON    # Enable PSA API tests
```

### 5.6 Development Tools

**1. OpenOCD (for hardware debugging)**
```bash
sudo apt-get install openocd

# Example: Debug STM32L562
openocd -f interface/stlink.cfg -f target/stm32l5x.cfg
```

**2. pyOCD (Python-based debugger)**
```bash
pip3 install pyocd

# List connected boards
pyocd list

# Flash and debug
pyocd flash -t stm32l562xe build/bin/combined.hex
pyocd gdbserver -t stm32l562xe
```

**3. JLink**
```bash
# Download from SEGGER website
# Use with Nordic boards, STM32, and others

JLinkGDBServer -device STM32L562ZE -if SWD -speed 4000
```

### 5.7 IDE Setup

**VS Code Configuration:**

Create `.vscode/settings.json`:
```json
{
    "cmake.configureSettings": {
        "TFM_PLATFORM": "arm/mps2/an521",
        "TFM_TOOLCHAIN_FILE": "${workspaceFolder}/toolchain_GNUARM.cmake",
        "CMAKE_BUILD_TYPE": "Debug"
    },
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
    "cmake.buildDirectory": "${workspaceFolder}/build",
    "cortex-debug.armToolchainPath": "/path/to/arm-none-eabi/bin"
}
```

Create `.vscode/launch.json` for debugging:
```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug TF-M (OpenOCD)",
            "type": "cortex-debug",
            "request": "launch",
            "servertype": "openocd",
            "cwd": "${workspaceRoot}",
            "executable": "${workspaceRoot}/build/bin/tfm_s.elf",
            "configFiles": [
                "interface/stlink.cfg",
                "target/stm32l5x.cfg"
            ],
            "svdFile": "${workspaceRoot}/platform/ext/target/stm/common/stm32l5xx.svd",
            "runToMain": true
        }
    ]
}
```

---

# PART 2: CORE CONCEPTS

## Module 6: Secure Partition Manager (SPM)

### 6.1 SPM Overview

The **Secure Partition Manager (SPM)** is the core component of TF-M that:
- Manages secure partitions (isolated services)
- Routes service requests from NS to S partitions
- Enforces isolation between partitions
- Handles secure interrupts
- Manages partition lifecycle

```
┌────────────────────────────────────────┐
│       Non-Secure Application           │
└────────────────────────────────────────┘
                 │ PSA Client API call
                 │ (e.g., psa_crypto_init())
                 ▼
┌────────────────────────────────────────┐
│              Veneer                     │  ← Secure Gateway
│         (NSC function)                  │
└────────────────────────────────────────┘
                 │
                 ▼
┌────────────────────────────────────────┐
│    Secure Partition Manager (SPM)      │
│  ┌──────────────────────────────────┐  │
│  │  1. Validate request             │  │
│  │  2. Identify target partition    │  │
│  │  3. Check permissions            │  │
│  │  4. Route to partition           │  │
│  │  5. Return result                │  │
│  └──────────────────────────────────┘  │
└────────────────────────────────────────┘
                 │
                 ▼
┌────────────────────────────────────────┐
│      Target Secure Partition            │
│      (e.g., Crypto Service)             │
│  ┌──────────────────────────────────┐  │
│  │  Execute requested operation     │  │
│  └──────────────────────────────────┘  │
└────────────────────────────────────────┘
```

### 6.2 SPM Backends

TF-M supports two SPM backend implementations:

**1. SFN (Secure Function) Backend**

```c
// Characteristics:
// - Lightweight
// - Function call based
// - Shared stack between partitions
// - Lower overhead
// - Isolation Level 1 only

Application → Veneer → SPM → Partition Function
                              ↓
                         Direct function call
```

**Configuration:**
```cmake
-DCONFIG_TFM_SPM_BACKEND=SFN
-DTFM_ISOLATION_LEVEL=1
```

**Use Cases:**
- Resource-constrained devices (< 256 KB Flash)
- Single-threaded applications
- Lower security requirements
- Minimal RAM overhead

**2. IPC (Inter-Process Communication) Backend**

```c
// Characteristics:
// - Full isolation
// - Message-based communication
// - Separate stacks per partition
// - Higher security
// - Supports all isolation levels

Application → Veneer → SPM → Message Queue → Partition Thread
                              ↓
                         Context switch
```

**Configuration:**
```cmake
-DCONFIG_TFM_SPM_BACKEND=IPC
-DTFM_ISOLATION_LEVEL=2  # or 3
```

**Use Cases:**
- Higher security requirements
- Multi-threaded partitions
- Complex service interactions
- PSA Certified Level 2/3

### 6.3 SPM Message Flow (IPC Backend)

**Detailed Call Sequence:**

```c
// Non-Secure Application
#include "psa/crypto.h"

void app_function(void) {
    psa_status_t status;

    // 1. NS application calls PSA API
    status = psa_crypto_init();

    // Function returns here after processing
}
```

**What happens internally:**

```
Step 1: NS calls psa_crypto_init()
   │
   ▼
Step 2: Veneer function (NSC region)
   │   __attribute__((cmse_nonsecure_entry))
   │   psa_status_t tfm_crypto_init_veneer(void) {
   │       return tfm_crypto_init_ipc();
   │   }
   ▼
Step 3: SPM IPC Handler
   │   - Allocate message buffer
   │   - Prepare message for crypto partition
   │   - psa_call(CRYPTO_HANDLE, CRYPTO_INIT, ...)
   ▼
Step 4: SPM Message Dispatch
   │   - Add message to crypto partition queue
   │   - Wake crypto partition thread
   │   - Context switch to crypto partition
   ▼
Step 5: Crypto Partition Thread
   │   - Receive message (psa_get())
   │   - Process: status = crypto_init_impl()
   │   - Reply with result (psa_reply())
   ▼
Step 6: SPM Return Path
   │   - Context switch back to caller
   │   - Copy result to NS memory
   │   - Return via veneer
   ▼
Step 7: NS application continues
```

### 6.4 Partition Manifest

Every secure partition has a manifest file that defines its properties:

**Example: Crypto Partition Manifest**

```json
{
  "psa_framework_version": 1.1,
  "name": "TFM_SP_CRYPTO",
  "type": "APPLICATION-ROT",
  "priority": "NORMAL",
  "entry_point": "tfm_crypto_init",
  "stack_size": "0x2000",
  "heap_size": "0x0",

  "services": [
    {
      "name": "TFM_CRYPTO",
      "sid": "0x00000100",
      "version": 1,
      "non_secure_clients": true,
      "connection_based": false,
      "stateless_handle": true,
      "mm_iovec": "enable"
    }
  ],

  "mmio_regions": [
    {
      "name": "TFM_CRYPTO_ACCELERATOR",
      "permission": "READ-WRITE"
    }
  ],

  "irqs": [
    {
      "signal": "TFM_CRYPTO_IRQ",
      "line": 42,
      "priority": 64
    }
  ]
}
```

**Manifest Fields Explained:**

```yaml
name: "TFM_SP_CRYPTO"
  # Unique partition identifier

type: "APPLICATION-ROT"
  # PSA-ROT: Platform Root of Trust (higher security)
  # APPLICATION-ROT: Application Root of Trust

priority: "NORMAL"
  # Thread priority: LOW, NORMAL, HIGH
  # Only for IPC backend

entry_point: "tfm_crypto_init"
  # Function called when partition starts

stack_size: "0x2000"
  # Stack size for partition thread (IPC)
  # Ignored in SFN backend

services:
  # List of services provided by partition

  sid: "0x00000100"
    # Service ID - unique identifier
    # Used in psa_connect() calls

  non_secure_clients: true
    # Allow NS applications to call this service

  connection_based: false
    # true: Use psa_connect/psa_call/psa_close
    # false: Use psa_call directly (stateless)

  mm_iovec: "enable"
    # Memory mapped I/O vectors
    # Allows direct memory access for performance

mmio_regions:
  # Hardware peripherals this partition can access
  # Enforced by MPU

irqs:
  # Interrupts handled by this partition
  # Secure interrupts only
```

### 6.5 SPM API for Partitions

**Service Implementation (Partition Side):**

```c
#include "psa/service.h"
#include "tfm_sp_log.h"

#define MY_SERVICE_SID    0x00001000
#define MY_SERVICE_SIGNAL (1U << 0)

/* Partition entry point */
void my_service_entry(void) {
    psa_msg_t msg;
    psa_status_t status;

    LOG_MSG("My service started");

    /* Main service loop */
    while (1) {
        /* Wait for signal */
        psa_signal_t signals = psa_wait(MY_SERVICE_SIGNAL, PSA_BLOCK);

        if (signals & MY_SERVICE_SIGNAL) {
            /* Receive message */
            if (psa_get(MY_SERVICE_SIGNAL, &msg) != PSA_SUCCESS) {
                continue;
            }

            /* Process based on message type */
            switch (msg.type) {
            case PSA_IPC_CONNECT:
                /* Client connecting */
                status = PSA_SUCCESS;
                break;

            case PSA_IPC_CALL:
                /* Process the request */
                status = handle_service_request(&msg);
                break;

            case PSA_IPC_DISCONNECT:
                /* Client disconnecting */
                status = PSA_SUCCESS;
                break;

            default:
                status = PSA_ERROR_NOT_SUPPORTED;
                break;
            }

            /* Reply to caller */
            psa_reply(msg.handle, status);
        }
    }
}

static psa_status_t handle_service_request(const psa_msg_t *msg) {
    uint8_t input_buffer[128];
    uint8_t output_buffer[128];
    size_t num_bytes;

    /* Read input parameters */
    if (msg->in_size[0] > sizeof(input_buffer)) {
        return PSA_ERROR_BUFFER_TOO_SMALL;
    }

    num_bytes = psa_read(msg->handle, 0, input_buffer, msg->in_size[0]);

    /* Perform operation */
    // ... process input_buffer, generate output_buffer ...

    /* Write output */
    psa_write(msg->handle, 0, output_buffer, sizeof(output_buffer));

    return PSA_SUCCESS;
}
```

**Client Side (Non-Secure or Other Partition):**

```c
#include "psa/client.h"

#define MY_SERVICE_SID 0x00001000

void client_function(void) {
    psa_handle_t handle;
    psa_status_t status;

    uint8_t input[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    uint8_t output[128];

    /* Connect to service (connection-based only) */
    handle = psa_connect(MY_SERVICE_SID, 1);
    if (handle <= 0) {
        // Handle error
        return;
    }

    /* Prepare I/O vectors */
    psa_invec in_vec[] = {
        {input, sizeof(input)}
    };

    psa_outvec out_vec[] = {
        {output, sizeof(output)}
    };

    /* Call service */
    status = psa_call(handle, PSA_IPC_CALL, in_vec, 1, out_vec, 1);

    if (status == PSA_SUCCESS) {
        // Process output
    }

    /* Disconnect */
    psa_close(handle);
}

/* For stateless services (connection_based = false) */
void client_stateless_call(void) {
    psa_status_t status;
    uint8_t data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    psa_invec in_vec = {data, sizeof(data)};

    /* Direct call without connect/close */
    status = psa_call(MY_SERVICE_SID, PSA_IPC_CALL, &in_vec, 1, NULL, 0);
}
```

### 6.6 SPM Configuration

**Key Configuration Options:**

```cmake
# SPM Backend selection
CONFIG_TFM_SPM_BACKEND=IPC  # or SFN

# Isolation level
TFM_ISOLATION_LEVEL=2       # 1, 2, or 3

# Stack protection
CONFIG_TFM_STACK_WATERMARKS=ON  # Stack usage monitoring

# Handle numbers
CONFIG_TFM_CONN_HANDLE_MAX_NUM=8  # Max concurrent connections

# Service handles
CONFIG_TFM_SERVICE_MAX_NUM=16     # Max services

# IRQ handling
CONFIG_TFM_ENABLE_IRQ_TEST=OFF    # Test secure IRQs

# Memory management
CONFIG_TFM_ENABLE_MEMORY_PROTECT=ON  # Enable MPU protection
```

### 6.7 SPM Security Features

**1. Input Validation**
```c
/* SPM validates all NS pointers */
if (cmse_check_address_range(ns_ptr, size, CMSE_NONSECURE) == NULL) {
    return PSA_ERROR_INVALID_ARGUMENT;
}
```

**2. Memory Isolation**
- MPU configured per partition
- Prevents unauthorized memory access
- Stack isolation (IPC backend)

**3. Priority-Based Scheduling**
- Partitions have configurable priorities
- Critical services get higher priority
- Prevents resource starvation

**4. Secure Interrupt Handling**
- Interrupts can be assigned to partitions
- Direct interrupt handling in partitions
- Signal-based notification

---

## Module 7: Isolation Levels and Security Models

### 7.1 Isolation Level Overview

TF-M supports three isolation levels, each providing increasing security at the cost of additional overhead:

```
Level 1: Basic Protection
├── SPE isolated from NSPE
├── No internal SPE isolation
└── Lowest overhead

Level 2: PSA RoT Isolation
├── SPE isolated from NSPE
├── PSA RoT isolated from App RoT
└── Recommended for most applications

Level 3: Maximum Isolation
├── SPE isolated from NSPE
├── PSA RoT isolated from App RoT
├── Each partition isolated from others
└── Highest security
```

### 7.2 Isolation Level 1

**Characteristics:**
- Minimal isolation
- SPE vs NSPE separation only
- All partitions share memory space
- Requires SFN backend
- Smallest footprint

**Security Boundaries:**

```
┌──────────────────────────────┐
│    Non-Secure World          │
│    (Application)             │  NS=1
└──────────────────────────────┘
         ═══════════════════════════ TrustZone Boundary
┌──────────────────────────────┐
│    Secure World (SPE)        │  NS=0
│  ┌────────────────────────┐  │
│  │ All Partitions         │  │
│  │ • Crypto               │  │  Shared
│  │ • Storage              │  │  Memory
│  │ • Attestation          │  │  Space
│  │ • Platform             │  │
│  └────────────────────────┘  │
└──────────────────────────────┘
```

**Memory Layout:**
```
Secure Memory:
├── SPM Code
├── All Partition Code (no separation)
├── All Partition Data (no separation)
└── Shared Stack
```

**Configuration:**
```cmake
-DTFM_ISOLATION_LEVEL=1
-DCONFIG_TFM_SPM_BACKEND=SFN
```

**Use Cases:**
- Very resource-constrained devices
- Lower security requirements
- Development and testing

### 7.3 Isolation Level 2

**Characteristics:**
- PSA RoT vs App RoT isolation
- IPC backend required
- Separate stacks per partition
- Recommended isolation level

**Security Boundaries:**

```
┌────────────────────────────────────┐
│       Non-Secure World             │  NS=1
└────────────────────────────────────┘
         ══════════════════════════════ TrustZone Boundary
┌────────────────────────────────────┐
│     Secure World (SPE)             │  NS=0
│  ┌──────────────────────────────┐  │
│  │   Application RoT            │  │
│  │   • Custom Partitions        │  │
│  │   • FWU Service              │  │  MPU
│  └──────────────────────────────┘  │  Region
│          ─────────────────────────────── Isolation
│  ┌──────────────────────────────┐  │
│  │   Platform RoT (PSA RoT)     │  │
│  │   • Crypto                   │  │
│  │   • ITS                      │  │  MPU
│  │   • PS                       │  │  Region
│  │   • Attestation              │  │
│  │   • Platform Service         │  │
│  └──────────────────────────────┘  │
└────────────────────────────────────┘
```

**Memory Protection:**
```
Each RoT Domain has:
├── Separate MPU region for code
├── Separate MPU region for data
├── Private stacks per partition
└── Controlled inter-partition communication
```

**Configuration:**
```cmake
-DTFM_ISOLATION_LEVEL=2
-DCONFIG_TFM_SPM_BACKEND=IPC
```

**Use Cases:**
- Production devices
- PSA Certified applications
- Balanced security and performance

### 7.4 Isolation Level 3

**Characteristics:**
- Maximum isolation
- Each partition isolated from all others
- Highest security
- Largest overhead

**Security Boundaries:**

```
┌────────────────────────────────────┐
│       Non-Secure World             │  NS=1
└────────────────────────────────────┘
         ══════════════════════════════ TrustZone Boundary
┌────────────────────────────────────┐
│     Secure World (SPE)             │  NS=0
│                                     │
│  ┌──────────────────────────────┐  │
│  │   Partition 1 (FWU)          │  │  MPU Region 1
│  └──────────────────────────────┘  │
│          ─────────────────────────────── MPU Boundary
│  ┌──────────────────────────────┐  │
│  │   Partition 2 (Crypto)       │  │  MPU Region 2
│  └──────────────────────────────┘  │
│          ─────────────────────────────── MPU Boundary
│  ┌──────────────────────────────┐  │
│  │   Partition 3 (ITS)          │  │  MPU Region 3
│  └──────────────────────────────┘  │
│          ─────────────────────────────── MPU Boundary
│  ┌──────────────────────────────┐  │
│  │   Partition 4 (PS)           │  │  MPU Region 4
│  └──────────────────────────────┘  │
│          ─────────────────────────────── MPU Boundary
│  ┌──────────────────────────────┐  │
│  │   Partition 5 (Attestation)  │  │  MPU Region 5
│  └──────────────────────────────┘  │
└────────────────────────────────────┘
```

**Memory Protection:**
```
Each Partition has:
├── Dedicated MPU region for code
├── Dedicated MPU region for data
├── Private stack
├── Protected MMIO regions
└── All communication via SPM
```

**Configuration:**
```cmake
-DTFM_ISOLATION_LEVEL=3
-DCONFIG_TFM_SPM_BACKEND=IPC
```

**Use Cases:**
- Maximum security requirements
- PSA Certified Level 3
- Critical infrastructure
- High-value assets

### 7.5 Choosing Isolation Level

**Decision Matrix:**

| Criterion | Level 1 | Level 2 | Level 3 |
|-----------|---------|---------|---------|
| **Flash** | ~40 KB | ~60 KB | ~80 KB |
| **RAM** | ~20 KB | ~30 KB | ~40 KB |
| **Performance** | Fastest | Medium | Slower |
| **Security** | Basic | Good | Maximum |
| **PSA Cert** | Level 1 | Level 2 | Level 3 |
| **Complexity** | Low | Medium | High |

**Recommendation Flow:**
```
Start
  │
  ▼
Do you need PSA Certification Level 3?
  ├─ YES → Use Level 3
  │
  └─ NO → Do you have < 100 KB Flash?
         ├─ YES → Use Level 1
         │
         └─ NO → Use Level 2 (Recommended)
```

### 7.6 RoT Classification

**Platform Root of Trust (PSA RoT):**
- Core security services
- Platform-level functionality
- Higher privilege
- Examples:
  - Crypto
  - Internal Trusted Storage
  - Attestation
  - Platform Service

**Application Root of Trust (App RoT):**
- Application-specific services
- Lower privilege (in Level 2+)
- Examples:
  - Firmware Update
  - Protected Storage
  - Custom secure services

**Partition Declaration:**

```json
{
  "name": "TFM_SP_CRYPTO",
  "type": "PSA-ROT",    ← Declares PSA RoT partition
  ...
}

{
  "name": "TFM_SP_FWU",
  "type": "APPLICATION-ROT",    ← Declares App RoT partition
  ...
}
```

### 7.7 Practical Isolation Example

**Scenario: Crypto Key Compromise**

**Level 1:**
```
If crypto partition is compromised:
❌ Attacker can access all other partitions
❌ Can read storage data directly
❌ Can extract attestation keys
❌ Full SPE compromise
```

**Level 2:**
```
If crypto partition is compromised:
✅ PSA RoT partitions protected (Crypto is PSA RoT)
❌ But if in App RoT, other App RoT accessible
⚠️  Limited damage with proper RoT classification
```

**Level 3:**
```
If crypto partition is compromised:
✅ All other partitions protected by MPU
✅ Cannot access ITS, PS, Attestation
✅ Isolated memory regions
✅ Compromise contained to crypto partition only
```

---

This is Part 1 of the training guide. Would you like me to continue with the remaining modules? The guide will include:

- **Part 2**: Core Concepts (continued - Modules 8-10)
- **Part 3**: Secure Services (Modules 11-15)
- **Part 4**: Secure Boot and MCUboot Deep Dive (Modules 16-20)
- **Part 5**: Advanced Topics (Modules 21-25)
- **Part 6**: Real-World Projects (Modules 26-27)
- **Comprehensive Lab Exercises** for each module
- **Detailed project guides** for STM32U5 and NRF52840 trackers

Let me know if you'd like me to continue with the next sections!

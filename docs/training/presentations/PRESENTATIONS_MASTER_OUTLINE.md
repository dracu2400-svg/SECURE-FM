# TF-M Training Package - Master Presentation Outline

**Purpose:** Complete slide deck specifications for all 6 sections
**Format:** PowerPoint/PDF (each section 40-60 slides)
**Audience:** Embedded engineers learning TF-M
**Style:** Professional, visual, code-heavy

---

## Section 1: TrustZone-M Foundations (50 slides)

### Slide 1: Title Slide
- **Title:** "TrustZone-M Foundations"
- **Subtitle:** "Hardware-Based Security for Embedded Systems"
- **Image:** ARM Cortex-M33 die photo + TrustZone logo
- **Footer:** "TF-M Training Package - Section 1"

### Slides 2-5: Why Security Matters
- **Slide 2:** Embedded Device Threats
  - Photos of hacked devices (smart lock, car, medical device)
  - Statistics: "70% of IoT devices have vulnerabilities"

- **Slide 3:** Cost of Security Breaches
  - Case study: Mirai botnet (IoT device botnet)
  - Financial impact: $millions in damages

- **Slide 4:** Traditional Security Limitations
  - Software-only protection can be bypassed
  - Need for hardware isolation

- **Slide 5:** Enter TrustZone-M
  - "Hardware-enforced security boundary"
  - Visual: Secure vs Non-Secure worlds

### Slides 6-15: TrustZone-M Architecture
- **Slide 6:** ARM Cortex-M33 Block Diagram
  - Highlight TrustZone components (SAU, IDAU, MPU)

- **Slide 7:** Secure vs Non-Secure Worlds
  - Split-screen visual
  - Green (Secure) | Blue (Non-Secure)

- **Slide 8:** Memory Partitioning
  - Flash: 256KB Secure | 8KB NSC | 256KB Non-Secure
  - SRAM: 64KB Secure | 192KB Non-Secure

- **Slide 9:** SAU (Security Attribution Unit)
  - Code example: SAU region configuration
  ```c
  SAU->RNR = 0;
  SAU->RBAR = 0x0C03E000;  // NSC region
  SAU->RLAR = (0x0C040000 - 1) | SAU_NSC_REGION;
  ```

- **Slide 10:** IDAU (Implementation Defined Attribution Unit)
  - Chip-specific security configuration
  - Cannot be changed by software

- **Slide 11:** Context Switching
  - Animation/diagram showing NS→S transition
  - Register clearing for security

- **Slide 12:** NSC (Non-Secure Callable) Regions
  - Special memory region for secure gateways
  - SG instruction explained

- **Slide 13:** Secure/Non-Secure Interrupts
  - Interrupt priority and routing
  - SecureFault exception

- **Slide 14:** Exception Handling
  - SecureFault, HardFault, BusFault
  - Debug authentication

- **Slide 15:** Lab 02 Demo
  - Screenshot of LED indicators
  - "See TrustZone in action!"

### Slides 16-25: Programming Model
- **Slide 16:** CMSE (Cortex-M Security Extensions)
  - Compiler intrinsics
  - `__attribute__((cmse_nonsecure_entry))`

- **Slide 17:** NSC Function Example
  ```c
  __attribute__((cmse_nonsecure_entry))
  void secure_led_blink(uint32_t count)
  {
      // Security checks
      // Secure implementation
  }
  ```

- **Slide 18:** Pointer Validation
  ```c
  void *checked = cmse_check_address_range(
      buffer, size, CMSE_NONSECURE);
  if (checked == NULL) {
      return ERROR;  // Invalid pointer!
  }
  ```

- **Slide 19:** Common Pitfalls
  - ❌ Not validating NS pointers
  - ❌ Leaking secure data via return values
  - ❌ Using NS pointers in secure memory access

- **Slide 20:** Best Practices
  - ✓ Always validate pointers from NS
  - ✓ Minimize NSC function count
  - ✓ Clear registers on return

- **Slides 21-25:** Code Walkthroughs
  - Lab 02 source code explained
  - main_s.c structure
  - main_ns.c structure
  - nsc_functions.c security checks

### Slides 26-35: TF-M Overview
- **Slide 26:** What is TF-M?
  - "Trusted Firmware-M: ARM's reference implementation"
  - Open source, PSA Certified

- **Slide 27:** TF-M Architecture
  - Layered diagram: SPM → Secure Services → PSA APIs

- **Slide 28:** Secure Partition Manager (SPM)
  - Manages secure partitions
  - Context switching
  - IPC mechanism

- **Slide 29:** PSA Firmware Framework
  - Isolation levels (1, 2, 3)
  - Partition manifest

- **Slide 30:** Core Secure Services
  - Crypto
  - Storage (ITS, PS)
  - Attestation
  - FWU (Firmware Update)

- **Slide 31:** PSA APIs
  - Standardized interface
  - Vendor-agnostic
  - Future-proof

- **Slide 32:** MCUboot Integration
  - Secure boot process
  - Image verification
  - Rollback protection

- **Slide 33:** PSA Certification
  - Levels 1, 2, 3
  - Compliance requirements

- **Slide 34:** TF-M vs Custom Solutions
  - Table comparison
  - Time to market benefits

- **Slide 35:** Ecosystem Support
  - Logos: ST, NXP, Nordic, Cypress
  - IDE support: Keil, IAR, GCC

### Slides 36-45: Development Tools
- **Slide 36:** Toolchain Setup
  - ARM GCC
  - CMake
  - OpenOCD

- **Slide 37:** STM32CubeMX
  - Screenshot: TrustZone configuration

- **Slide 38:** Memory Configuration
  - Linker script examples

- **Slide 39:** Build Process
  - Flowchart: Compile S → Compile NS → Link → Merge

- **Slide 40:** Debugging TrustZone
  - GDB with TrustZone support
  - Separate S/NS debug sessions

- **Slides 41-42:** Lab Exercises
  - Lab 01: Hello TrustZone
  - Lab 02: Interactive Demo

- **Slides 43-44:** Hands-On Workshop
  - Build and flash instructions
  - Expected results

- **Slide 45:** Troubleshooting
  - Common errors and solutions

### Slides 46-50: Summary & Next Steps
- **Slide 46:** Key Concepts Review
  - TrustZone-M provides hardware isolation
  - SAU/IDAU enforce security boundaries
  - NSC regions for secure entry points

- **Slide 47:** What You Learned
  - ✓ TrustZone-M architecture
  - ✓ Secure/Non-Secure partitioning
  - ✓ NSC function implementation
  - ✓ CMSE intrinsics

- **Slide 48:** Next Section Preview
  - "Section 2: PSA Core Services"
  - Crypto, Storage, Attestation

- **Slide 49:** Additional Resources
  - ARM TrustZone-M documentation
  - TF-M project on GitHub
  - PSA Certified website

- **Slide 50:** Q&A
  - Contact information
  - Course website

---

## Section 2: PSA Core Services (55 slides)

### Slide 1: Title Slide
- **Title:** "PSA Core Services"
- **Subtitle:** "Crypto, Storage & Attestation"
- **Image:** Padlock icon + key icon
- **Footer:** "TF-M Training Package - Section 2"

### Slides 2-20: PSA Crypto API
- **Slide 2:** Why Cryptography Matters
  - Data confidentiality
  - Authentication
  - Integrity

- **Slide 3:** PSA Crypto Architecture
  - Hardware abstraction layer
  - Algorithm agnostic

- **Slide 4:** Key Management
  - Key generation
  - Key storage
  - Key lifecycle

- **Slide 5:** Symmetric Encryption
  - AES-128, AES-256
  - Modes: CBC, CTR, GCM

- **Slide 6:** AES-GCM Explained
  - Diagram: Plaintext → AES → Ciphertext + Tag
  - Nonce importance

- **Slide 7:** Code Example: AES Encryption
  ```c
  psa_aead_encrypt(key_id, PSA_ALG_GCM,
                   nonce, sizeof(nonce),
                   NULL, 0,
                   plaintext, plaintext_len,
                   ciphertext, ciphertext_size,
                   &ciphertext_len);
  ```

- **Slide 8:** Asymmetric Cryptography
  - RSA, ECDSA, ECDH
  - Public/private key pairs

- **Slide 9:** Hash Functions
  - SHA-256, SHA-384, SHA-512
  - Use cases

- **Slide 10:** HMAC
  - Message authentication codes
  - Integrity + authenticity

- **Slide 11:** Key Derivation (KDF)
  - HKDF, PBKDF2
  - Deriving keys from passwords

- **Slide 12:** Random Number Generation
  - TRNG vs PRNG
  - Importance of entropy

- **Slides 13-15:** Lab 03 Walkthrough
  - Interactive crypto demo
  - LED feedback
  - Button-triggered operations

- **Slides 16-18:** Security Considerations
  - Side-channel attacks
  - Timing attacks
  - Power analysis

- **Slides 19-20:** Best Practices
  - Never reuse nonces
  - Use hardware acceleration
  - Zeroize sensitive data

### Slides 21-35: PSA Secure Storage
- **Slide 21:** Why Secure Storage?
  - Credentials protection
  - Tamper resistance
  - Data persistence

- **Slide 22:** ITS vs PS
  - Table comparison
  - Use case examples

- **Slide 23:** ITS (Internal Trusted Storage)
  - Internal flash only
  - Rollback protected
  - Critical data

- **Slide 24:** PS (Protected Storage)
  - Can use external flash
  - Larger capacity
  - Application data

- **Slide 25:** Storage APIs
  ```c
  psa_its_set(uid, data_len, data, flags);
  psa_its_get(uid, offset, size, buffer, &len);
  psa_its_remove(uid);
  psa_its_get_info(uid, &info);
  ```

- **Slide 26:** Data Encryption
  - Automatic encryption at rest
  - Device-unique keys

- **Slide 27:** Write-Once Protection
  ```c
  psa_its_set(uid, len, data,
              PSA_STORAGE_FLAG_WRITE_ONCE);
  ```

- **Slide 28:** Rollback Protection
  - Prevents downgrade attacks
  - Monotonic counters

- **Slide 29:** Use Case: API Key Storage
  - Store cloud API keys
  - Retrieve on demand
  - Never expose to debugger

- **Slide 30:** Use Case: Device Provisioning
  - Factory programming
  - Unique device ID
  - Certificate storage

- **Slides 31-33:** Lab 04 Demo
  - Store/retrieve credentials
  - Persistence test (reboot)
  - Write-once demonstration

- **Slides 34-35:** Storage Security
  - Encryption details
  - Attack scenarios
  - Mitigation strategies

### Slides 36-50: PSA Initial Attestation
- **Slide 36:** What is Attestation?
  - "Proving device identity and state"
  - Remote verification

- **Slide 37:** Use Cases
  - Device onboarding
  - Secure firmware update
  - Cloud authentication

- **Slide 38:** Attestation Token
  - CBOR/COSE format
  - Claims included

- **Slide 39:** Claims in Token
  - Device ID
  - Firmware version
  - Boot state
  - Security lifecycle

- **Slide 40:** Token Generation
  ```c
  psa_initial_attest_get_token(
      challenge, challenge_size,
      token_buf, token_buf_size,
      &token_size);
  ```

- **Slide 41:** Verification Process
  - Server checks signature
  - Validates claims
  - Trusts or rejects

- **Slide 42:** Attestation Keys
  - Device Attestation Key (DAK)
  - Initial Attestation Key (IAK)
  - Key provisioning

- **Slides 43-45:** Lab 05 Example
  - Generate attestation token
  - Parse claims
  - Verify signature

- **Slides 46-48:** Integration with Cloud
  - AWS IoT Core
  - Azure IoT Hub
  - Google Cloud IoT

- **Slides 49-50:** Summary
  - Core services overview
  - Integration examples

### Slides 51-55: Section Summary
- **Slide 51:** What You Learned
  - ✓ PSA Crypto for encryption/hashing
  - ✓ Secure storage (ITS/PS)
  - ✓ Device attestation

- **Slide 52:** Real-World Application
  - GPS tracker credentials
  - Encrypted communication

- **Slide 53:** Next Section
  - "Section 3: Advanced Features"

- **Slide 54:** Resources
  - PSA Crypto specification
  - TF-M documentation

- **Slide 55:** Q&A

---

## Section 3: Advanced TF-M Topics (50 slides)

### Topics Covered:
1. Custom Secure Services (Slides 1-15)
2. Partition Design (Slides 16-25)
3. IPC Mechanism (Slides 26-35)
4. Isolation Levels (Slides 36-45)
5. Summary (Slides 46-50)

*(Detailed slide breakdown similar to above)*

---

## Section 4: Integration & Deployment (45 slides)

### Topics Covered:
1. RTOS Integration (FreeRTOS, Zephyr)
2. MCUboot Secure Boot
3. Firmware Update (OTA)
4. Debug & Testing
5. Production Deployment

---

## Section 5: Performance & Optimization (40 slides)

### Topics Covered:
1. Memory Optimization
2. Power Management
3. Latency Reduction
4. Code Size Optimization
5. Benchmarking

---

## Section 6: Security & Attack Mitigations (60 slides)

### Topics Covered:
1. Threat Modeling
2. Software Attacks
3. Hardware Attacks
4. Side-Channel Attacks
5. Fault Injection
6. Physical Security
7. Countermeasures
8. Secure Coding Practices

---

## Presentation Design Guidelines

### Visual Style
- **Colors:**
  - Secure world: Green (#00AA00)
  - Non-Secure world: Blue (#0066CC)
  - Errors/Attacks: Red (#CC0000)
  - Neutral: Gray (#666666)

### Fonts
- **Titles:** Arial Bold 44pt
- **Body:** Arial 24pt
- **Code:** Consolas 18pt

### Code Blocks
- Dark background (#1E1E1E)
- Syntax highlighting
- Line numbers
- Maximum 15 lines per slide

### Images
- High-resolution (300 DPI)
- Annotated diagrams
- Photos of actual hardware
- Screenshots from labs

### Animations
- Minimal, purposeful
- Highlight security boundaries
- Show data flow
- Step-by-step processes

---

## Delivery Notes

### Each Section Includes:
1. **Theory Slides** (60% of content)
2. **Code Examples** (25% of content)
3. **Lab References** (10% of content)
4. **Quizzes/Activities** (5% of content)

### Estimated Presentation Time:
- Section 1: 2.5 hours (with breaks)
- Section 2: 3 hours
- Section 3: 2 hours
- Section 4: 2 hours
- Section 5: 1.5 hours
- Section 6: 3 hours

**Total Course:** 14 hours (2-day workshop)

---

## Next Steps for Conversion

1. ✅ Use this outline to create PowerPoint slides
2. ✅ Add diagrams using draw.io or PowerPoint SmartArt
3. ✅ Insert code snippets from labs
4. ✅ Add photos of NUCLEO board
5. ✅ Create animations for complex concepts
6. ✅ Export as PDF for distribution

---

**Document Version:** 1.0
**Last Updated:** 2025-11-22
**Total Slides:** ~295 slides across 6 sections

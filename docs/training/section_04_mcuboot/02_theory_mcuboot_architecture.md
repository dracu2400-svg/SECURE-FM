# MCUboot Complete Guide for TF-M
## Comprehensive Boot Security and Firmware Update

---

## 📚 Table of Contents

1. [Introduction to MCUboot](#1-introduction-to-mcuboot)
2. [MCUboot Architecture](#2-mcuboot-architecture)
3. [Image Format and Signing](#3-image-format-and-signing)
4. [Swap Mechanisms](#4-swap-mechanisms)
5. [Rollback Protection](#5-rollback-protection)
6. [Encrypted Images](#6-encrypted-images)
7. [Multi-Image Boot](#7-multi-image-boot)
8. [Integration with TF-M](#8-integration-with-tf-m)
9. [Hands-On Labs](#9-hands-on-labs)
10. [Advanced Topics](#10-advanced-topics)

---

## 1. Introduction to MCUboot

### 1.1 What is MCUboot?

**MCUboot** is a secure bootloader for 32-bit microcontrollers. It is designed to be operating system and hardware independent, providing secure boot and firmware update functionality.

**Key Features:**
- ✅ **Secure Boot**: Cryptographic verification of firmware images
- ✅ **Firmware Update**: Over-the-air (OTA) and wired updates
- ✅ **Rollback Protection**: Prevent downgrade to vulnerable versions
- ✅ **Encrypted Images**: Support for encrypted firmware
- ✅ **Multi-Image**: Boot multiple images (e.g., secure + non-secure)
- ✅ **Multiple Signatures**: RSA-2048, RSA-3072, ECDSA-P256, ED25519
- ✅ **Portable**: Works across ARM, RISC-V, x86, Xtensa

### 1.2 Why MCUboot?

**Security Requirements:**
```
Without Secure Boot:
┌─────────────────────┐
│  Attacker can:      │
│  • Load malware     │
│  • Bypass security  │
│  • Extract secrets  │
│  • Brick device     │
└─────────────────────┘

With MCUboot:
┌─────────────────────┐
│  Protection:        │
│  ✓ Verify firmware  │
│  ✓ Reject unsigned  │
│  ✓ Prevent rollback │
│  ✓ Encrypted images │
└─────────────────────┘
```

**Use Cases:**
- IoT devices requiring secure updates
- Medical devices (FDA requirements)
- Industrial control systems
- Automotive applications
- Consumer electronics
- Smart home devices

### 1.3 MCUboot vs Other Bootloaders

| Feature | MCUboot | U-Boot | Custom | Vendor |
|---------|---------|--------|--------|--------|
| Open Source | ✅ Apache 2.0 | ✅ GPL | ⚠️ Varies | ❌ Proprietary |
| MCU Optimized | ✅ Yes | ❌ No (MPU) | ⚠️ Maybe | ✅ Yes |
| Footprint | ✅ Small (24KB+) | ❌ Large (100KB+) | ✅ Varies | ⚠️ Varies |
| Security | ✅ Strong | ⚠️ Good | ❌ Unknown | ⚠️ Varies |
| Portability | ✅ High | ⚠️ Medium | ❌ Low | ❌ Vendor lock |
| Community | ✅ Active | ✅ Large | ❌ None | ⚠️ Limited |
| Certification | ✅ PSA ready | ❌ No | ❌ No | ⚠️ Some |

---

## 2. MCUboot Architecture

### 2.1 High-Level Architecture

```
┌────────────────────────────────────────────────────────┐
│                    Flash Memory                         │
├────────────────────────────────────────────────────────┤
│                                                         │
│  ┌──────────────────────────────────────────────────┐  │
│  │  MCUboot Bootloader                              │  │
│  │  • Image verification                            │  │
│  │  • Swap management                               │  │
│  │  • Rollback protection                           │  │
│  │  Size: ~24-64 KB                                 │  │
│  └──────────────────────────────────────────────────┘  │
│                                                         │
│  ┌─────────────────────┬────────────────────────────┐  │
│  │  Primary Slot       │  Secondary Slot            │  │
│  │  (Active Firmware)  │  (Update Staging)          │  │
│  ├─────────────────────┼────────────────────────────┤  │
│  │  Image Header       │  Image Header              │  │
│  │  TLV Area           │  TLV Area                  │  │
│  │  ┌───────────────┐  │  ┌───────────────────────┐ │  │
│  │  │ Secure Image  │  │  │ Secure Image (Update)│ │  │
│  │  └───────────────┘  │  └───────────────────────┘ │  │
│  │  ┌───────────────┐  │  ┌───────────────────────┐ │  │
│  │  │ NS Image      │  │  │ NS Image (Update)    │ │  │
│  │  └───────────────┘  │  └───────────────────────┘ │  │
│  │  Image Trailer      │  Image Trailer             │  │
│  └─────────────────────┴────────────────────────────┘  │
│                                                         │
│  ┌──────────────────────────────────────────────────┐  │
│  │  Scratch Area (for swap operations)              │  │
│  └──────────────────────────────────────────────────┘  │
│                                                         │
└────────────────────────────────────────────────────────┘
```

### 2.2 Boot Flow

**Complete Boot Sequence:**

```
Power-On Reset
      │
      ▼
┌──────────────────────┐
│ 1. MCUboot Starts    │
│    • Init HW         │
│    • Setup MPU/SAU   │
└──────────────────────┘
      │
      ▼
┌──────────────────────┐
│ 2. Check Swap Status │
│    • Read trailer    │
│    • Determine state │
└──────────────────────┘
      │
      ├─────→ Swap Needed? ─────Yes────┐
      │                                 │
      No                                ▼
      │                         ┌──────────────┐
      │                         │ 3. Perform   │
      │                         │    Swap      │
      │                         └──────────────┘
      │                                 │
      ▼                                 ▼
┌──────────────────────┐       ┌──────────────┐
│ 4. Validate Primary  │       │ Update swap  │
│    Slot Image        │       │ status       │
│    • Check magic     │◄──────┤ markers      │
│    • Verify signature│       └──────────────┘
│    • Check version   │
└──────────────────────┘
      │
      ├─────→ Valid? ──────No─────┐
      │                            │
      Yes                          ▼
      │                    ┌────────────────┐
      │                    │ 5. Fail-safe   │
      │                    │    • Try       │
      │                    │      secondary │
      │                    │    • Or halt   │
      │                    └────────────────┘
      ▼
┌──────────────────────┐
│ 6. Jump to           │
│    Application       │
│    • Setup stack     │
│    • Branch to entry │
└──────────────────────┘
      │
      ▼
┌──────────────────────┐
│ Application Running  │
│ (TF-M + NS App)      │
└──────────────────────┘
```

### 2.3 Image Slots

MCUboot uses a **slot-based** architecture:

**Primary Slot:**
- Contains the active (currently booting) firmware
- Always verified before boot
- Updated via swap from secondary

**Secondary Slot:**
- Staging area for new firmware
- Receives OTA/wired updates
- Swapped to primary when update is triggered

**Scratch Area:**
- Temporary storage during swap operations
- Size: typically one flash sector
- Only needed for "swap" mode

**Configuration Examples:**

```c
/* Overwrite Mode (No scratch needed) */
Primary:    [====ACTIVE_FW====]
Secondary:  [====UPDATE_FW====]

After Update:
Primary:    [====UPDATE_FW====]  // Overwrites old
Secondary:  [====undefined====]

/* Swap Mode (Requires scratch) */
Before:
Primary:    [====FW_V1.0======]
Secondary:  [====FW_V2.0======]
Scratch:    [================]

After Swap:
Primary:    [====FW_V2.0======]
Secondary:  [====FW_V1.0======]  // Preserved for rollback
Scratch:    [================]
```

### 2.4 Key MCUboot Concepts

**1. Image Magic**
- 16-byte identifier: `0x96f3b83d 0x5d3d1f10`
- Marks a valid MCUboot image
- Checked before verification

**2. Image Trailer**
- Located at end of slot
- Contains swap state information
- Magic numbers, swap type, copy done flags

**3. TLV (Type-Length-Value) Area**
- Located after image data
- Contains metadata:
  - Image hash (SHA256)
  - Signature (RSA/ECDSA/ED25519)
  - Protected TLVs (in signature)
  - Dependency information
  - Security counter (for rollback protection)

**4. Security Counter**
- Monotonic counter to prevent rollback
- Stored in flash or OTP
- Each image has a counter value
- Boot only if: `image_counter >= stored_counter`

---

## 3. Image Format and Signing

### 3.1 MCUboot Image Structure

```
┌─────────────────────────────────────────────────────┐
│  Image Header (32 bytes)                            │
│  ┌───────────────────────────────────────────────┐  │
│  │  Magic: 0x96f3b83d                           │  │
│  │  Load Address: 0x10000000                     │  │
│  │  Header Size: 0x200                           │  │
│  │  Protected TLV Size: 0x00                     │  │
│  │  Image Size: 0x20000                          │  │
│  │  Flags: 0x00                                  │  │
│  │  Version: 1.2.3.4                             │  │
│  │  Pad: ...                                     │  │
│  └───────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────┤
│  Padding (to Header Size)                           │
│  (Header Size - 32 bytes)                           │
├─────────────────────────────────────────────────────┤
│  Application Binary                                 │
│  • .text (code)                                     │
│  • .rodata (constants)                              │
│  • .data (initialized data)                         │
│  • ... (padded to alignment)                        │
├─────────────────────────────────────────────────────┤
│  Protected TLV Area (Optional)                      │
│  • Included in signature                            │
│  • Dependencies, custom data                        │
├─────────────────────────────────────────────────────┤
│  TLV Area                                           │
│  ┌───────────────────────────────────────────────┐  │
│  │  TLV Info (Magic: 0x6907)                     │  │
│  │  Total TLV Size                               │  │
│  ├───────────────────────────────────────────────┤  │
│  │  TLV: SHA256 Hash                             │  │
│  │  Type: 0x10, Len: 32                          │  │
│  │  Value: [32 bytes of hash]                    │  │
│  ├───────────────────────────────────────────────┤  │
│  │  TLV: Key Hash (for key revocation)           │  │
│  │  Type: 0x01, Len: 32                          │  │
│  │  Value: [32 bytes]                            │  │
│  ├───────────────────────────────────────────────┤  │
│  │  TLV: Signature (RSA/ECDSA/ED25519)           │  │
│  │  Type: 0x20, Len: varies                      │  │
│  │  Value: [signature bytes]                     │  │
│  ├───────────────────────────────────────────────┤  │
│  │  TLV: Dependency (optional)                   │  │
│  │  Type: 0x40, Len: 12                          │  │
│  │  Value: [image_id, version]                   │  │
│  ├───────────────────────────────────────────────┤  │
│  │  TLV: Security Counter (rollback protection)  │  │
│  │  Type: 0x50, Len: 4                           │  │
│  │  Value: [counter value]                       │  │
│  └───────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

### 3.2 Image Header Details

```c
struct image_header {
    uint32_t ih_magic;           /* 0x96f3b83d */
    uint32_t ih_load_addr;       /* Where to load image in RAM (optional) */
    uint16_t ih_hdr_size;        /* Header size (for future extensions) */
    uint16_t ih_protect_tlv_size;/* Protected TLV area size */
    uint32_t ih_img_size;        /* Image size (excluding header + TLV) */
    uint32_t ih_flags;           /* Image flags */
    struct image_version ih_ver; /* Image version */
    uint32_t _pad1;
};

struct image_version {
    uint8_t iv_major;            /* Major version */
    uint8_t iv_minor;            /* Minor version */
    uint16_t iv_revision;        /* Revision */
    uint32_t iv_build_num;       /* Build number */
};
```

**Example:**
```
Version 1.2.3.4:
  Major:    1
  Minor:    2
  Revision: 3
  Build:    4

Encoded as: 0x01020003, 0x00000004
```

### 3.3 Signing Process

**Step-by-Step Image Signing:**

#### Step 1: Generate Signing Keys

```bash
# Generate RSA-2048 key pair
imgtool keygen -k signing_key.pem -t rsa-2048

# Generate ECDSA-P256 key pair (smaller, faster)
imgtool keygen -k signing_key_ecdsa.pem -t ecdsa-p256

# Generate ED25519 key pair (smallest, fastest)
imgtool keygen -k signing_key_ed25519.pem -t ed25519

# Extract public key for embedding in bootloader
imgtool getpub -k signing_key.pem
```

**Key Storage Security:**
```
⚠️  CRITICAL: Protect Private Keys!

✓ Store in Hardware Security Module (HSM)
✓ Use key management service
✓ Encrypt at rest
✓ Restrict access
✓ Audit key usage
✗ NEVER commit to git
✗ NEVER share unencrypted
✗ NEVER use default/test keys in production
```

#### Step 2: Build Application Binary

```bash
# Build your application
cd build
cmake --build .

# Locate unsigned binary
# e.g., build/bin/tfm_s.bin
```

#### Step 3: Sign the Image

```bash
# Sign with imgtool
imgtool sign \
    --key signing_key.pem \
    --header-size 0x400 \
    --align 8 \
    --version 1.2.3 \
    --pad-header \
    --slot-size 0x80000 \
    tfm_s.bin \
    tfm_s_signed.bin

# Parameters explained:
# --key: Private key for signing
# --header-size: Reserved space for MCUboot header
# --align: Write alignment (flash requirement)
# --version: Image version (for rollback protection)
# --pad-header: Pad to full header size
# --slot-size: Maximum slot size (for padding)
# --security-counter: Rollback counter (optional)
# --dependencies: Image dependencies (optional)
```

**Advanced Signing:**

```bash
# Sign with security counter (rollback protection)
imgtool sign \
    --key signing_key.pem \
    --header-size 0x400 \
    --align 8 \
    --version 1.2.3 \
    --security-counter 5 \
    --pad-header \
    --slot-size 0x80000 \
    tfm_s.bin \
    tfm_s_v1.2.3_signed.bin

# Sign with dependencies (multi-image)
# Ensure image 1 (NS) requires at least image 0 (S) v1.0.0
imgtool sign \
    --key ns_signing_key.pem \
    --header-size 0x400 \
    --align 8 \
    --version 2.0.0 \
    --dependencies "(0,1.0.0+0)" \
    --pad-header \
    --slot-size 0x100000 \
    tfm_ns.bin \
    tfm_ns_signed.bin

# Dependencies format: "(image_id,version+0)"
# +0 means accept any version >= specified
```

#### Step 4: Verify Signed Image

```bash
# Display image information
imgtool verify \
    --key signing_key.pem \
    tfm_s_signed.bin

# Expected output:
# Image verified successfully!
# Image version: 1.2.3.4
# Image hash: a3f5...
# Security counter: 5
```

### 3.4 Signature Algorithms Comparison

| Algorithm | Signature Size | Verify Speed | Security | Recommended |
|-----------|----------------|--------------|----------|-------------|
| RSA-2048  | 256 bytes      | Fast         | Good     | ⚠️ Legacy   |
| RSA-3072  | 384 bytes      | Medium       | Better   | ⚠️ Large    |
| ECDSA-P256| 64-72 bytes    | Medium       | Good     | ✅ Balanced |
| ED25519   | 64 bytes       | Very Fast    | Excellent| ✅ Modern   |

**Recommendation:**
- **General use**: ECDSA-P256 or ED25519
- **Smallest size**: ED25519
- **Fastest verify**: ED25519
- **Legacy compatibility**: RSA-2048
- **Maximum security**: RSA-3072 or ECDSA-P384

### 3.5 Public Key Embedding

The bootloader must contain the public key(s) to verify signatures:

```c
/* Example: Embedded ECDSA-P256 public key */
/* Generated with: imgtool getpub -k signing_key.pem */

const unsigned char ecdsa_pub_key[] = {
    0x04,  /* Uncompressed point */
    /* X coordinate (32 bytes) */
    0x8d, 0x61, 0x75, 0x48, 0x3b, 0x35, 0xbe, 0xb8,
    0x72, 0x0c, 0xf8, 0xd3, 0x4d, 0xd0, 0x73, 0xd5,
    0xf0, 0x2d, 0x7c, 0x96, 0xa0, 0xe5, 0x85, 0x7c,
    0x4e, 0x01, 0x70, 0xb5, 0x94, 0xc7, 0x24, 0x29,
    /* Y coordinate (32 bytes) */
    0x59, 0x2a, 0xf7, 0xec, 0x92, 0x68, 0xe7, 0xb1,
    0x51, 0x0a, 0xb4, 0x4f, 0xc6, 0x00, 0x28, 0xd7,
    0xf4, 0x0e, 0x87, 0x90, 0xaf, 0xb0, 0xf8, 0xaa,
    0x9e, 0x48, 0x83, 0x8d, 0x38, 0x29, 0x6f, 0x5d
};

const unsigned int ecdsa_pub_key_len = sizeof(ecdsa_pub_key);
```

---

## 4. Swap Mechanisms

MCUboot supports multiple swap strategies for updating firmware.

### 4.1 Swap Modes

**1. Overwrite Mode**

```
Simplest mode - new image overwrites old

Before Update:
Primary:   [====FW_V1====]
Secondary: [====FW_V2====]

After Update:
Primary:   [====FW_V2====]  ← Copied from secondary
Secondary: [==garbage==]     ← No longer valid

Characteristics:
✓ Smallest footprint (no scratch needed)
✓ Fastest update
✗ No rollback capability
✗ No test mode
```

**Use Cases:**
- Memory-constrained devices
- Updates are always tested before deployment
- Rollback not required

**Configuration:**
```c
#define MCUBOOT_OVERWRITE_ONLY
```

---

**2. Swap Using Scratch**

```
Swaps primary and secondary using scratch area

Before:
Primary:   [====FW_V1====]
Secondary: [====FW_V2====]
Scratch:   [===========]

Swap Process (sector by sector):
Step 1: Primary[0] → Scratch
Step 2: Secondary[0] → Primary[0]
Step 3: Scratch → Secondary[0]
... repeat for all sectors ...

After Swap:
Primary:   [====FW_V2====]
Secondary: [====FW_V1====]  ← Old version preserved
Scratch:   [==garbage==]

Characteristics:
✓ Rollback supported
✓ Test mode (revert if not confirmed)
✗ Requires scratch area
✗ Slower than overwrite
```

**Use Cases:**
- Production devices
- Critical systems requiring rollback
- Test-before-commit updates

**Configuration:**
```c
#define MCUBOOT_SWAP_USING_SCRATCH
```

---

**3. Swap Using Move**

```
Swaps without scratch area using sector moves

Algorithm:
- Uses last sector of primary as temporary
- More complex swap algorithm
- No dedicated scratch needed

Characteristics:
✓ Rollback supported
✓ No scratch area needed
✗ More complex
✗ Slightly slower than scratch method
```

**Configuration:**
```c
#define MCUBOOT_SWAP_USING_MOVE
```

---

### 4.2 Swap States and Image Trailers

**Image Trailer Structure:**

Located at the end of each slot, stores swap state:

```
Trailer Layout (from end of slot, growing downward):

Offset from end:
-32:  [Magic: IMAGE_MAGIC]      (16 bytes) 0x96f3b83d5d3d1f10
-16:  [Swap Status]              (varies based on sector count)
-8:   [Copy Done]                (1 byte) + padding
-4:   [Image OK]                 (1 byte) + padding
```

**Swap Status Encoding:**

```c
#define IMAGE_MAGIC_NONE    0xffffffff  /* Erased flash */
#define IMAGE_MAGIC         0x96f3b83d  /* Good MCUboot image */
#define IMAGE_MAGIC_TEST    0x12345678  /* Test mode */

/* Swap type in trailer */
#define SWAP_TYPE_NONE      0x01  /* No swap needed */
#define SWAP_TYPE_TEST      0x02  /* Test swap (revert if not confirmed) */
#define SWAP_TYPE_PERM      0x03  /* Permanent swap */
#define SWAP_TYPE_REVERT    0x04  /* Revert swap */
#define SWAP_TYPE_FAIL      0x05  /* Failed swap */
```

**Swap Type State Machine:**

```
Initial State:
  Primary: SWAP_TYPE_NONE, Image OK
  Secondary: Empty

Update Downloaded to Secondary:
  Primary: SWAP_TYPE_NONE, Image OK
  Secondary: SWAP_TYPE_TEST (or PERM)

After Test Swap:
  Primary: New image, SWAP_TYPE_REVERT
  Secondary: Old image, marked for revert

If Application Confirms (image_ok):
  Primary: SWAP_TYPE_NONE, Image OK
  Secondary: Old image preserved

If Application DOESN'T Confirm:
  Reboot triggers revert swap
  Primary: Old image restored
  Secondary: Failed new image
```

### 4.3 Swap Process Details

**Detailed Swap-Using-Scratch Algorithm:**

```
Assumptions:
- Primary has 4 sectors: P0, P1, P2, P3
- Secondary has 4 sectors: S0, S1, S2, S3
- Scratch has 1 sector: SCR

Step-by-Step Swap:

1. Copy P0 → SCR
   Primary:   [  , P1, P2, P3]
   Secondary: [S0, S1, S2, S3]
   Scratch:   [P0]

2. Copy S0 → P0, Mark swap status
   Primary:   [S0, P1, P2, P3]
   Secondary: [  , S1, S2, S3]
   Scratch:   [P0]
   Status:    Sector 0 swapped ✓

3. Copy SCR → S0
   Primary:   [S0, P1, P2, P3]
   Secondary: [P0, S1, S2, S3]
   Scratch:   [  ]

4. Repeat for P1/S1...
   ... (sectors 1, 2, 3) ...

Final State:
   Primary:   [S0, S1, S2, S3]  ← New image
   Secondary: [P0, P1, P2, P3]  ← Old image
   Scratch:   [  ]

If power loss occurs during swap:
- Check swap status markers
- Resume swap from last completed sector
- Ensures atomic swap operation
```

**Power-Loss Resilience:**

MCUboot is designed to handle power loss at any point:

```
Swap Interrupted:
Primary:   [S0, S1, P2, P3]  ← Partially swapped
Secondary: [P0, P1, S2, S3]
Status:    Sector 0 ✓, Sector 1 ✓, Sector 2 ✗

On Next Boot:
1. Read swap status
2. Detect incomplete swap (sector 2 not swapped)
3. Resume swap from sector 2
4. Complete swap operation
5. Boot new image
```

### 4.4 Test Mode and Confirmation

**Test Mode Workflow:**

```
1. Download New Firmware to Secondary

2. Mark for Test Swap:
   Write SWAP_TYPE_TEST to secondary trailer

3. Reboot

4. MCUboot performs swap
   Primary ← Secondary (new image)
   Secondary ← Primary (old image)
   Primary trailer: SWAP_TYPE_REVERT

5. New Image Boots

6. Application Tests Itself:
   ✓ Check sensors
   ✓ Test connectivity
   ✓ Verify functionality

7. If tests pass:
   Call boot_set_confirmed()
   → Writes IMAGE_OK to trailer
   → Makes update permanent

8. If tests fail (or timeout):
   Reboot without confirming
   → MCUboot sees no IMAGE_OK
   → Reverts to old image from secondary
```

**Application Code for Confirmation:**

```c
#include "bootutil/bootutil.h"
#include "bootutil/image.h"

void application_startup(void) {
    struct boot_rsp rsp;
    int rc;

    /* Check if this is first boot after update */
    rc = boot_read_swap_state_by_id(FLASH_AREA_IMAGE_PRIMARY(0), &state);

    if (state == BOOT_SWAP_TYPE_REVERT) {
        printf("New image detected, running tests...\n");

        /* Run self-tests */
        bool tests_passed = run_system_tests();

        if (tests_passed) {
            printf("Tests passed! Confirming image...\n");

            /* Confirm this image is good */
            boot_set_confirmed();

            printf("Image confirmed. Update successful!\n");
        } else {
            printf("Tests failed! Image will revert on next boot.\n");
            /* Don't confirm - reboot will revert */
        }
    }
}

bool run_system_tests(void) {
    /* Example tests */
    if (!test_crypto_functionality()) return false;
    if (!test_sensor_readings()) return false;
    if (!test_network_connectivity()) return false;
    if (!test_secure_storage()) return false;

    return true;  /* All tests passed */
}
```

---

## 5. Rollback Protection

Rollback protection prevents attackers from downgrading firmware to vulnerable versions.

### 5.1 Why Rollback Protection?

**Attack Scenario:**

```
Timeline:
v1.0.0 - Released
       - Vulnerability discovered (CVE-1234)
v1.0.1 - Security patch released
       - Attacker has exploit for CVE-1234

Without Rollback Protection:
Attacker → Downgrades device to v1.0.0
        → Exploits CVE-1234
        → Gains control

With Rollback Protection:
Attacker → Attempts downgrade to v1.0.0
        → MCUboot rejects (version < current)
        → Attack prevented ✓
```

### 5.2 Security Counter Mechanism

**Concept:**

Each firmware version has a monotonic security counter. The bootloader stores the highest counter value seen and refuses to boot images with lower counters.

```
Firmware Versions and Security Counters:

v1.0.0 → Security Counter: 1
v1.0.1 → Security Counter: 2  (security fix)
v1.1.0 → Security Counter: 2  (new features, same security level)
v1.1.1 → Security Counter: 3  (another security fix)

Boot Decision:
Stored Counter = 3

Can Boot:
v1.1.1 (counter 3) ✓
v2.0.0 (counter 4) ✓

Cannot Boot:
v1.1.0 (counter 2) ✗
v1.0.1 (counter 2) ✗
v1.0.0 (counter 1) ✗
```

### 5.3 Implementation

**1. Embed Security Counter in Image:**

```bash
# Sign image with security counter
imgtool sign \
    --key signing_key.pem \
    --version 1.1.1 \
    --security-counter 3 \
    --header-size 0x400 \
    --slot-size 0x80000 \
    app.bin \
    app_signed.bin
```

**2. MCUboot Configuration:**

```c
/* Enable security counter checking */
#define MCUBOOT_HW_ROLLBACK_PROT

/* Configure counter storage */
#define MCUBOOT_SECURITY_COUNTER_FLASH  /* Store in flash */
/* OR */
#define MCUBOOT_SECURITY_COUNTER_OTP    /* Store in OTP (better) */
```

**3. Counter Storage:**

```
Option A: Flash-based (Easier, less secure)
- Store in internal flash
- Protected by flash write permissions
- Can be attacked with physical access

Option B: OTP-based (Harder, more secure)
- One-Time Programmable memory
- Cannot be erased or decremented
- Maximum security

Option C: External Secure Element
- Store in dedicated security chip (e.g., ATECC608)
- Tamper-resistant
- Highest security
```

**4. Counter Update Flow:**

```
Boot Process:

1. MCUboot reads image from primary slot
2. Extracts security counter from TLV
   Image Counter = 3

3. Reads stored counter from flash/OTP
   Stored Counter = 2

4. Comparison:
   if (Image Counter >= Stored Counter) {
       /* Image is acceptable */
       verify_signature();
       if (valid) {
           /* Update stored counter */
           if (Image Counter > Stored Counter) {
               write_counter(Image Counter);  // 2 → 3
           }
           boot_image();
       }
   } else {
       /* Rollback attempt detected! */
       reject_image();
       boot_recovery();
   }
```

### 5.4 Counter Management Strategy

**Best Practices:**

```
Security Counter Policy:

1. Increment counter ONLY for security updates
   v1.0.0 → v1.0.1 (CVE fix)    → Counter: 1 → 2 ✓
   v1.0.1 → v1.1.0 (features)   → Counter: 2 → 2 (same) ✓
   v1.1.0 → v1.1.1 (CVE fix)    → Counter: 2 → 3 ✓

2. Document counter values
   CHANGELOG.md:
   v1.1.1 (Security Counter: 3)
   - Fixed CVE-2024-1234
   - Added new features

3. Plan counter space
   32-bit counter = 4,294,967,295 updates
   Even 1 update/day = 11,780 years ✓

4. Never decrement
   Once incremented, cannot go back
   Plan carefully before incrementing
```

---

## 6. Encrypted Images

MCUboot supports encrypted firmware images for additional confidentiality.

### 6.1 Why Encrypt Firmware?

**Threat Model:**

```
Without Encryption:
Attacker intercepts OTA update
    → Extracts firmware binary
    → Reverse engineers code
    → Finds vulnerabilities
    → Discovers crypto keys hardcoded
    → Extracts proprietary algorithms

With Encryption:
Attacker intercepts OTA update
    → Encrypted binary (AES-256)
    → Cannot read code
    → Cannot extract secrets ✓
```

**Use Cases:**
- Protect intellectual property
- Prevent reverse engineering
- Comply with export regulations
- Hide cryptographic implementations
- Protect embedded secrets

### 6.2 Encryption Modes

MCUboot supports two encryption modes:

**1. RSA-OAEP Encryption:**

```
Algorithm:
1. Generate random AES key (TLK - Transport Layer Key)
2. Encrypt firmware with AES-128-CTR using TLK
3. Encrypt TLK with RSA public key (RSA-OAEP)
4. Embed encrypted TLK in image TLV

Decryption (by MCUboot):
1. Extract encrypted TLK from TLV
2. Decrypt TLK using RSA private key
3. Decrypt firmware using decrypted TLK
4. Verify signature
5. Boot decrypted image

Key Management:
- RSA key pair (2048 or 3072 bits)
- RSA private key stays on device (secure storage)
- RSA public key used for encryption (can be public)
```

**2. AES-KW Encryption (Key Wrapping):**

```
Algorithm:
1. Device has pre-shared AES-256 KEK (Key Encryption Key)
2. Generate random AES-128 TLK
3. Encrypt firmware with AES-128-CTR using TLK
4. Wrap TLK with KEK using AES-KW (RFC 3394)
5. Embed wrapped TLK in image TLV

Decryption (by MCUboot):
1. Extract wrapped TLK from TLV
2. Unwrap TLK using KEK
3. Decrypt firmware using TLK
4. Verify signature
5. Boot decrypted image

Key Management:
- KEK provisioned during manufacturing
- Stored in secure storage (crypto accelerator, OTP, etc.)
- Never leaves device
```

### 6.3 Creating Encrypted Images

**Generate Encryption Keys:**

```bash
# Generate RSA key pair for encryption
imgtool keygen -k encrypt_key_rsa.pem -t rsa-2048

# Generate AES-128 KEK for AES-KW
# (Random 128-bit key, hex-encoded)
openssl rand -hex 16 > kek.txt
# Example: 2b7e151628aed2a6abf7158809cf4f3c
```

**Encrypt Image:**

```bash
# Method 1: RSA-OAEP Encryption
imgtool sign \
    --key signing_key.pem \        # For signature
    --encrypt encrypt_key_rsa.pem \ # For encryption
    --header-size 0x400 \
    --align 8 \
    --version 1.2.3 \
    --slot-size 0x80000 \
    app.bin \
    app_encrypted_signed.bin

# Method 2: AES-KW Encryption
imgtool sign \
    --key signing_key.pem \
    --encrypt kek.txt \             # KEK in hex
    --encrypt-keylen 128 \          # KEK length
    --header-size 0x400 \
    --align 8 \
    --version 1.2.3 \
    --slot-size 0x80000 \
    app.bin \
    app_aes_encrypted_signed.bin
```

**Encrypted Image Structure:**

```
┌─────────────────────────────────────────────┐
│  Image Header                               │
│  • Flags: ENCRYPTED_AES128                  │
├─────────────────────────────────────────────┤
│  Encrypted Application Binary               │
│  (AES-128-CTR encrypted)                    │
├─────────────────────────────────────────────┤
│  TLV Area                                   │
│  ┌───────────────────────────────────────┐  │
│  │  TLV: Encrypted TLK                   │  │
│  │  Type: 0x30                           │  │
│  │  Len: 256 bytes (RSA-2048)            │  │
│  │  Value: RSA-OAEP(TLK)                 │  │
│  ├───────────────────────────────────────┤  │
│  │  TLV: SHA256 (of encrypted data)      │  │
│  │  Type: 0x10, Len: 32                  │  │
│  ├───────────────────────────────────────┤  │
│  │  TLV: Signature (of encrypted data)   │  │
│  │  Type: 0x20, Len: varies              │  │
│  └───────────────────────────────────────┘  │
└─────────────────────────────────────────────┘

Note: Signature is over ENCRYPTED data
      Ensures integrity of ciphertext
```

### 6.4 MCUboot Decryption Configuration

**Enable Encryption Support:**

```c
/* In MCUboot configuration */

/* Enable image encryption support */
#define MCUBOOT_ENCRYPT_RSA
/* OR */
#define MCUBOOT_ENCRYPT_KW

/* Specify key location */
#define MCUBOOT_ENC_KEY_LEN 256  /* RSA-2048 = 256 bytes */
```

**Embed Decryption Key:**

```c
/* For RSA: Embed private key in MCUboot */
/* Generated with: imgtool getpriv -k encrypt_key_rsa.pem */

const unsigned char enc_priv_key[] = {
    /* RSA private key in DER format */
    0x30, 0x82, 0x04, 0xa4, ...
};

/* For AES-KW: Store KEK in secure storage */
/* Typically provisioned during manufacturing */
/* Retrieved from crypto accelerator or secure element */

psa_status_t get_kek(uint8_t *kek, size_t kek_len) {
    /* Read from secure storage */
    return psa_its_get(UID_KEK, 0, kek_len, kek, NULL);
}
```

### 6.5 Security Considerations

**Key Security:**

```
⚠️  CRITICAL: Encryption Key Protection!

RSA Method:
✓ Private key must be protected in bootloader
✓ Bootloader must be immutable (ROM or locked flash)
✓ Use HW crypto accelerator if available
⚠️  Key is in device, advanced attacks possible

AES-KW Method:
✓ KEK provisioned per-device (unique)
✓ Store KEK in crypto accelerator (e.g., TrustZone, secure element)
✓ KEK never accessible to application
✓ Better security than RSA method

Best Practice:
✓ Use hardware crypto acceleration
✓ Store keys in tamper-resistant storage
✓ Use per-device unique keys
✓ Combine encryption + signing
✓ Rotate keys periodically
```

**Encryption vs Signing:**

```
Signing:      Ensures AUTHENTICITY and INTEGRITY
              "This firmware came from the manufacturer"

Encryption:   Ensures CONFIDENTIALITY
              "This firmware cannot be read by attacker"

Best Practice: Use BOTH
┌─────────────────────────────────────────┐
│  Firmware Update Process                │
├─────────────────────────────────────────┤
│  1. Encrypt firmware (confidentiality)  │
│  2. Sign encrypted firmware (integrity) │
│  3. Transmit to device                  │
│  4. Verify signature (MCUboot)          │
│  5. Decrypt firmware (MCUboot)          │
│  6. Boot decrypted image                │
└─────────────────────────────────────────┘
```

---

## 7. Multi-Image Boot

MCUboot can manage multiple images, essential for TF-M which has separate secure and non-secure images.

### 7.1 Multi-Image Concept

**TF-M Typical Configuration:**

```
Flash Layout:

┌──────────────────────────────────────────┐
│  MCUboot Bootloader                      │
├──────────────────────────────────────────┤
│  Image 0 Primary Slot (Secure - TF-M)   │
│  ┌────────────────────────────────────┐  │
│  │  Image Header (ID: 0)              │  │
│  │  TF-M Secure Firmware              │  │
│  │  TLV + Signature                   │  │
│  └────────────────────────────────────┘  │
├──────────────────────────────────────────┤
│  Image 1 Primary Slot (Non-Secure App)  │
│  ┌────────────────────────────────────┐  │
│  │  Image Header (ID: 1)              │  │
│  │  Application Firmware              │  │
│  │  TLV + Signature                   │  │
│  └────────────────────────────────────┘  │
├──────────────────────────────────────────┤
│  Image 0 Secondary Slot (S Update)      │
├──────────────────────────────────────────┤
│  Image 1 Secondary Slot (NS Update)     │
├──────────────────────────────────────────┤
│  Scratch Area                            │
└──────────────────────────────────────────┘
```

### 7.2 Image Dependencies

Images can declare dependencies on other images to ensure compatibility.

**Example:**
```
Non-Secure App v2.0 requires TF-M Secure v1.5+

Dependency Chain:
Image 1 (NS App v2.0)
    └─→ Depends on: Image 0 (TF-M S v1.5+)

Boot Decision:
if (Image0_version >= 1.5.0) {
    boot(Image1_v2.0);  ✓
} else {
    reject(Image1_v2.0);  ✗
}
```

**Encoding Dependencies:**

```bash
# Sign NS image with dependency on S image
imgtool sign \
    --key ns_signing_key.pem \
    --version 2.0.0 \
    --dependencies "(0,1.5.0+0)" \
    --header-size 0x400 \
    --slot-size 0x100000 \
    ns_app.bin \
    ns_app_signed.bin

# Dependency format: "(image_id,min_version+flags)"
# Flags:
#   +0: Accept version >= min_version
#   +1: Require exact version match
```

**TLV Encoding:**

```
TLV: Dependency
  Type: 0x40
  Length: 12 bytes
  Value:
    Image ID: 0 (1 byte)
    Padding: (3 bytes)
    Version: 1.5.0.0 (8 bytes)
      Major: 1
      Minor: 5
      Revision: 0
      Build: 0
```

### 7.3 Multi-Image Boot Flow

**Boot Process:**

```
MCUboot Start
      │
      ▼
┌─────────────────────────┐
│ 1. Validate Image 0     │
│    (Secure - TF-M)      │
│    • Check magic        │
│    • Verify signature   │
│    • Check version      │
└─────────────────────────┘
      │
      ├─ Valid? ──No──→ FAIL: Cannot boot
      │
      Yes
      ▼
┌─────────────────────────┐
│ 2. Validate Image 1     │
│    (Non-Secure App)     │
│    • Check magic        │
│    • Verify signature   │
│    • Check version      │
│    • Check dependencies │
└─────────────────────────┘
      │
      ├─ Dependencies OK? ──No──→ FAIL: Dependency mismatch
      │
      Yes
      ▼
┌─────────────────────────┐
│ 3. Check Update Status  │
│    • Swap Image 0?      │
│    • Swap Image 1?      │
└─────────────────────────┘
      │
      ├─ Swap Needed? ──Yes──→ Perform Swap(s)
      │                              │
      No                             │
      │                              │
      └──────────────────────────────┘
      │
      ▼
┌─────────────────────────┐
│ 4. Boot Image 0         │
│    Jump to TF-M entry   │
└─────────────────────────┘
      │
      ▼
┌─────────────────────────┐
│ TF-M Initializes        │
│ Then boots Image 1      │
│ (NS Application)        │
└─────────────────────────┘
```

**Update Scenarios:**

```
Scenario 1: Update both images
  Secondary 0: TF-M v1.6
  Secondary 1: NS App v2.1 (depends on TF-M >= 1.5)

  MCUboot:
    Swap Image 0 (TF-M 1.5 → 1.6) ✓
    Check dependency: 1.6 >= 1.5 ✓
    Swap Image 1 (NS App 2.0 → 2.1) ✓
    Boot both new images ✓

Scenario 2: Update NS only
  Secondary 0: (empty)
  Secondary 1: NS App v2.2 (depends on TF-M >= 1.6)

  MCUboot:
    No swap for Image 0
    Check dependency: Current TF-M is 1.6 >= 1.6 ✓
    Swap Image 1 (2.1 → 2.2) ✓
    Boot ✓

Scenario 3: Dependency violation
  Secondary 0: (empty)
  Secondary 1: NS App v3.0 (depends on TF-M >= 2.0)

  MCUboot:
    No swap for Image 0
    Check dependency: Current TF-M is 1.6 < 2.0 ✗
    Reject Image 1 update ✗
    Boot old Image 1 (v2.2) ✓
```

### 7.4 Configuration

```c
/* MCUboot configuration for multi-image */

/* Number of images */
#define MCUBOOT_IMAGE_NUMBER 2

/* Image 0: Secure (TF-M) */
#define FLASH_AREA_IMAGE_PRIMARY(x)   /* Image x primary slot */
#define FLASH_AREA_IMAGE_SECONDARY(x) /* Image x secondary slot */

/* Flash areas per image */
#define FLASH_AREA_0_OFFSET  0x00020000  /* Image 0 primary */
#define FLASH_AREA_0_SIZE    0x00080000  /* 512 KB */
#define FLASH_AREA_1_OFFSET  0x000A0000  /* Image 1 primary */
#define FLASH_AREA_1_SIZE    0x00100000  /* 1 MB */
#define FLASH_AREA_2_OFFSET  0x001A0000  /* Image 0 secondary */
#define FLASH_AREA_2_SIZE    0x00080000  /* 512 KB */
#define FLASH_AREA_3_OFFSET  0x00220000  /* Image 1 secondary */
#define FLASH_AREA_3_SIZE    0x00100000  /* 1 MB */
#define FLASH_AREA_SCRATCH_OFFSET 0x00320000
#define FLASH_AREA_SCRATCH_SIZE   0x00010000  /* 64 KB */
```

---

## 8. Integration with TF-M

### 8.1 TF-M Boot Architecture

**Complete TF-M Boot Chain:**

```
┌──────────────────────────────────────────┐
│  Stage 1: MCUboot (BL2)                  │
│  • Immutable or updatable                │
│  • Verifies TF-M Secure image            │
│  • Verifies NS application image         │
│  • Checks dependencies                   │
│  • Handles updates (swap)                │
│  • Jumps to TF-M                         │
└──────────────────────────────────────────┘
               │
               ▼
┌──────────────────────────────────────────┐
│  Stage 2: TF-M Secure (SPE)              │
│  • Initialize Secure world               │
│  • Set up TrustZone (SAU/MPU)            │
│  • Initialize secure services            │
│  • Set up non-secure entry points        │
│  • Jump to NS application                │
└──────────────────────────────────────────┘
               │
               ▼
┌──────────────────────────────────────────┐
│  Stage 3: Application (NSPE)             │
│  • Normal application code               │
│  • Calls PSA APIs for secure services    │
└──────────────────────────────────────────┘
```

### 8.2 Building TF-M with MCUboot

**CMake Configuration:**

```bash
cmake .. \
    -DTFM_PLATFORM=<platform> \
    -DTFM_TOOLCHAIN_FILE=../toolchain_GNUARM.cmake \
    -DBL2=ON \                          # Enable MCUboot
    -DMCUBOOT_IMAGE_NUMBER=2 \          # Secure + NS images
    -DMCUBOOT_UPGRADE_STRATEGY="SWAP_USING_SCRATCH" \
    -DMCUBOOT_SIGNATURE_TYPE="RSA-2048" \  # or ECDSA-P256, ED25519
    -DMCUBOOT_HW_ROLLBACK_PROT=ON \     # Enable rollback protection
    -DMCUBOOT_ENC_IMAGES=ON \           # Enable encryption (optional)
    -DMCUBOOT_ENCRYPT_RSA=ON            # Encryption method

cmake --build .
```

**Build Output:**

```
build/bin/
├── bl2.axf                   # MCUboot bootloader
├── bl2.bin
├── tfm_s.axf                 # TF-M secure (unsigned)
├── tfm_s.bin
├── tfm_s_signed.bin          # TF-M secure (signed)
├── tfm_ns.axf                # NS application (unsigned)
├── tfm_ns.bin
├── tfm_ns_signed.bin         # NS application (signed)
└── tfm_s_ns_signed.bin       # Combined signed image (for initial flash)
```

### 8.3 Memory Layout

**Typical TF-M + MCUboot Layout:**

```
Flash (2 MB):

0x00000000  ┌─────────────────────────────────┐
            │  MCUboot (BL2)                  │  64 KB
0x00010000  ├─────────────────────────────────┤
            │  Scratch Area                   │  64 KB
0x00020000  ├─────────────────────────────────┤
            │  Image 0 Primary (TF-M Secure)  │  512 KB
            │  • Image header                 │
            │  • TF-M SPM + Services          │
            │  • TLV + Signature              │
0x000A0000  ├─────────────────────────────────┤
            │  Image 1 Primary (NS App)       │  512 KB
            │  • Image header                 │
            │  • Application code             │
            │  • TLV + Signature              │
0x00120000  ├─────────────────────────────────┤
            │  Image 0 Secondary (TF-M Update)│  512 KB
0x001A0000  ├─────────────────────────────────┤
            │  Image 1 Secondary (NS Update)  │  512 KB
0x00220000  ├─────────────────────────────────┤
            │  PS Storage                     │
0x00240000  ├─────────────────────────────────┤
            │  ITS Storage                    │
0x00260000  └─────────────────────────────────┘

RAM (256 KB):

0x20000000  ┌─────────────────────────────────┐
            │  Secure RAM                     │  128 KB
            │  • TF-M data                    │
            │  • Secure stacks                │
0x20020000  ├─────────────────────────────────┤
            │  Non-Secure RAM                 │  128 KB
            │  • Application data             │
            │  • NS stacks                    │
0x20040000  └─────────────────────────────────┘
```

### 8.4 Firmware Update Workflow

**Complete Update Process:**

```
Step 1: Build New Firmware
────────────────────────────
$ cd build
$ cmake --build .
$ ls bin/
  tfm_s_v1.2.0_signed.bin
  tfm_ns_v2.1.0_signed.bin

Step 2: Transfer to Device
────────────────────────────
Via UART, USB, BLE, Wi-Fi, etc.

Application receives update:
write_to_flash(SECONDARY_SLOT_0, tfm_s_v1.2.0_signed.bin);
write_to_flash(SECONDARY_SLOT_1, tfm_ns_v2.1.0_signed.bin);

Step 3: Mark for Update
────────────────────────────
#include "bootutil/bootutil.h"

/* Set Image 0 for test swap */
boot_set_pending(0);  /* Image 0 */

/* Set Image 1 for test swap */
boot_set_pending(1);  /* Image 1 */

/* Or use FWU service in TF-M */
psa_fwu_request_reboot();

Step 4: Reboot
────────────────────────────
NVIC_SystemReset();

Step 5: MCUboot Swap
────────────────────────────
MCUboot detects pending images
Performs swap for both images
Boots new firmware in test mode

Step 6: Application Confirms
────────────────────────────
void app_startup(void) {
    /* Run self-tests */
    if (system_health_check()) {
        /* Confirm update */
        boot_set_confirmed();
    }
    /* If not confirmed, will revert on next boot */
}

Step 7: Update Complete
────────────────────────────
Device running new firmware
Old firmware preserved in secondary for rollback
```

---

This guide continues with sections 9 (Hands-On Labs) and 10 (Advanced Topics). Would you like me to:

1. Complete the MCUboot guide with the labs section
2. Move on to creating the STM32U5 project guide
3. Create the NRF52840 project guide

Which would you prefer next?

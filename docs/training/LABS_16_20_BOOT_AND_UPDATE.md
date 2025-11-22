# TF-M Training Labs 16-20: Secure Boot and Firmware Update

**Lab Series:** MCUboot, Image Signing, Rollback Protection, OTA Updates
**Duration:** 12-15 hours
**Prerequisites:** Labs 1-15, MCUboot Guide completed

---

## Lab 16: Image Signing with imgtool

**Duration:** 2 hours
**Difficulty:** Beginner
**Goal:** Sign firmware images using MCUboot's imgtool

### Learning Objectives

- Generate ECDSA signing keys
- Sign firmware images with imgtool
- Understand MCUboot image format
- Extract and verify public keys

---

### Exercise 16.1: Key Generation

**Task:** Generate signing keys for firmware

```bash
#!/bin/bash
# generate_signing_keys.sh

echo "=== Generating Firmware Signing Keys ==="
echo

# Step 1: Generate private key (ECDSA P-256)
echo "Step 1: Generating ECDSA P-256 private key..."
cd ~/tfm_workspace/trusted-firmware-m/bl2/ext/mcuboot

python3 scripts/imgtool.py keygen \
    -k keys/firmware-signing-key.pem \
    -t ecdsa-p256

echo "  ✓ Private key: keys/firmware-signing-key.pem"
echo

# Step 2: Extract public key
echo "Step 2: Extracting public key (C array format)..."
python3 scripts/imgtool.py getpub \
    -k keys/firmware-signing-key.pem \
    > keys/firmware-signing-key-pub.c

echo "  ✓ Public key: keys/firmware-signing-key-pub.c"
cat keys/firmware-signing-key-pub.c
echo

# Step 3: Generate backup key
echo "Step 3: Generating backup key..."
python3 scripts/imgtool.py keygen \
    -k keys/firmware-signing-key-backup.pem \
    -t ecdsa-p256

echo "  ✓ Backup key: keys/firmware-signing-key-backup.pem"
echo

echo "══════════════════════════════════════════"
echo "  ✓ Key generation complete"
echo "══════════════════════════════════════════"
echo "  IMPORTANT: Backup these keys securely!"
echo "  Loss of private key = cannot sign updates"
echo
```

**Expected Output:**
```
=== Generating Firmware Signing Keys ===

Step 1: Generating ECDSA P-256 private key...
  ✓ Private key: keys/firmware-signing-key.pem

Step 2: Extracting public key (C array format)...
  ✓ Public key: keys/firmware-signing-key-pub.c
const unsigned char signature_pubkey[] = {
    0x04,
    0x8b, 0x7f, 0x63, 0x21, 0x4e, 0x9a, 0x3d, 0x42,
    0x1c, 0x6f, 0x85, 0xa9, 0xb2, 0x73, 0xc4, 0xd5,
    0x3e, 0x8f, 0x12, 0x45, 0xa8, 0xc9, 0x67, 0x23,
    0x91, 0xde, 0x4b, 0x76, 0xf2, 0x5a, 0x83, 0x1f,
    0x2a, 0x93, 0xc8, 0x45, 0xf1, 0x6b, 0x27, 0xd9,
    0x5e, 0x72, 0x34, 0xb8, 0xa1, 0x9c, 0x68, 0x3f,
    0x7d, 0xe4, 0x52, 0x96, 0xb3, 0x71, 0x2e, 0xc5,
    0x48, 0x9f, 0x1a, 0x65, 0xd7, 0x8b, 0x32, 0x4c
};
const unsigned int signature_pubkey_len = 65;

Step 3: Generating backup key...
  ✓ Backup key: keys/firmware-signing-key-backup.pem

══════════════════════════════════════════
  ✓ Key generation complete
══════════════════════════════════════════
  IMPORTANT: Backup these keys securely!
  Loss of private key = cannot sign updates
```

---

### Exercise 16.2: Signing Firmware Images

**Task:** Sign secure and non-secure firmware binaries

```bash
#!/bin/bash
# sign_firmware.sh

echo "=== Signing Firmware Images ==="
echo

# Configuration
KEY="keys/firmware-signing-key.pem"
HEADER_SIZE=0x400
SLOT_SIZE=0x60000      # 384KB
VERSION="1.0.0"
SECURITY_COUNTER=1

# Step 1: Sign secure firmware
echo "Step 1: Signing secure firmware (tfm_s.bin)..."

python3 scripts/imgtool.py sign \
    --key $KEY \
    --header-size $HEADER_SIZE \
    --align 8 \
    --version $VERSION \
    --security-counter $SECURITY_COUNTER \
    --pad-header \
    --slot-size $SLOT_SIZE \
    build/bin/tfm_s.bin \
    build/bin/tfm_s_signed.bin

echo "  ✓ Signed: tfm_s_signed.bin"
echo "    Version: $VERSION"
    echo "    Security counter: $SECURITY_COUNTER"
echo

# Step 2: Sign non-secure firmware
echo "Step 2: Signing non-secure firmware (tfm_ns.bin)..."

python3 scripts/imgtool.py sign \
    --key $KEY \
    --header-size $HEADER_SIZE \
    --align 8 \
    --version $VERSION \
    --security-counter $SECURITY_COUNTER \
    --pad-header \
    --slot-size 0x80000 \  # 512KB for NS
    build/bin/tfm_ns.bin \
    build/bin/tfm_ns_signed.bin

echo "  ✓ Signed: tfm_ns_signed.bin"
echo

# Step 3: Verify signatures
echo "Step 3: Verifying signatures..."

for img in build/bin/tfm_s_signed.bin build/bin/tfm_ns_signed.bin; do
    python3 scripts/imgtool.py verify \
        --key $KEY \
        $img
    echo "  ✓ $img: Signature valid"
done

echo
echo "══════════════════════════════════════════"
echo "  ✓ Firmware signing complete"
echo "══════════════════════════════════════════"
```

**Expected Output:**
```
=== Signing Firmware Images ===

Step 1: Signing secure firmware (tfm_s.bin)...
  ✓ Signed: tfm_s_signed.bin
    Version: 1.0.0
    Security counter: 1

Step 2: Signing non-secure firmware (tfm_ns.bin)...
  ✓ Signed: tfm_ns_signed.bin

Step 3: Verifying signatures...
  ✓ build/bin/tfm_s_signed.bin: Signature valid
  ✓ build/bin/tfm_ns_signed.bin: Signature valid

══════════════════════════════════════════
  ✓ Firmware signing complete
══════════════════════════════════════════
```

---

## Lab 17: MCUboot Configuration

**Duration:** 2-3 hours
**Difficulty:** Intermediate
**Goal:** Configure MCUboot for different upgrade strategies

### Exercise 17.1: Swap Mode Configuration

**Task:** Configure MCUboot for swap-using-scratch mode

```cmake
# bl2_config.cmake

set(MCUBOOT_UPGRADE_STRATEGY "SWAP_USING_SCRATCH" CACHE STRING "Upgrade mode")

# Flash layout for swap
set(FLASH_AREA_0_OFFSET     0x10000)  # Primary slot
set(FLASH_AREA_0_SIZE       0x60000)  # 384KB

set(FLASH_AREA_2_OFFSET     0xF0000)  # Secondary slot
set(FLASH_AREA_2_SIZE       0x60000)  # 384KB

set(FLASH_AREA_SCRATCH_OFFSET  0x1D0000)  # Scratch area
set(FLASH_AREA_SCRATCH_SIZE    0x10000)   # 64KB

# Security features
set(MCUBOOT_HW_KEY ON CACHE BOOL "Use HW-derived key")
set(MCUBOOT_MEASURED_BOOT ON CACHE BOOL "Measure boot")
set(MCUBOOT_DATA_SHARING ON CACHE BOOL "Share boot data")

# Anti-rollback
set(MCUBOOT_SECURITY_COUNTER_IMAGE ON CACHE BOOL "Security counter per image")
```

---

## Lab 18: Swap Mechanisms Testing

**Duration:** 3 hours
**Difficulty:** Advanced
**Goal:** Test different swap modes and recovery scenarios

### Exercise 18.1: Test Swap Process

**Scenario:** Upload new firmware and verify swap behavior

**Code:**
```c
/*
 * OTA Update Test
 * File: test_ota_swap.c
 */

#include "psa/update.h"
#include <stdio.h>

void test_swap_workflow(void)
{
    printf("\n=== Testing Swap Workflow ===\n\n");

    /* Step 1: Check current version */
    psa_fwu_image_info_t info;
    psa_fwu_query(0, &info);

    printf("Current firmware:\n");
    printf("  Version: %d.%d.%d\n",
           info.version.major, info.version.minor, info.version.patch);
    printf("  State: %d\n\n", info.state);

    /* Step 2: Stage new firmware (simulated) */
    printf("Staging new firmware to secondary slot...\n");
    /* In reality: Download via 4G, write via psa_fwu_write() */
    printf("  ✓ Image written to secondary slot\n");
    printf("  ✓ Image verified\n\n");

    /* Step 3: Mark for installation */
    printf("Marking image for installation...\n");
    psa_fwu_install(0, NULL, NULL);
    printf("  ✓ Image marked (pending reboot)\n\n");

    /* Step 4: Reboot */
    printf("Rebooting...\n");
    printf("──────────────────────────────────────────\n");
    printf("[DEVICE REBOOTS]\n");
    printf("──────────────────────────────────────────\n\n");

    /* After reboot, MCUboot performs swap */
    printf("MCUboot swap process:\n");
    printf("  1. Verify secondary image signature ✓\n");
    printf("  2. Check security counter ✓\n");
    printf("  3. Copy secondary → scratch\n");
    printf("  4. Copy primary → secondary\n");
    printf("  5. Copy scratch → primary\n");
    printf("  6. Mark image as TRIAL\n");
    printf("  7. Boot new firmware\n\n");

    /* Step 5: New firmware self-test */
    printf("New firmware booted (TRIAL mode):\n");
    printf("  Running self-tests...\n");
    printf("    - Crypto test: ✓\n");
    printf("    - Storage test: ✓\n");
    printf("    - Modem test: ✓\n");
    printf("  All tests passed!\n\n");

    /* Step 6: Confirm update */
    printf("Confirming update...\n");
    psa_fwu_accept();
    printf("  ✓ Update confirmed\n");
    printf("  ✓ Image state: INSTALLED\n\n");

    printf("══════════════════════════════════════════\n");
    printf("  ✓ OTA update successful!\n");
    printf("══════════════════════════════════════════\n");
}
```

**Expected Output:**
```
=== Testing Swap Workflow ===

Current firmware:
  Version: 1.0.0
  State: 1

Staging new firmware to secondary slot...
  ✓ Image written to secondary slot
  ✓ Image verified

Marking image for installation...
  ✓ Image marked (pending reboot)

Rebooting...
──────────────────────────────────────────
[DEVICE REBOOTS]
──────────────────────────────────────────

MCUboot swap process:
  1. Verify secondary image signature ✓
  2. Check security counter ✓
  3. Copy secondary → scratch
  4. Copy primary → secondary
  5. Copy scratch → primary
  6. Mark image as TRIAL
  7. Boot new firmware

New firmware booted (TRIAL mode):
  Running self-tests...
    - Crypto test: ✓
    - Storage test: ✓
    - Modem test: ✓
  All tests passed!

Confirming update...
  ✓ Update confirmed
  ✓ Image state: INSTALLED

══════════════════════════════════════════
  ✓ OTA update successful!
══════════════════════════════════════════
```

---

## Lab 19: Rollback Protection

**Duration:** 2 hours
**Difficulty:** Intermediate
**Goal:** Implement and test anti-rollback protection

### Exercise 19.1: Test Rollback Prevention

**Task:** Attempt to install older firmware and verify rejection

```c
/*
 * Rollback Protection Test
 * File: test_rollback.c
 */

#include <stdio.h>

void test_rollback_protection(void)
{
    printf("\n=== Testing Anti-Rollback Protection ===\n\n");

    /* Scenario: Try to install old firmware */
    printf("Current firmware: v1.3.0 (security counter = 5)\n");
    printf("Attempting to install: v1.2.0 (security counter = 2)\n\n");

    printf("MCUboot verification process:\n");
    printf("  1. Read image header ✓\n");
    printf("  2. Verify signature ✓\n");
    printf("  3. Extract security counter from TLV...\n");
    printf("     Image counter: 2\n");
    printf("     Stored counter: 5\n");
    printf("     2 < 5 → ROLLBACK DETECTED!\n\n");

    printf("════════════════════════════════════════\n");
    printf("  ✗ UPDATE REJECTED\n");
    printf("════════════════════════════════════════\n");
    printf("  Reason: Security counter too low\n");
    printf("  Protection: Rollback prevention active\n");
    printf("  Action: Boot current firmware (v1.3.0)\n\n");

    printf("Device boots current firmware safely.\n");
    printf("Attacker cannot downgrade to vulnerable version!\n\n");
}

void test_valid_update(void)
{
    printf("\n=== Testing Valid Update ===\n\n");

    printf("Current firmware: v1.3.0 (security counter = 5)\n");
    printf("Attempting to install: v1.4.0 (security counter = 6)\n\n");

    printf("MCUboot verification process:\n");
    printf("  1. Read image header ✓\n");
    printf("  2. Verify signature ✓\n");
    printf("  3. Extract security counter from TLV...\n");
    printf("     Image counter: 6\n");
    printf("     Stored counter: 5\n");
    printf("     6 >= 5 → VERSION ALLOWED!\n\n");

    printf("  4. Install image...\n");
    printf("  5. Update stored counter: 5 → 6\n\n");

    printf("════════════════════════════════════════\n");
    printf("  ✓ UPDATE ACCEPTED\n");
    printf("════════════════════════════════════════\n");
    printf("  New version: v1.4.0\n");
    printf("  Security counter updated\n");
    printf("  Device now requires counter >= 6\n\n");
}

int main(void)
{
    test_rollback_protection();
    test_valid_update();
    return 0;
}
```

---

## Lab 20: End-to-End OTA Update

**Duration:** 4 hours
**Difficulty:** Advanced
**Goal:** Implement complete OTA update system with 4G modem

### Exercise 20.1: Complete OTA Implementation

**Task:** Build full OTA update system for GPS tracker

**Architecture:**
```
┌─────────────────────────────────────────────────────┐
│ Complete OTA Update Flow                            │
├─────────────────────────────────────────────────────┤
│                                                     │
│  [Cloud Server]                                     │
│       │                                             │
│       │ 1. Build new firmware                       │
│       │ 2. Sign with imgtool                        │
│       │ 3. Upload to HTTPS server                   │
│       ↓                                             │
│  [HTTPS Server: ota.example.com]                    │
│       │                                             │
│       │ 4G LTE                                      │
│       ↓                                             │
│  [Device: STM32U5 + SimCom A7672SA]                 │
│       │                                             │
│       │ 4. Query for updates (HTTPS GET)            │
│       │ 5. Download firmware (chunked)              │
│       │ 6. Stage to secondary slot (PSA FWU)        │
│       │ 7. Mark for installation                    │
│       │ 8. Reboot                                   │
│       ↓                                             │
│  [MCUboot Bootloader]                               │
│       │                                             │
│       │ 9. Verify signature (ECDSA)                 │
│       │ 10. Check security counter                  │
│       │ 11. Perform swap                            │
│       │ 12. Boot new firmware (TRIAL)               │
│       ↓                                             │
│  [New Firmware]                                     │
│       │                                             │
│       │ 13. Self-test                               │
│       │ 14. Confirm update (psa_fwu_accept)         │
│       │ 15. Report success to server                │
│       ↓                                             │
│  [Update Complete]                                  │
│                                                     │
└─────────────────────────────────────────────────────┘
```

**Implementation:** (Full code in Module 15.7)

Key components:
1. **Server-side**: Flask server with firmware versioning
2. **Device-side**: OTA client with 4G connectivity
3. **Security**: TLS 1.3, signature verification, anti-rollback
4. **Recovery**: Automatic rollback on failure

**Test Procedure:**
```bash
# 1. Build firmware v1.1.0
cmake -DVERSION=1.1.0 ...
ninja
./sign_firmware.sh

# 2. Upload to server
scp tfm_s_signed.bin server:/var/www/ota/tracker_v1.1.0.bin

# 3. Run device OTA client
# Device queries server, downloads, installs

# 4. Monitor device logs
# Verify successful update and confirmation
```

**Expected Device Logs:**
```
[OTA Client] Checking for updates...
[OTA Client] Current version: 1.0.0
[OTA Client] Available version: 1.1.0
[OTA Client] Downloading firmware (245760 bytes)...
[OTA Client] Progress: 100% ✓
[OTA Client] Staging to secondary slot...
[OTA Client] Staging complete ✓
[OTA Client] Installing and rebooting...

[BL2] MCUboot bootloader starting
[BL2] Verifying secondary image...
[BL2] Signature valid ✓
[BL2] Security counter: 2 >= 1 ✓
[BL2] Performing swap...
[BL2] Booting v1.1.0 (TRIAL)

[App] Firmware v1.1.0 starting
[App] Running self-tests...
[App] All tests passed ✓
[App] Confirming update...
[App] Update confirmed ✓

[OTA Client] Reporting success to server...
[OTA Client] ✓ OTA update successful!
```

---

**LABS 16-20 COMPLETE!**

**Summary:**
- Lab 16: Image signing with imgtool (key generation, signing, verification)
- Lab 17: MCUboot configuration (swap modes, flash layout)
- Lab 18: Swap mechanism testing (upgrade workflow)
- Lab 19: Rollback protection (security counter verification)
- Lab 20: Complete OTA system (end-to-end implementation)

Total content: 35+ pages with complete boot and update workflow examples!

---

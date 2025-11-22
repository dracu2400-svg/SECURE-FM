# Lab 06: Firmware Update with MCUboot - Secure OTA Updates

**Objective:** Learn how to implement secure Over-The-Air (OTA) firmware updates using MCUboot bootloader with TF-M.

**Duration:** 120 minutes

**Hardware:** NUCLEO-U545RE-Q with TF-M + MCUboot

---

## Overview

MCUboot is a secure bootloader for 32-bit microcontrollers. It provides:
- **Secure Boot:** Verify firmware signature before execution
- **Rollback Protection:** Prevent downgrade attacks
- **A/B Slot Updates:** Swap between two firmware images
- **Fault Recovery:** Automatically revert to previous version if new firmware fails

Combined with TF-M, MCUboot enables production-grade secure firmware updates.

---

## Learning Objectives

By the end of this lab, you will be able to:

- ✅ Understand MCUboot architecture (slots, swap, rollback)
- ✅ Sign firmware images with cryptographic keys
- ✅ Perform secure firmware updates
- ✅ Implement version checking and rollback protection
- ✅ Test fault recovery (revert to previous firmware)
- ✅ See immediate visual feedback with NUCLEO LEDs

---

## Architecture

### Memory Layout

```
┌─────────────────────────────────────────────────────────┐
│ Flash Memory (STM32U545)                                │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  0x08000000  ┌────────────────────────────────┐        │
│              │                                 │        │
│              │  MCUboot Bootloader (64 KB)     │        │
│              │  - Signature verification        │        │
│              │  - Slot management              │        │
│              │  - Swap algorithm               │        │
│  0x08010000  ├────────────────────────────────┤        │
│              │                                 │        │
│              │  Slot 0: PRIMARY (512 KB)       │        │
│              │  - Active firmware image         │        │
│              │  - Boots from here              │        │
│              │  - Image header + signature     │        │
│  0x08090000  ├────────────────────────────────┤        │
│              │                                 │        │
│              │  Slot 1: SECONDARY (512 KB)     │        │
│              │  - Staging area for updates     │        │
│              │  - Downloaded firmware          │        │
│              │  - Swapped with Slot 0          │        │
│  0x08110000  ├────────────────────────────────┤        │
│              │                                 │        │
│              │  Scratch Area (64 KB)           │        │
│              │  - Temporary storage for swap   │        │
│              │                                 │        │
│  0x08120000  └────────────────────────────────┘        │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### Boot Flow

```
Power On / Reset
      │
      ↓
┌──────────────────────┐
│ MCUboot Bootloader   │
└──────────────────────┘
      │
      ↓
1. Read Slot 0 Image Header
      │
      ↓
2. Verify Signature (RSA-2048 or ECDSA P-256)
      │
      ├─ Valid ──→ Boot Image
      │
      └─ Invalid ─→ Check Slot 1
            │
            ├─ Valid ──→ Swap & Boot
            │
            └─ Invalid ─→ HALT (No valid firmware)
```

---

## Part 1: Understanding MCUboot (20 minutes)

### 1.1 Image Header Structure

Every firmware image has a header:

```c
struct image_header {
    uint32_t magic;        /* 0x96f3b83d (IMAGE_MAGIC) */
    uint32_t load_addr;    /* Address to load image */
    uint16_t hdr_size;     /* Header size */
    uint16_t protect_tlv_size;  /* Protected TLV area size */
    uint32_t img_size;     /* Image size (excluding header) */
    uint32_t flags;        /* Image flags */
    struct image_version ver;  /* Version number */
    uint32_t _pad1;
};

struct image_version {
    uint8_t major;
    uint8_t minor;
    uint16_t revision;
    uint32_t build_num;
};
```

### 1.2 Image Trailer (TLV)

After the image, there's a TLV (Type-Length-Value) area:

```
┌──────────────────────────────────────┐
│ Image Data                            │
├──────────────────────────────────────┤
│ TLV Area:                             │
│   - SHA256 Hash                       │
│   - RSA-2048/ECDSA Signature          │
│   - Dependency (minimum version)      │
│   - Encryption key (if encrypted)     │
└──────────────────────────────────────┘
```

---

## Part 2: Sign Firmware Image (25 minutes)

### 2.1 Generate Signing Keys

```bash
cd lab_06

# Generate RSA-2048 key pair
imgtool keygen -k keys/root-rsa-2048.pem -t rsa-2048

# Generate ECDSA P-256 key pair (smaller, faster)
imgtool keygen -k keys/root-ec-p256.pem -t ecdsa-p256

echo "✓ Keys generated"
```

### 2.2 Sign Firmware (Version 1.0.0)

Create `firmware_v1/main.c`:

```c
#include <stdio.h>
#include "board_leds.h"

#define FIRMWARE_VERSION "1.0.0"

int main(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║          FIRMWARE VERSION %s                        ║\n", FIRMWARE_VERSION);
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("\n");

    while (1) {
        /* Version 1.0.0: Slow green blink */
        LED_Green_On();
        HAL_Delay(1000);
        LED_Green_Off();
        HAL_Delay(1000);
    }
}
```

Build and sign:

```bash
# Build firmware
cd firmware_v1
make clean && make

# Sign image with MCUboot
imgtool sign \
    --key ../keys/root-ec-p256.pem \
    --header-size 0x400 \
    --align 8 \
    --version 1.0.0 \
    --slot-size 0x80000 \
    --pad-header \
    _build/firmware.bin \
    _build/firmware_signed.bin

echo "✓ Firmware v1.0.0 signed"
```

**Output:**
```
Signing image with EC256 key
Header size: 1024 bytes
Image size: 45678 bytes
TLV size: 512 bytes
Total size: 47214 bytes
✓ Firmware v1.0.0 signed
```

---

## Part 3: Flash and Boot Firmware (20 minutes)

### 3.1 Flash Signed Firmware to Slot 0

```bash
# Flash MCUboot bootloader
st-flash write mcuboot.bin 0x08000000

# Flash signed firmware v1.0.0 to Slot 0 (PRIMARY)
st-flash write firmware_v1/_build/firmware_signed.bin 0x08010000

echo "✓ Firmware v1.0.0 flashed to Slot 0"
```

### 3.2 Observe Boot

Connect serial console:

```bash
minicom -D /dev/ttyACM0 -b 115200
```

**Expected Boot Log:**
```
[MCUboot] Starting MCUboot Bootloader
[MCUboot] Version: 1.9.0
[MCUboot] Checking Slot 0 (PRIMARY)...
[MCUboot]   Image magic: 0x96f3b83d (valid)
[MCUboot]   Image version: 1.0.0+0
[MCUboot]   Image size: 45678 bytes
[MCUboot] Verifying signature (ECDSA P-256)...
[MCUboot]   ✓ Signature valid
[MCUboot] Booting Slot 0 firmware
[MCUboot] Starting application at 0x08010400

╔══════════════════════════════════════════════════════════╗
║          FIRMWARE VERSION 1.0.0                          ║
╚══════════════════════════════════════════════════════════╝

[LED] LD1 (Green) blinks slowly (1s on, 1s off)
```

---

## Part 4: Perform Firmware Update (30 minutes)

### 4.1 Create Updated Firmware (Version 2.0.0)

Create `firmware_v2/main.c`:

```c
#include <stdio.h>
#include "board_leds.h"

#define FIRMWARE_VERSION "2.0.0"

int main(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║      🚀 FIRMWARE VERSION %s (UPDATED!)            ║\n", FIRMWARE_VERSION);
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("\n");

    while (1) {
        /* Version 2.0.0: Fast blue blink */
        LED_Blue_On();
        HAL_Delay(250);
        LED_Blue_Off();
        HAL_Delay(250);
    }
}
```

Build and sign:

```bash
cd firmware_v2
make clean && make

# Sign with HIGHER version number
imgtool sign \
    --key ../keys/root-ec-p256.pem \
    --header-size 0x400 \
    --align 8 \
    --version 2.0.0 \
    --slot-size 0x80000 \
    --pad-header \
    _build/firmware.bin \
    _build/firmware_signed.bin

echo "✓ Firmware v2.0.0 signed"
```

### 4.2 Download to Slot 1 (SECONDARY)

Simulate OTA download by flashing to Slot 1:

```bash
# Flash signed firmware v2.0.0 to Slot 1 (SECONDARY)
st-flash write firmware_v2/_build/firmware_signed.bin 0x08090000

echo "✓ Firmware v2.0.0 downloaded to Slot 1"
```

### 4.3 Mark Image for Test

Mark the new image to be tested on next boot:

```c
/* In firmware v1.0.0, add OTA update trigger */
#include "bootutil/bootutil.h"

void perform_ota_update(void)
{
    printf("\n[OTA] New firmware detected in Slot 1\n");
    printf("[OTA] Marking for test on next boot...\n");

    /* Mark Slot 1 image for testing */
    int rc = boot_set_pending(1);  /* 1 = confirm on next boot */

    if (rc == 0) {
        printf("[OTA] ✓ Update scheduled\n");
        printf("[OTA] Rebooting in 3 seconds...\n");
        HAL_Delay(3000);

        /* Trigger reset */
        NVIC_SystemReset();
    } else {
        printf("[OTA] ✗ Failed to schedule update\n");
    }
}
```

### 4.4 Observe Swap and Boot

After reset, MCUboot swaps images:

**Boot Log:**
```
[MCUboot] Starting MCUboot Bootloader
[MCUboot] Version: 1.9.0
[MCUboot] Checking Slot 0 (PRIMARY)...
[MCUboot]   Image version: 1.0.0+0
[MCUboot] Checking Slot 1 (SECONDARY)...
[MCUboot]   Image version: 2.0.0+0
[MCUboot]   Status: PENDING TEST
[MCUboot] ⚠️  Image marked for test - swapping slots
[MCUboot] Performing image swap...
[MCUboot]   Copying Slot 1 → Scratch
[MCUboot]   Copying Slot 0 → Slot 1
[MCUboot]   Copying Scratch → Slot 0
[MCUboot]   ✓ Swap complete
[MCUboot] Booting Slot 0 firmware (was Slot 1)

╔══════════════════════════════════════════════════════════╗
║      🚀 FIRMWARE VERSION 2.0.0 (UPDATED!)                ║
╚══════════════════════════════════════════════════════════╝

[LED] LD2 (Blue) blinks fast (250ms on, 250ms off)
```

---

## Part 5: Confirm or Revert (15 minutes)

### 5.1 Confirm Update (Success)

If new firmware works correctly:

```c
/* In firmware v2.0.0 */
#include "bootutil/bootutil.h"

void confirm_firmware(void)
{
    printf("\n[CONFIRM] Firmware v2.0.0 working correctly\n");
    printf("[CONFIRM] Confirming update...\n");

    /* Confirm this image as permanent */
    int rc = boot_set_confirmed();

    if (rc == 0) {
        printf("[CONFIRM] ✓ Update confirmed!\n");
        printf("[CONFIRM] This firmware will boot permanently\n");
    } else {
        printf("[CONFIRM] ✗ Failed to confirm\n");
    }
}
```

**Result:** v2.0.0 becomes permanent, v1.0.0 is discarded.

### 5.2 Revert Update (Failure)

If new firmware has bugs, DON'T confirm:

```c
/* Simulate failure in v2.0.0 */
void simulate_firmware_failure(void)
{
    printf("\n[ERROR] Critical failure detected!\n");
    printf("[ERROR] Firmware v2.0.0 is faulty\n");
    printf("[ERROR] NOT confirming update\n");
    printf("[ERROR] Rebooting will revert to v1.0.0...\n");

    HAL_Delay(3000);
    NVIC_SystemReset();
}
```

**Boot Log After Revert:**
```
[MCUboot] Starting MCUboot Bootloader
[MCUboot] Checking Slot 0 (PRIMARY)...
[MCUboot]   Image version: 2.0.0+0
[MCUboot]   Status: PENDING (not confirmed)
[MCUboot] ⚠️  Image not confirmed - reverting
[MCUboot] Performing rollback swap...
[MCUboot]   Swapping Slot 0 ↔ Slot 1
[MCUboot]   ✓ Rollback complete
[MCUboot] Booting Slot 0 firmware (restored v1.0.0)

╔══════════════════════════════════════════════════════════╗
║          FIRMWARE VERSION 1.0.0                          ║
╚══════════════════════════════════════════════════════════╝

[LED] LD1 (Green) blinks slowly (1s on, 1s off)
```

---

## Part 6: Version Anti-Rollback (10 minutes)

### 6.1 Enable Security Counter

Prevent downgrade to old (vulnerable) firmware:

```c
/* In MCUboot configuration */
#define MCUBOOT_HW_ROLLBACK_PROT

/* Define minimum allowed version */
#define MCUBOOT_MIN_VER_MAJOR  2
#define MCUBOOT_MIN_VER_MINOR  0
```

### 6.2 Test Downgrade Prevention

Try to install v1.0.0 after v2.0.0:

```bash
# Flash old firmware v1.0.0 to Slot 1
st-flash write firmware_v1/_build/firmware_signed.bin 0x08090000

# Mark for test
boot_set_pending(1);

# Reboot
```

**Boot Log:**
```
[MCUboot] Checking Slot 1 (SECONDARY)...
[MCUboot]   Image version: 1.0.0+0
[MCUboot] ✗ Version 1.0.0 < minimum 2.0.0
[MCUboot] ✗ Rollback protection: REJECTED
[MCUboot] Booting Slot 0 firmware (v2.0.0)
```

**LED:** Red LED blinks 5 times (downgrade rejected)

---

## Complete Demo Application

Create `lab_06/src/main.c`:

```c
#include <stdio.h>
#include <stdbool.h>
#include "board_leds.h"
#include "bootutil/bootutil.h"
#include "bootutil/image.h"

#define FIRMWARE_VERSION "2.0.0"

static void display_boot_info(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║        Firmware Update Demo - Version %s            ║\n", FIRMWARE_VERSION);
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("\n");

    /* Get boot status */
    struct boot_rsp rsp;
    int rc = boot_get_image_info(&rsp);

    if (rc == 0) {
        printf("Boot Information:\n");
        printf("  Slot: %d\n", rsp.br_image_slot);
        printf("  Version: %d.%d.%d+%lu\n",
               rsp.br_image_version.iv_major,
               rsp.br_image_version.iv_minor,
               rsp.br_image_version.iv_revision,
               rsp.br_image_version.iv_build_num);
        printf("  Confirmed: %s\n", boot_is_img_confirmed() ? "Yes" : "No (TESTING)");
    }

    printf("\n");
}

static void check_for_updates(void)
{
    /* Check if Slot 1 has a new image */
    printf("[UPDATE] Checking for new firmware...\n");

    /* Simulate checking Slot 1 image header */
    /* In real implementation, read flash at Slot 1 address */

    printf("[UPDATE] New firmware v2.1.0 available\n");
    printf("[UPDATE] Download and install? (Press USER button)\n");

    /* Wait for button press */
    while (!Button_IsPressed()) {
        HAL_Delay(100);
    }

    printf("[UPDATE] Installing update...\n");

    /* Mark Slot 1 for test */
    int rc = boot_set_pending(1);
    if (rc == 0) {
        printf("[UPDATE] ✓ Update scheduled\n");
        printf("[UPDATE] Rebooting...\n");
        HAL_Delay(2000);
        NVIC_SystemReset();
    }
}

static void test_firmware_health(void)
{
    printf("\n[HEALTH] Running firmware health check...\n");

    /* Simulate health check */
    bool all_tests_passed = true;

    printf("  Testing peripherals...\n");
    HAL_Delay(500);
    printf("  Testing connectivity...\n");
    HAL_Delay(500);
    printf("  Testing sensors...\n");
    HAL_Delay(500);

    if (all_tests_passed) {
        printf("[HEALTH] ✓ All tests passed\n");

        /* Confirm this firmware if not already confirmed */
        if (!boot_is_img_confirmed()) {
            printf("[CONFIRM] Confirming firmware update...\n");
            boot_set_confirmed();
            printf("[CONFIRM] ✓ Firmware confirmed as permanent\n");
            LED_Green_Blink(5);
        }
    } else {
        printf("[HEALTH] ✗ Tests failed - triggering rollback\n");
        printf("[ROLLBACK] Rebooting to previous firmware...\n");
        LED_Red_Blink(10);
        HAL_Delay(3000);
        NVIC_SystemReset();
    }
}

int main(void)
{
    /* Initialize hardware */
    Board_Init();

    /* Display boot information */
    display_boot_info();

    /* If firmware is not confirmed, run health check */
    if (!boot_is_img_confirmed()) {
        printf("\n⚠️  Firmware is in TEST mode\n");
        test_firmware_health();
    }

    /* Main loop: check for updates */
    printf("\n[MAIN] Entering main loop\n");
    printf("[MAIN] Press USER button to simulate OTA update\n\n");

    while (1) {
        /* Blink LED based on version */
        #if defined(VERSION_1_0_0)
            LED_Green_On();
            HAL_Delay(1000);
            LED_Green_Off();
            HAL_Delay(1000);
        #elif defined(VERSION_2_0_0)
            LED_Blue_On();
            HAL_Delay(250);
            LED_Blue_Off();
            HAL_Delay(250);
        #endif

        /* Check for button press (simulate OTA check) */
        if (Button_IsPressed()) {
            check_for_updates();
        }
    }
}
```

---

## Building and Testing

### Build Script

Create `lab_06/build.sh`:

```bash
#!/bin/bash

echo "Building MCUboot Lab 06..."

# Build MCUboot bootloader
cd mcuboot/boot/mynewt
newt build stm32u5_boot
cp bin/targets/stm32u5_boot/app/boot/mynewt/mynewt.elf.bin ../../mcuboot.bin

# Build firmware v1.0.0
cd ../../..
cd firmware_v1
make clean
make
imgtool sign --key ../keys/root-ec-p256.pem --header-size 0x400 \
    --align 8 --version 1.0.0 --slot-size 0x80000 --pad-header \
    _build/firmware.bin _build/firmware_signed.bin

# Build firmware v2.0.0
cd ../firmware_v2
make clean
make
imgtool sign --key ../keys/root-ec-p256.pem --header-size 0x400 \
    --align 8 --version 2.0.0 --slot-size 0x80000 --pad-header \
    _build/firmware.bin _build/firmware_signed.bin

echo "✓ Build complete"
echo ""
echo "Flash commands:"
echo "  MCUboot:     st-flash write mcuboot.bin 0x08000000"
echo "  Firmware v1: st-flash write firmware_v1/_build/firmware_signed.bin 0x08010000"
echo "  Firmware v2: st-flash write firmware_v2/_build/firmware_signed.bin 0x08090000"
```

---

## Exercises

### Exercise 1: Encrypted Firmware
Modify the signing process to encrypt the firmware image with AES-128.

**Hint:** Use `imgtool sign --encrypt <key>`

### Exercise 2: Delta Updates
Implement delta (diff-based) updates to reduce download size.

**Hint:** Use `imgtool` with `--diff` option

### Exercise 3: Multi-Image Update
Update both secure (TF-M) and non-secure firmware simultaneously.

**Hint:** Use MCUboot multi-image support

---

## Security Best Practices

### ✅ DO

1. **Always verify signatures** before booting
2. **Use hardware-backed keys** for signing (HSM, secure element)
3. **Enable rollback protection** for production
4. **Encrypt sensitive firmware** (if IP protection needed)
5. **Test update process** thoroughly
6. **Monitor update success rate**

### ❌ DON'T

1. **Don't hardcode keys** in firmware
2. **Don't skip signature verification** (even in debug)
3. **Don't allow unsigned images** in Slot 1
4. **Don't use same key** for all devices (use per-device keys)
5. **Don't auto-confirm** without health check

---

## Troubleshooting

### Update Fails to Install

**Symptoms:** MCUboot doesn't swap images

**Causes:**
1. Image not marked as pending
2. Signature verification failed
3. Insufficient scratch area

**Solution:** Check MCUboot logs, verify signing key matches

### Infinite Reboot Loop

**Symptoms:** Device keeps rebooting

**Cause:** New firmware crashes before confirming

**Solution:** MCUboot will automatically rollback after 3 failed boots

### Slot 1 Empty After Update

**Symptoms:** Old firmware lost

**Cause:** This is normal - swap moves Slot 1 to Slot 0

**Solution:** Keep backup of previous firmware externally

---

## Summary

In this lab, you learned:

✅ **MCUboot Architecture**
- Slot-based firmware management
- Swap algorithm
- Rollback protection

✅ **Secure Updates**
- Image signing with RSA/ECDSA
- Signature verification
- Encrypted firmware (optional)

✅ **Update Process**
- Download to Slot 1
- Mark for test
- Automatic swap on boot
- Confirm or revert

✅ **Visual Feedback**
- Different LED patterns for different versions
- See updates in action on NUCLEO board

---

## Next Steps

- **Lab 07:** Secure Boot Measurements
- **Lab 08:** Advanced Protected Storage
- **Lab 09:** Runtime Integrity Monitoring

---

## References

- [MCUboot Documentation](https://docs.mcuboot.com/)
- [TF-M Firmware Update Service](https://tf-m-user-guide.trustedfirmware.org/integration_guide/services/tfm_fwu_service.html)
- [PSA Firmware Update API](https://arm-software.github.io/psa-api/fwu/)

---

**Lab 06 Complete!** ✅

You now understand how to implement production-grade secure firmware updates using MCUboot - a critical capability for maintaining IoT device security throughout their lifecycle!

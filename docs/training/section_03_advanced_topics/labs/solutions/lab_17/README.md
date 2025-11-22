# Lab 17: Secure Firmware Update Integration

## Overview

Complete MCUboot integration with encrypted firmware images, secure flash operations, and production deployment workflow.

**Duration:** 120 minutes
**Difficulty:** Advanced
**Prerequisites:** Labs 02-16

---

## Learning Objectives

1. ✅ Integrate MCUboot with TF-M
2. ✅ Implement encrypted firmware updates
3. ✅ Configure secure flash operations
4. ✅ Test A/B slot swapping
5. ✅ Implement rollback protection
6. ✅ Deploy over-the-air updates

---

## Exercise 1: MCUboot + TF-M Integration

### Memory Layout

```
0x08000000: MCUboot (64 KB)
0x08010000: Slot 0 PRIMARY (TF-M Secure + Non-Secure) (512 KB)
0x08090000: Slot 1 SECONDARY (512 KB)
0x08110000: Scratch (64 KB)
```

### Build MCUboot with TF-M

```bash
cd mcuboot
cmake -S . -B build \
  -DMCUBOOT_TARGET=stm32u545 \
  -DMCUBOOT_ENCRYPT_AES256=ON \
  -DMCUBOOT_SECURITY_COUNTER=ON

cmake --build build
```

---

## Exercise 2: Encrypted Firmware Images

### Sign and Encrypt Image

```bash
# Generate encryption key (AES-256)
openssl rand -hex 32 > aes-key.txt

# Sign and encrypt firmware
imgtool sign \
  --key ec256-key.pem \
  --encrypt aes-key.txt \
  --align 8 \
  --version 1.2.0 \
  --header-size 0x400 \
  --pad-header \
  --slot-size 0x80000 \
  app.bin \
  app-signed-encrypted.bin
```

### Flash Encrypted Image

```c
int SecureFirmwareUpdate_Flash(const uint8_t *encrypted_image, size_t len)
{
    /* Unlock flash */
    HAL_FLASH_Unlock();

    /* Write to SECONDARY slot */
    uint32_t address = SECONDARY_SLOT_BASE;

    for (size_t i = 0; i < len; i += 8) {
        uint64_t data = *(uint64_t*)(encrypted_image + i);
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address + i, data);
    }

    HAL_FLASH_Lock();

    printf("✓ Encrypted firmware written to SECONDARY slot\n");
    return 0;
}
```

---

## Exercise 3: Secure OTA Update

### Download and Apply Update

```c
int SecureOTA_ApplyUpdate(const char *firmware_url)
{
    /* Download firmware via HTTPS */
    uint8_t *firmware_data;
    size_t firmware_len;

    int ret = HTTPS_Download(firmware_url, &firmware_data, &firmware_len);
    if (ret != 0) {
        return -1;
    }

    /* Verify signature */
    if (MCUboot_VerifySignature(firmware_data, firmware_len) != 0) {
        printf("❌ Signature verification failed\n");
        return -1;
    }

    /* Flash to SECONDARY slot */
    SecureFirmwareUpdate_Flash(firmware_data, firmware_len);

    /* Mark for swap */
    boot_set_pending(0);

    /* Reboot to apply update */
    HAL_NVIC_SystemReset();

    return 0;
}
```

---

## Key Takeaways

1. ✅ **MCUboot + TF-M** provides secure boot chain
2. ✅ **Encrypt firmware images** for confidentiality
3. ✅ **Verify signatures** before flashing
4. ✅ **A/B slot swapping** enables rollback
5. ✅ **Secure OTA updates** over HTTPS

---

**Lab 17 Complete! 🎉**

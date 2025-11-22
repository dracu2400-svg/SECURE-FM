# Lab 16: Secure DMA Operations

## Overview

This lab demonstrates secure DMA configuration using GTZC MPCBB to prevent unauthorized memory access and DMA-based attacks in TrustZone-M systems.

**Duration:** 90 minutes
**Difficulty:** Advanced
**Prerequisites:** Labs 02-15

---

## Learning Objectives

1. ✅ Understand STM32U5 DMA security architecture
2. ✅ Configure GTZC MPCBB for memory protection
3. ✅ Assign DMA channels to Secure/Non-Secure worlds
4. ✅ Prevent DMA-to-Secure-memory attacks
5. ✅ Implement secure DMA for cryptographic operations
6. ✅ Validate DMA security configuration

---

## Exercise 1: DMA Security Configuration

### Prevent Non-Secure DMA accessing Secure SRAM

```c
int SecureDMA_ConfigureProtection(void)
{
    /* Configure MPCBB1 to protect Secure SRAM */
    HAL_GTZC_MPCBB_ConfigMem(
        GTZC_MPCBB1,
        SRAM_BASE,
        0x00000000,  // Blocks 0-31: Secure (no DMA access)
        GTZC_MPCBB_BLOCK_SEC
    );

    /* Configure DMA1 as Non-Secure */
    HAL_GTZC_TZSC_ConfigPeriphAttributes(
        GTZC_PERIPH_DMA1,
        GTZC_TZSC_PERIPH_NSEC
    );

    printf("✓ Non-Secure DMA blocked from Secure SRAM\n");
    return 0;
}
```

### Test DMA Attack Prevention

```c
void test_dma_attack_prevention(void)
{
    __attribute__((section(".secure_sram"))) volatile uint32_t secure_key = 0xDEADBEEF;
    uint32_t stolen_data = 0;

    /* Attempt DMA transfer from Secure memory (should fail) */
    HAL_StatusTypeDef status = HAL_DMA_Start(
        &hdma1,
        (uint32_t)&secure_key,   // Source: Secure SRAM
        (uint32_t)&stolen_data,   // Destination: Non-Secure
        1
    );

    if (stolen_data != 0xDEADBEEF) {
        printf("✓ Attack BLOCKED: DMA cannot read Secure memory\n");
        LED_Green_Blink(3);
    } else {
        printf("❌ Attack SUCCESS: Security violation!\n");
        LED_Red_Blink(10);
    }
}
```

---

## Exercise 2: Secure Crypto DMA

### Use Secure DMA for AES Encryption

```c
int SecureDMA_AES_Encrypt(const uint8_t *plaintext,
                           uint8_t *ciphertext, size_t len)
{
    /* Configure DMA2 as Secure for crypto operations */
    HAL_GTZC_TZSC_ConfigPeriphAttributes(
        GTZC_PERIPH_DMA2,
        GTZC_TZSC_PERIPH_SEC | GTZC_TZSC_PERIPH_PRIV
    );

    /* Setup DMA channels for AES input/output */
    hdma_aes_in.Instance = DMA2_Channel1;
    hdma_aes_in.Init.Request = DMA_REQUEST_AES_IN;
    hdma_aes_in.Init.Direction = DMA_MEMORY_TO_PERIPH;
    HAL_DMA_Init(&hdma_aes_in);

    hdma_aes_out.Instance = DMA2_Channel2;
    hdma_aes_out.Init.Request = DMA_REQUEST_AES_OUT;
    hdma_aes_out.Init.Direction = DMA_PERIPH_TO_MEMORY;
    HAL_DMA_Init(&hdma_aes_out);

    /* Perform DMA-based AES encryption */
    HAL_CRYP_Encrypt_DMA(&hcryp, (uint32_t*)plaintext, len,
                          (uint32_t*)ciphertext);

    printf("✓ Secure DMA encryption complete\n");
    return 0;
}
```

---

## Key Takeaways

1. ✅ **MPCBB protects Secure SRAM** from Non-Secure DMA
2. ✅ **Configure DMA channels** with proper security attributes
3. ✅ **Use Secure DMA** for cryptographic operations
4. ✅ **Verify DMA cannot bypass** TrustZone protection
5. ✅ **Test attack scenarios** to validate configuration

---

**Lab 16 Complete! 🎉**

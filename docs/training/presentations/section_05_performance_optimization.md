---
marp: true
theme: default
paginate: true
backgroundColor: #ffffff
header: 'TF-M Training Package - Section 5'
footer: 'Performance & Optimization | © 2025'
---

<!-- _class: lead -->
<!-- _paginate: false -->

# Performance & Optimization

**Memory, Power & Speed**

![bg right:40%](https://via.placeholder.com/400x300/00AA00/ffffff?text=Performance)

*TF-M Training Package - Section 5*

---

## Section Overview

### Topics

1. **Memory Optimization**
2. **Power Management**
3. **Latency Reduction**
4. **Code Size Optimization**
5. **Benchmarking**

---

## Memory Layout Optimization

### STM32U545RE-Q (256 KB SRAM)

```
SRAM Usage:
├── Secure World (64 KB)
│   ├── TF-M SPM: 24 KB
│   ├── Crypto Partition: 16 KB
│   ├── Storage Partition: 12 KB
│   └── Custom Partitions: 12 KB
│
└── Non-Secure World (192 KB)
    ├── FreeRTOS Heap: 64 KB
    ├── Application Stack: 32 KB
    ├── Network Buffers: 48 KB
    └── Application Data: 48 KB
```

---

## Reducing TF-M Memory Footprint

```cmake
# tfm_config.cmake

# Disable unused partitions
set(TFM_PARTITION_INTERNAL_TRUSTED_STORAGE OFF)
set(TFM_PARTITION_PROTECTED_STORAGE OFF)
set(TFM_PARTITION_FIRMWARE_UPDATE OFF)

# Reduce crypto algorithms
set(TFM_CRYPTO_ENGINE "BUILTIN_KEYS_ONLY")

# Optimize partition sizes
set(TFM_PARTITION_CRYPTO_STACK_SIZE 0x1800)  # 6 KB
```

**Can save 20-40 KB SRAM**

---

## Stack Size Tuning

```c
/* Measure actual stack usage */
void check_stack_usage(void)
{
    extern uint32_t __StackLimit;
    extern uint32_t __StackTop;

    uint32_t *stack = &__StackLimit;
    uint32_t used = 0;

    /* Count non-0xAA bytes (stack canary pattern) */
    while (*stack != 0xAAAAAAAA) {
        used += 4;
        stack++;
    }

    printf("Stack used: %lu / %lu bytes\n", used,
           (uint32_t)(&__StackTop - &__StackLimit));
}
```

**Reduce stack size to minimum safe value**

---

## Heap Optimization

```c
/* FreeRTOS heap_4.c */
#define configTOTAL_HEAP_SIZE  (64 * 1024)  /* Adjust based on needs */

/* Monitor heap usage */
void print_heap_stats(void)
{
    size_t free = xPortGetFreeHeapSize();
    size_t min_free = xPortGetMinimumEverFreeHeapSize();

    printf("Heap free: %zu bytes\n", free);
    printf("Min free: %zu bytes (high water mark)\n", min_free);

    if (min_free < 4096) {
        printf("⚠️  Heap almost full!\n");
    }
}
```

---

## Power Modes (STM32U5)

| Mode | CPU | Peripherals | SRAM | Current | Wake-up |
|------|-----|-------------|------|---------|---------|
| **Run** | Active | Active | Retained | 19 mA | N/A |
| **Sleep** | Stopped | Active | Retained | 4.5 mA | Instant |
| **Stop 0** | Off | Stopped | Retained | 120 µA | Fast |
| **Stop 2** | Off | Off | Retained | 16 µA | Medium |
| **Standby** | Off | Off | Lost | 170 nA | Slow |

---

## Sleep Mode

```c
/* Simple sleep (WFI) */
void enter_sleep(void)
{
    /* Configure wake-up source (e.g., RTC alarm) */
    HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 10, RTC_WAKEUPCLOCK_CK_SPRE_16BITS);

    /* Enter sleep */
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

    /* Wakes up here */
    printf("Woke up from sleep\n");
}
```

**Current drops from 19 mA → 4.5 mA**

---

## Stop Mode

```c
void enter_stop_mode(void)
{
    /* Disable SysTick to prevent wake-up */
    HAL_SuspendTick();

    /* Configure wake-up (e.g., button press) */
    HAL_GPIO_EXTI_IRQHandler(BUTTON_PIN);

    /* Enter Stop 2 mode */
    HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);

    /* Resume SysTick after wake-up */
    HAL_ResumeTick();

    /* Reconfigure clocks (LSI running only) */
    SystemClock_Config();

    printf("Woke from Stop mode\n");
}
```

**Current: 16 µA (ultra-low power)**

---

## Secure Sleep

### Saving Secure Context

```c
void secure_sleep_prepare(void)
{
    /* 1. Complete all crypto operations */
    psa_crypto_operation_abort(&op);

    /* 2. Close all IPC connections */
    psa_close(handle);

    /* 3. Disable secure interrupts */
    NVIC_DisableIRQ(SECURE_TIMER_IRQn);

    /* 4. Save secure context */
    save_secure_context();

    /* 5. Allow NS to enter sleep */
    permit_ns_sleep();
}
```

---

## Dynamic Voltage Scaling (DVS)

```c
/* STM32U5 supports voltage scaling */
void set_performance_mode(perf_mode_t mode)
{
    switch (mode) {
        case PERF_HIGH:
            /* 160 MHz, VOS1, 19 mA */
            HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
            SystemClock_Config_160MHz();
            break;

        case PERF_LOW:
            /* 24 MHz, VOS2, 2.5 mA */
            HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2);
            SystemClock_Config_24MHz();
            break;
    }
}
```

**Lower frequency = Lower power**

---

## Crypto Hardware Acceleration

### AES Hardware Accelerator

```c
/* Software AES: ~500 cycles/block */
/* Hardware AES: ~50 cycles/block (10x faster!) */

/* Enable AES hardware */
__HAL_RCC_AES_CLK_ENABLE();

/* PSA Crypto automatically uses hardware if available */
psa_aead_encrypt(key_id, PSA_ALG_GCM, ...);
/* ↑ Uses SAES peripheral on STM32U5 */
```

**Throughput:** ~50 MB/s (hardware) vs ~5 MB/s (software)

---

## DMA for Large Transfers

```c
/* DMA offloads CPU for data transfers */
void crypto_encrypt_large_dma(uint8_t *data, size_t len)
{
    /* Configure DMA */
    HAL_DMA_Start(&hdma_aes_in, (uint32_t)data,
                  (uint32_t)&SAES->DINR, len);

    /* Configure SAES */
    HAL_CRYP_Encrypt_DMA(&hcryp, data, len, ciphertext, len);

    /* CPU free to do other work */
    while (HAL_CRYP_GetState(&hcryp) != HAL_CRYP_STATE_READY) {
        /* Do other tasks */
        process_network_packets();
    }
}
```

---

## IPC Latency Reduction

### Minimize Context Switches

```c
/* ❌ SLOW: Multiple IPC calls */
psa_hash_setup(&op, PSA_ALG_SHA_256);
psa_hash_update(&op, data1, len1);
psa_hash_update(&op, data2, len2);
psa_hash_update(&op, data3, len3);
psa_hash_finish(&op, hash, 32, &hash_len);
/* 5 context switches! */

/* ✅ FAST: Single IPC call */
psa_hash_compute(PSA_ALG_SHA_256, all_data, total_len,
                 hash, 32, &hash_len);
/* 1 context switch */
```

---

## Code Size Reduction

### Compiler Optimization Flags

```cmake
# CMakeLists.txt

# -Os: Optimize for size
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")

# Link-Time Optimization (LTO)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")

# Remove unused sections
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

# Result: 20-30% code size reduction
```

---

## Disable Unused Features

```c
/* mbedTLS config for minimal size */
#define MBEDTLS_AES_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_ECDSA_C

/* Disable unnecessary features */
#undef MBEDTLS_RSA_C  /* Not needed */
#undef MBEDTLS_MD5_C  /* Insecure */
#undef MBEDTLS_DES_C  /* Obsolete */
#undef MBEDTLS_SSL_PROTO_DTLS  /* Not using DTLS */

/* Result: 50+ KB flash savings */
```

---

## Benchmarking PSA Crypto

```c
void benchmark_crypto(void)
{
    uint32_t start, end;
    uint8_t data[1024];

    /* AES-256-GCM encryption */
    start = HAL_GetTick();
    for (int i = 0; i < 1000; i++) {
        psa_aead_encrypt(key_id, PSA_ALG_GCM, nonce, 12,
                         NULL, 0, data, 1024, out, 1040, NULL);
    }
    end = HAL_GetTick();

    printf("AES-GCM: %lu KB/s\n",
           (1024 * 1000) / (end - start));

    /* ECDSA P-256 signing */
    start = HAL_GetTick();
    for (int i = 0; i < 100; i++) {
        psa_sign_hash(key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256),
                      hash, 32, sig, 64, NULL);
    }
    end = HAL_GetTick();

    printf("ECDSA sign: %lu ops/sec\n",
           (100 * 1000) / (end - start));
}
```

---

## Performance Metrics (STM32U5 @ 160 MHz)

| Operation | Software | Hardware | Speedup |
|-----------|----------|----------|---------|
| **AES-128 encrypt** | 5 MB/s | 50 MB/s | 10x |
| **SHA-256 hash** | 8 MB/s | 60 MB/s | 7.5x |
| **ECDSA P-256 sign** | 40 ops/s | 400 ops/s | 10x |
| **ECDSA verify** | 20 ops/s | 200 ops/s | 10x |

**Always use hardware acceleration when available**

---

## Memory Profiling

```c
/* Track memory allocations */
void *secure_malloc(size_t size)
{
    void *ptr = malloc(size);

    if (ptr) {
        total_allocated += size;
        alloc_count++;

        if (total_allocated > peak_allocated) {
            peak_allocated = total_allocated;
        }
    }

    return ptr;
}

void print_memory_stats(void)
{
    printf("Total allocated: %lu bytes\n", total_allocated);
    printf("Peak allocated: %lu bytes\n", peak_allocated);
    printf("Allocation count: %lu\n", alloc_count);
}
```

---

## Flash Wear Leveling

```c
/* Distribute writes across flash sectors */
typedef struct {
    uint32_t write_count;
    uint32_t last_used;
} SectorInfo_t;

SectorInfo_t sectors[FLASH_SECTOR_COUNT];

uint32_t select_wear_leveled_sector(void)
{
    uint32_t min_writes = UINT32_MAX;
    uint32_t selected = 0;

    for (uint32_t i = 0; i < FLASH_SECTOR_COUNT; i++) {
        if (sectors[i].write_count < min_writes) {
            min_writes = sectors[i].write_count;
            selected = i;
        }
    }

    sectors[selected].write_count++;
    return selected;
}
```

---

## Interrupt Latency

```c
/* Measure interrupt response time */
void EXTI0_IRQHandler(void)
{
    uint32_t entry_time = DWT->CYCCNT;  /* Cycle counter */

    /* Handle interrupt */
    process_interrupt();

    uint32_t exit_time = DWT->CYCCNT;
    uint32_t cycles = exit_time - entry_time;

    printf("IRQ latency: %lu cycles (%.1f µs)\n",
           cycles, cycles / 160.0f);  /* 160 MHz */

    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}
```

**Target: <10 µs for critical interrupts**

---

## Cache Optimization (I-Cache)

```c
/* Enable instruction cache */
void enable_icache(void)
{
    /* STM32U5 has 8 KB I-Cache */
    __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();

    printf("I-Cache enabled\n");
}

/* Result: ~30% performance improvement for code in flash */
```

**Keep critical code in SRAM for zero-latency access**

---

## Optimizing PSA Storage

```c
/* Batch writes instead of individual */
typedef struct {
    uint32_t sensor_readings[100];
    uint32_t count;
} SensorBatch_t;

SensorBatch_t batch;

void add_reading(uint32_t value)
{
    batch.sensor_readings[batch.count++] = value;

    /* Write only when batch full */
    if (batch.count >= 100) {
        psa_ps_set(UID_SENSOR_DATA, sizeof(batch), &batch, 0);
        batch.count = 0;
    }
}
```

**Reduces flash writes by 100x**

---

## Section Summary

### Optimization Techniques

✓ Minimize memory footprint (stack, heap, code)
✓ Use hardware crypto acceleration
✓ Implement power management (Sleep, Stop, Standby)
✓ Reduce IPC overhead
✓ Profile and measure performance
✓ Optimize flash and SRAM usage

**Balance security, performance, and power consumption**

---

<!-- _class: lead -->
<!-- _paginate: false -->

# Questions?

**Next:** Section 6 - Security & Attack Mitigations

---

**End of Section 5**
*Total Slides: 40*
*Estimated Duration: 1.5 hours*

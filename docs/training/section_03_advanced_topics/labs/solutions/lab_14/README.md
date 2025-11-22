# Lab 14: Power Management and Secure Sleep Modes

## Overview

This lab demonstrates how to implement secure low-power modes in TrustZone-M systems, manage power states across Secure/Non-Secure worlds, and protect sensitive data during sleep. You'll learn power optimization techniques while maintaining security guarantees.

**Duration:** 90-120 minutes
**Difficulty:** Advanced
**Prerequisites:** Labs 02-13

---

## Learning Objectives

1. ✅ Understand STM32U5 low-power modes (Sleep, Stop, Standby)
2. ✅ Configure secure sleep with TrustZone protection
3. ✅ Implement wake-up authentication
4. ✅ Protect secure memory during low-power states
5. ✅ Measure power consumption in different modes
6. ✅ Optimize security services for low power
7. ✅ Handle tamper events during sleep

---

## STM32U5 Low-Power Modes

### Power Mode Comparison

| Mode | CPU | Peripherals | SRAM | Retention | Wake Time | Power (typ.) |
|------|-----|-------------|------|-----------|-----------|--------------|
| **Run** | Active | Active | Retained | N/A | N/A | 19 mA @ 160MHz |
| **Sleep** | Stopped | Active | Retained | Full | Instant | 3.5 mA |
| **Stop 0** | Stopped | Stopped | Retained | Full | 6 µs | 15 µA |
| **Stop 1** | Stopped | Stopped | Retained | Full | 7 µs | 9 µA |
| **Stop 2** | Stopped | Stopped | Partial | SRAM2 only | 8 µs | 2.8 µA |
| **Stop 3** | Stopped | Stopped | Minimal | Backup regs | 10 µs | 840 nA |
| **Standby** | Off | Off | Lost | Backup regs | 30 µs | 170 nA |

### Secure Sleep Architecture

```
┌────────────────────────────────────────────────┐
│       Secure Low-Power Management               │
├────────────────────────────────────────────────┤
│                                                 │
│  Non-Secure World         Secure World (TF-M)  │
│  ┌──────────────┐                              │
│  │ Application  │                              │
│  │ __WFI()      │──────────┐                   │
│  └──────────────┘          │                   │
│                            ↓                    │
│         ┌────────────────────────────┐         │
│         │   SPM Sleep Coordinator    │         │
│         │  - Validate sleep request  │         │
│         │  - Check Secure tasks      │         │
│         │  - Save Secure context     │         │
│         │  - Configure wake sources  │         │
│         └────────────────────────────┘         │
│                     ↓                           │
│         ┌────────────────────────────┐         │
│         │   Secure Power Controller  │         │
│         │  - SRAM retention config   │         │
│         │  - Secure backup regs      │         │
│         │  - Tamper monitoring       │         │
│         └────────────────────────────┘         │
│                     ↓                           │
│         ┌────────────────────────────┐         │
│         │   Hardware (PWR module)    │         │
│         │  - Enter low-power mode    │         │
│         └────────────────────────────┘         │
│                                                 │
│         [WAKE EVENT] ← EXTI, RTC, Tamper       │
│                     ↓                           │
│         ┌────────────────────────────┐         │
│         │   Secure Wake Handler      │         │
│         │  - Authenticate wake       │         │
│         │  - Restore Secure context  │         │
│         │  - Check integrity         │         │
│         └────────────────────────────┘         │
│                                                 │
└────────────────────────────────────────────────┘
```

---

## Exercise 1: Basic Secure Sleep Implementation

### Objective
Implement secure Sleep mode with proper context saving and restoration.

### Implementation

**File: `secure_power.h`**
```c
#ifndef SECURE_POWER_H
#define SECURE_POWER_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32u5xx.h"

/* Power Modes */
typedef enum {
    POWER_MODE_RUN,
    POWER_MODE_SLEEP,
    POWER_MODE_STOP0,
    POWER_MODE_STOP1,
    POWER_MODE_STOP2,
    POWER_MODE_STOP3,
    POWER_MODE_STANDBY
} PowerMode_t;

/* Wake Sources */
typedef enum {
    WAKE_SOURCE_BUTTON = (1 << 0),
    WAKE_SOURCE_RTC = (1 << 1),
    WAKE_SOURCE_UART = (1 << 2),
    WAKE_SOURCE_TAMPER = (1 << 3)
} WakeSource_t;

/* Secure Context (saved during sleep) */
typedef struct {
    uint32_t stack_pointer;
    uint32_t control_reg;
    uint32_t psp;
    uint32_t msp;
    uint8_t crypto_keys[32];  // Sensitive data to protect
    uint32_t checksum;         // Integrity check
} SecureSleepContext_t;

/**
 * @brief Initialize secure power management
 * @return 0 on success
 */
int SecurePower_Init(void);

/**
 * @brief Enter secure sleep mode
 * @param mode Power mode to enter
 * @param wake_sources Enabled wake sources (bitmask)
 * @return 0 on success
 */
int SecurePower_EnterSleep(PowerMode_t mode, uint32_t wake_sources);

/**
 * @brief Save Secure context before sleep
 * @param context Output context structure
 * @return 0 on success
 */
int SecurePower_SaveContext(SecureSleepContext_t *context);

/**
 * @brief Restore Secure context after wake
 * @param context Saved context structure
 * @return 0 on success, -1 if integrity check fails
 */
int SecurePower_RestoreContext(const SecureSleepContext_t *context);

/**
 * @brief Configure SRAM retention for low-power modes
 * @param retain_secure_sram true to retain Secure SRAM
 * @return 0 on success
 */
int SecurePower_ConfigureSRAMRetention(bool retain_secure_sram);

/**
 * @brief Measure current power consumption
 * @return Power consumption in µA
 */
uint32_t SecurePower_MeasureCurrent(void);

/**
 * @brief Get power mode name
 */
const char* SecurePower_GetModeName(PowerMode_t mode);

#endif /* SECURE_POWER_H */
```

**File: `secure_power.c`**
```c
#include "secure_power.h"
#include "psa/crypto.h"
#include <stdio.h>
#include <string.h>

/* Static context storage */
static SecureSleepContext_t saved_context;

/**
 * @brief Initialize secure power management
 */
int SecurePower_Init(void)
{
    printf("[Secure Power] Initializing power management...\n");

    /* Enable Power Controller clock */
    __HAL_RCC_PWR_CLK_ENABLE();

    /* Configure voltage regulator for low-power modes */
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2);

    /* Enable SRAM2 retention in Stop modes */
    HAL_PWREx_EnableSRAM2ContentRetention();

    /* Configure backup domain */
    HAL_PWR_EnableBkUpAccess();

    printf("[Secure Power] Power management initialized\n");
    return 0;
}

/**
 * @brief Save Secure context
 */
int SecurePower_SaveContext(SecureSleepContext_t *context)
{
    if (context == NULL) {
        return -1;
    }

    printf("[Secure Power] Saving Secure context...\n");

    /* Save processor state */
    context->control_reg = __get_CONTROL();
    context->psp = __get_PSP();
    context->msp = __get_MSP();

    /* Save sensitive cryptographic material (example) */
    /* In production, retrieve from PSA Crypto key storage */
    memset(context->crypto_keys, 0xAA, sizeof(context->crypto_keys));

    /* Compute integrity checksum */
    uint32_t sum = 0;
    uint32_t *data = (uint32_t*)context;
    for (size_t i = 0; i < (sizeof(SecureSleepContext_t) - 4) / 4; i++) {
        sum += data[i];
    }
    context->checksum = ~sum;  // One's complement checksum

    printf("[Secure Power] Context saved (checksum: 0x%08lX)\n",
           context->checksum);

    return 0;
}

/**
 * @brief Restore Secure context with integrity check
 */
int SecurePower_RestoreContext(const SecureSleepContext_t *context)
{
    if (context == NULL) {
        return -1;
    }

    printf("[Secure Power] Restoring Secure context...\n");

    /* Verify integrity checksum */
    uint32_t sum = 0;
    uint32_t *data = (uint32_t*)context;
    for (size_t i = 0; i < (sizeof(SecureSleepContext_t) - 4) / 4; i++) {
        sum += data[i];
    }
    sum += context->checksum;

    if (sum != 0xFFFFFFFF) {
        printf("[Secure Power] ERROR: Context integrity check FAILED!\n");
        printf("  Expected: 0xFFFFFFFF, Got: 0x%08lX\n", sum);
        /* Trigger tamper event */
        return -1;
    }

    printf("[Secure Power] Context integrity verified\n");

    /* Restore processor state */
    __set_CONTROL(context->control_reg);
    __set_PSP(context->psp);
    __set_MSP(context->msp);

    /* Restore cryptographic keys */
    /* In production, restore to PSA Crypto key storage */

    printf("[Secure Power] Context restored successfully\n");
    return 0;
}

/**
 * @brief Configure SRAM retention
 */
int SecurePower_ConfigureSRAMRetention(bool retain_secure_sram)
{
    printf("[Secure Power] Configuring SRAM retention...\n");

    if (retain_secure_sram) {
        /* Retain SRAM2 (Secure SRAM) in Stop modes */
        HAL_PWREx_EnableSRAM2ContentRetention();
        printf("  ✓ Secure SRAM (SRAM2) will be retained\n");
    } else {
        /* Power down SRAM2 for maximum power saving */
        HAL_PWREx_DisableSRAM2ContentRetention();
        printf("  ⚠️  Secure SRAM will be LOST in low-power mode\n");
    }

    return 0;
}

/**
 * @brief Enter secure sleep mode
 */
int SecurePower_EnterSleep(PowerMode_t mode, uint32_t wake_sources)
{
    printf("\n[Secure Power] Entering %s mode...\n",
           SecurePower_GetModeName(mode));

    /* Save Secure context */
    if (SecurePower_SaveContext(&saved_context) != 0) {
        printf("[Secure Power] ERROR: Failed to save context\n");
        return -1;
    }

    /* Configure wake sources */
    if (wake_sources & WAKE_SOURCE_BUTTON) {
        /* Enable EXTI for User Button (PC13) */
        HAL_NVIC_EnableIRQ(EXTI13_IRQn);
        printf("  ✓ Wake source: USER BUTTON enabled\n");
    }

    if (wake_sources & WAKE_SOURCE_RTC) {
        /* Enable RTC wake */
        HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
        printf("  ✓ Wake source: RTC ALARM enabled\n");
    }

    if (wake_sources & WAKE_SOURCE_TAMPER) {
        /* Enable tamper detection */
        HAL_NVIC_EnableIRQ(TAMP_IRQn);
        printf("  ✓ Wake source: TAMPER enabled\n");
    }

    /* Enter power mode */
    printf("[Secure Power] Entering low-power mode NOW...\n");
    printf("[LED] Blue LED off = sleeping\n");

    LED_Blue_Off();

    switch (mode) {
        case POWER_MODE_SLEEP:
            /* Sleep mode: CPU clock stopped, peripherals running */
            HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
            break;

        case POWER_MODE_STOP0:
            /* Stop 0: All clocks stopped, SRAM retained, fast wake */
            HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
            break;

        case POWER_MODE_STOP1:
            /* Stop 1: Lower power than Stop 0 */
            HAL_PWREx_EnterSTOP1Mode(PWR_STOPENTRY_WFI);
            break;

        case POWER_MODE_STOP2:
            /* Stop 2: SRAM2 retained only */
            HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
            break;

        case POWER_MODE_STANDBY:
            /* Standby: Lowest power, SRAM lost except backup registers */
            HAL_PWR_EnterSTANDBYMode();
            /* Never returns - device resets on wake */
            break;

        default:
            printf("[Secure Power] ERROR: Unknown power mode\n");
            return -1;
    }

    /* ═══ WAKE EVENT OCCURRED ═══ */

    LED_Blue_On();  // Device awake

    printf("\n[Secure Power] *** WAKE EVENT ***\n");

    /* Restore system clock after Stop mode */
    if (mode >= POWER_MODE_STOP0 && mode <= POWER_MODE_STOP3) {
        SystemClock_Config();
        printf("[Secure Power] System clock restored\n");
    }

    /* Restore Secure context */
    if (SecurePower_RestoreContext(&saved_context) != 0) {
        printf("[Secure Power] ERROR: Context restoration failed!\n");
        LED_Red_Blink(10);
        return -1;
    }

    printf("[Secure Power] Wake complete, context restored\n");
    return 0;
}

/**
 * @brief Measure power consumption (simulated)
 */
uint32_t SecurePower_MeasureCurrent(void)
{
    /* In production, use INA219 or similar current sensor */
    /* For STM32U5 NUCLEO, use X-NUCLEO-LPM01A power shield */

    /* Simulated values based on datasheet */
    uint32_t current_ua;

    if (__HAL_RCC_GET_SYSCLK_SOURCE() == RCC_SYSCLKSOURCE_STATUS_MSI) {
        current_ua = 3500;  // Sleep mode (~3.5 mA)
    } else {
        current_ua = 19000;  // Run mode @ 160 MHz (~19 mA)
    }

    return current_ua;
}

/**
 * @brief Get power mode name
 */
const char* SecurePower_GetModeName(PowerMode_t mode)
{
    switch (mode) {
        case POWER_MODE_RUN: return "RUN";
        case POWER_MODE_SLEEP: return "SLEEP";
        case POWER_MODE_STOP0: return "STOP 0";
        case POWER_MODE_STOP1: return "STOP 1";
        case POWER_MODE_STOP2: return "STOP 2";
        case POWER_MODE_STOP3: return "STOP 3";
        case POWER_MODE_STANDBY: return "STANDBY";
        default: return "UNKNOWN";
    }
}
```

### Testing Exercise 1

```c
void test_secure_sleep(void)
{
    printf("\n=== Exercise 1: Secure Sleep Mode ===\n");

    /* Initialize power management */
    SecurePower_Init();

    /* Measure run mode power */
    printf("\n[Test] Measuring RUN mode power...\n");
    uint32_t run_current = SecurePower_MeasureCurrent();
    printf("  Current: %lu µA (%lu mA)\n", run_current, run_current / 1000);
    LED_Green_On();
    HAL_Delay(2000);
    LED_Green_Off();

    /* Test Sleep mode */
    printf("\n[Test] Testing SLEEP mode...\n");
    printf("  Press USER BUTTON to wake...\n");

    SecurePower_EnterSleep(POWER_MODE_SLEEP, WAKE_SOURCE_BUTTON);

    printf("  ✓ Woke from SLEEP mode\n");
    LED_Green_Blink(2);

    /* Test Stop 0 mode */
    printf("\n[Test] Testing STOP 0 mode...\n");
    printf("  Device will sleep for 5 seconds (RTC wake)...\n");

    /* Configure RTC alarm for 5 seconds */
    RTC_AlarmTypeDef alarm = {0};
    alarm.Alarm = RTC_ALARM_A;
    alarm.AlarmTime.Seconds = 5;  // Wake in 5 seconds
    HAL_RTC_SetAlarm_IT(&hrtc, &alarm, RTC_FORMAT_BIN);

    SecurePower_EnterSleep(POWER_MODE_STOP0, WAKE_SOURCE_RTC);

    printf("  ✓ Woke from STOP 0 mode after 5 seconds\n");
    LED_Green_Blink(3);

    printf("\n✓ Exercise 1: COMPLETE\n");
}
```

**Expected Output:**
```
=== Exercise 1: Secure Sleep Mode ===

[Secure Power] Initializing power management...
[Secure Power] Power management initialized

[Test] Measuring RUN mode power...
  Current: 19000 µA (19 mA)

[Test] Testing SLEEP mode...
  Press USER BUTTON to wake...
[Secure Power] Entering SLEEP mode...
[Secure Power] Saving Secure context...
[Secure Power] Context saved (checksum: 0x12345678)
  ✓ Wake source: USER BUTTON enabled
[Secure Power] Entering low-power mode NOW...
[LED] Blue LED off = sleeping

[User presses button]

[Secure Power] *** WAKE EVENT ***
[Secure Power] Restoring Secure context...
[Secure Power] Context integrity verified
[Secure Power] Context restored successfully
[Secure Power] Wake complete, context restored
  ✓ Woke from SLEEP mode

[Test] Testing STOP 0 mode...
  Device will sleep for 5 seconds (RTC wake)...
[Secure Power] Entering STOP 0 mode...
[Secure Power] Saving Secure context...
[Secure Power] Context saved (checksum: 0xABCDEF01)
  ✓ Wake source: RTC ALARM enabled
[Secure Power] Entering low-power mode NOW...

[5 seconds pass]

[Secure Power] *** WAKE EVENT ***
[Secure Power] System clock restored
[Secure Power] Restoring Secure context...
[Secure Power] Context integrity verified
[Secure Power] Context restored successfully
  ✓ Woke from STOP 0 mode after 5 seconds

✓ Exercise 1: COMPLETE
```

**Visual Feedback:**
- 🟢 **Green LED on** during RUN mode power measurement
- 🔵 **Blue LED off** during sleep
- 🔵 **Blue LED on** after wake
- 🟢 **Green LED blinks** after successful wake

---

## Exercise 2: Tamper Detection During Sleep

### Objective
Configure tamper monitoring to detect physical attacks during low-power modes.

### Theory

**Tamper Events During Sleep:**
- Physical intrusion detection
- Voltage glitching
- Temperature anomalies
- External tamper pins (TAMP_IN1-5)
- Internal tamper (RTC, backup registers)

**Tamper Response:**
1. Immediate wake from low-power mode
2. Erase sensitive data in backup registers
3. Trigger security alert
4. Log tamper event

### Implementation

**File: `secure_tamper.h`**
```c
#ifndef SECURE_TAMPER_H
#define SECURE_TAMPER_H

#include <stdint.h>
#include <stdbool.h>

/* Tamper Sources */
typedef enum {
    TAMPER_SOURCE_PIN1 = (1 << 0),
    TAMPER_SOURCE_PIN2 = (1 << 1),
    TAMPER_SOURCE_VOLTAGE = (1 << 2),
    TAMPER_SOURCE_TEMPERATURE = (1 << 3),
    TAMPER_SOURCE_RTC = (1 << 4)
} TamperSource_t;

/* Tamper Event */
typedef struct {
    TamperSource_t source;
    uint32_t timestamp;
    bool during_sleep;
} TamperEvent_t;

/**
 * @brief Initialize tamper detection
 * @param sources Enabled tamper sources (bitmask)
 * @return 0 on success
 */
int SecureTamper_Init(uint32_t sources);

/**
 * @brief Configure active tamper (output signal on TAMP_OUT)
 * @return 0 on success
 */
int SecureTamper_ConfigureActiveTamper(void);

/**
 * @brief Get last tamper event
 * @param event Output event structure
 * @return 0 if event available, -1 if no event
 */
int SecureTamper_GetLastEvent(TamperEvent_t *event);

/**
 * @brief Clear tamper flags
 */
void SecureTamper_ClearFlags(void);

/**
 * @brief Erase sensitive data on tamper
 */
void SecureTamper_EraseSecretsOnDetection(void);

#endif /* SECURE_TAMPER_H */
```

**File: `secure_tamper.c`**
```c
#include "secure_tamper.h"
#include "psa/internal_trusted_storage.h"
#include <stdio.h>

static TamperEvent_t last_tamper_event;
static bool tamper_detected = false;

/**
 * @brief Initialize tamper detection
 */
int SecureTamper_Init(uint32_t sources)
{
    printf("[Secure Tamper] Initializing tamper detection...\n");

    /* Enable TAMP clock */
    __HAL_RCC_RTCAPB_CLK_ENABLE();

    /* Configure tamper sources */
    RTC_TamperTypeDef tamper_config = {0};

    if (sources & TAMPER_SOURCE_PIN1) {
        tamper_config.Tamper = RTC_TAMPER_1;
        tamper_config.Trigger = RTC_TAMPERTRIGGER_FALLINGEDGE;
        tamper_config.NoErase = RTC_TAMPER_ERASE_BACKUP_ENABLE;  // Erase on tamper
        tamper_config.MaskFlag = RTC_TAMPERMASK_FLAG_DISABLE;
        tamper_config.Filter = RTC_TAMPERFILTER_DISABLE;
        tamper_config.SamplingFrequency = RTC_TAMPERSAMPLINGFREQ_RTCCLK_DIV32768;
        tamper_config.PrechargeDuration = RTC_TAMPERPRECHARGEDURATION_1RTCCLK;
        tamper_config.TamperPullUp = RTC_TAMPER_PULLUP_ENABLE;
        tamper_config.TimeStampOnTamperDetection = RTC_TIMESTAMPONTAMPERDETECTION_ENABLE;

        HAL_RTCEx_SetTamper_IT(&hrtc, &tamper_config);
        printf("  ✓ TAMPER PIN 1 enabled\n");
    }

    /* Enable tamper interrupt */
    HAL_NVIC_SetPriority(TAMP_IRQn, 0, 0);  // Highest priority
    HAL_NVIC_EnableIRQ(TAMP_IRQn);

    printf("[Secure Tamper] Tamper detection initialized\n");
    return 0;
}

/**
 * @brief Configure active tamper
 *
 * Active tamper outputs a signal on TAMP_OUT that must be connected
 * to TAMP_IN via a secure PCB trace. If the trace is cut, tamper is triggered.
 */
int SecureTamper_ConfigureActiveTamper(void)
{
    printf("[Secure Tamper] Configuring active tamper...\n");

    RTC_ActiveTamperTypeDef active_tamper = {0};
    active_tamper.ActiveTamper = RTC_ATAMP_1;
    active_tamper.ActiveOutputChangePeriod = 4;  // Change every 4 cycles
    active_tamper.ActiveAsyncPrescaler = RTC_ATAMP_ASYNCPRES_RTCCLK_2;
    active_tamper.TimeStampOnTamperDetection = RTC_TIMESTAMPONTAMPERDETECTION_ENABLE;

    HAL_RTCEx_SetActiveTamper(&hrtc, &active_tamper);

    printf("[Secure Tamper] Active tamper enabled\n");
    printf("  ⚠️  Connect TAMP_OUT to TAMP_IN on PCB\n");
    return 0;
}

/**
 * @brief Tamper interrupt handler
 */
void TAMP_IRQHandler(void)
{
    HAL_RTCEx_TamperIRQHandler(&hrtc);
}

/**
 * @brief Tamper detection callback
 */
void HAL_RTCEx_Tamper1EventCallback(RTC_HandleTypeDef *hrtc)
{
    printf("\n");
    printf("═══════════════════════════════════════════\n");
    printf(" ⚠️  TAMPER EVENT DETECTED!\n");
    printf("═══════════════════════════════════════════\n");

    /* Log event */
    last_tamper_event.source = TAMPER_SOURCE_PIN1;
    last_tamper_event.timestamp = HAL_GetTick();
    last_tamper_event.during_sleep = (__get_PRIMASK() != 0);  // Check if in WFI
    tamper_detected = true;

    /* Erase sensitive data */
    SecureTamper_EraseSecretsOnDetection();

    /* Visual alert */
    LED_Red_On();

    printf("[Secure Tamper] Event logged at %lu ms\n",
           last_tamper_event.timestamp);
    printf("[Secure Tamper] Secrets erased\n");
}

/**
 * @brief Erase secrets on tamper detection
 */
void SecureTamper_EraseSecretsOnDetection(void)
{
    printf("[Secure Tamper] Erasing sensitive data...\n");

    /* Erase PSA ITS keys */
    psa_its_remove(0x1234);  // Example: encryption keys
    psa_its_remove(0x5678);  // Example: attestation keys

    /* Erase backup registers */
    HAL_PWR_EnableBkUpAccess();
    for (int i = 0; i < 32; i++) {
        HAL_RTCEx_BKUPWrite(&hrtc, i, 0x00000000);
    }
    HAL_PWR_DisableBkUpAccess();

    /* Zeroize cryptographic material in SRAM */
    /* (In production, zero all key buffers) */

    printf("[Secure Tamper] All sensitive data erased\n");
}

/**
 * @brief Get last tamper event
 */
int SecureTamper_GetLastEvent(TamperEvent_t *event)
{
    if (!tamper_detected || event == NULL) {
        return -1;
    }

    *event = last_tamper_event;
    return 0;
}

/**
 * @brief Clear tamper flags
 */
void SecureTamper_ClearFlags(void)
{
    __HAL_RTC_TAMPER_CLEAR_FLAG(&hrtc, RTC_FLAG_TAMP1F);
    tamper_detected = false;
    LED_Red_Off();
}
```

### Testing Exercise 2

```c
void test_tamper_during_sleep(void)
{
    printf("\n=== Exercise 2: Tamper Detection During Sleep ===\n");

    /* Initialize tamper detection */
    SecureTamper_Init(TAMPER_SOURCE_PIN1);

    /* Write sensitive data to backup register (for testing) */
    printf("\n[Test] Writing sensitive data to backup register...\n");
    HAL_PWR_EnableBkUpAccess();
    HAL_RTCEx_BKUPWrite(&hrtc, 0, 0xDEADBEEF);  // Secret value
    HAL_PWR_DisableBkUpAccess();
    printf("  Backup Register 0: 0xDEADBEEF\n");

    /* Enter sleep with tamper monitoring */
    printf("\n[Test] Entering STOP 1 mode with tamper monitoring...\n");
    printf("  ⚠️  Trigger TAMPER PIN 1 to simulate attack\n");
    printf("  (or press USER BUTTON to wake normally)\n");

    SecurePower_EnterSleep(POWER_MODE_STOP1,
                            WAKE_SOURCE_BUTTON | WAKE_SOURCE_TAMPER);

    /* Check if tamper occurred */
    TamperEvent_t event;
    if (SecureTamper_GetLastEvent(&event) == 0) {
        printf("\n🚨 TAMPER DETECTED DURING SLEEP!\n");
        printf("  Source: 0x%02X\n", event.source);
        printf("  Time: %lu ms\n", event.timestamp);
        printf("  During sleep: %s\n", event.during_sleep ? "YES" : "NO");

        /* Verify secrets were erased */
        uint32_t backup_value = HAL_RTCEx_BKUPRead(&hrtc, 0);
        if (backup_value == 0x00000000) {
            printf("  ✓ Backup register erased (was 0xDEADBEEF, now 0x%08lX)\n",
                   backup_value);
        } else {
            printf("  ❌ ERROR: Backup register not erased!\n");
        }

        LED_Red_Blink(5);
        SecureTamper_ClearFlags();
    } else {
        printf("\n✓ No tamper - woke normally\n");
        LED_Green_Blink(2);
    }

    printf("\n✓ Exercise 2: COMPLETE\n");
}
```

---

## Key Takeaways

1. ✅ **Always save/restore Secure context** during low-power transitions
2. ✅ **Verify context integrity** with checksums after wake
3. ✅ **Configure SRAM retention** based on security needs
4. ✅ **Enable tamper detection** during sleep for physical security
5. ✅ **Erase secrets immediately** on tamper events
6. ✅ **Balance power vs security** (Stop 2 vs Standby)
7. ✅ **Measure actual power** with hardware tools (X-NUCLEO-LPM01A)

---

## Power Optimization Tips

1. **Use Stop 1 for IoT devices** (9 µA with SRAM retention)
2. **Use Standby for ultra-low power** (170 nA, but SRAM lost)
3. **Retain only necessary SRAM** (SRAM2 for Secure data)
4. **Configure wake sources carefully** (minimize false wakes)
5. **Use RTC for periodic wake** (e.g., sensor sampling every 5 min)
6. **Disable unused peripherals** before sleep
7. **Lower clock speed** during active periods

---

## References

- [STM32U5 Power Management (AN5373)](https://www.st.com/resource/en/application_note/an5373-optimized-lowpower-modes-operation-of-stm32u5-series-microcontrollers-stmicroelectronics.pdf)
- [STM32U5 Tamper Detection (RM0456)](https://www.st.com/resource/en/reference_manual/rm0456-stm32u5-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)

---

**Lab 14 Complete! 🎉**

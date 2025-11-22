# Lab 15: Secure Timers and Watchdogs

## Overview

This lab demonstrates how to configure secure watchdogs and timers for system reliability and security. You'll learn to detect and recover from system faults, prevent denial-of-service attacks, and implement timeout-based security policies.

**Duration:** 90-120 minutes
**Difficulty:** Advanced
**Prerequisites:** Labs 02-14

---

## Learning Objectives

1. ✅ Configure STM32U5 watchdogs (IWDG, WWDG)
2. ✅ Implement secure watchdog with TrustZone
3. ✅ Create timeout-based security policies
4. ✅ Detect and handle system hangs
5. ✅ Prevent watchdog manipulation attacks
6. ✅ Implement secure timing for cryptographic operations
7. ✅ Handle watchdog in low-power modes

---

## STM32U5 Watchdog Architecture

### Watchdog Types

| Watchdog | Description | Use Case | Security |
|----------|-------------|----------|----------|
| **IWDG** | Independent Watchdog | System hang detection | Secure (LSI clock) |
| **WWDG** | Window Watchdog | Timing constraint enforcement | Configurable |
| **Secure IWDG** | IWDG with TrustZone | Critical secure tasks | Highest |

### Watchdog Security Model

```
┌────────────────────────────────────────────────┐
│         Secure Watchdog Architecture            │
├────────────────────────────────────────────────┤
│                                                 │
│  Secure World                Non-Secure World  │
│  ┌──────────────────┐       ┌──────────────┐  │
│  │ Secure IWDG      │       │ WWDG         │  │
│  │ - 12-bit down    │       │ - 7-bit down │  │
│  │ - LSI (32 kHz)   │       │ - PCLK1      │  │
│  │ - Refresh from S │       │ - Window mode│  │
│  │ - Reset on expire│       │ - Early warn │  │
│  └────────┬─────────┘       └──────┬───────┘  │
│           │                        │           │
│           └────────────┬───────────┘           │
│                        ↓                        │
│           ┌────────────────────────┐           │
│           │  Security Monitor       │           │
│           │  - Timeout detection    │           │
│           │  - Attack response      │           │
│           │  - Secret erasure       │           │
│           └────────────────────────┘           │
│                                                 │
└────────────────────────────────────────────────┘
```

---

## Exercise 1: Independent Watchdog (IWDG) Configuration

### Objective
Configure IWDG to detect system hangs and reset the device.

### Implementation

**File: `secure_watchdog.h`**
```c
#ifndef SECURE_WATCHDOG_H
#define SECURE_WATCHDOG_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32u5xx_hal.h"

/* Watchdog Configuration */
typedef struct {
    uint32_t timeout_ms;      // Timeout in milliseconds
    bool enable_early_warning; // Enable interrupt before reset
    bool secure_only;          // Only Secure world can refresh
} WatchdogConfig_t;

/* Watchdog Status */
typedef enum {
    WDG_STATUS_OK,
    WDG_STATUS_WARNING,
    WDG_STATUS_EXPIRED,
    WDG_STATUS_ATTACK
} WatchdogStatus_t;

/**
 * @brief Initialize secure IWDG
 * @param config Watchdog configuration
 * @return 0 on success
 */
int SecureWatchdog_Init(const WatchdogConfig_t *config);

/**
 * @brief Refresh watchdog (must be called periodically)
 * @return 0 on success
 */
int SecureWatchdog_Refresh(void);

/**
 * @brief Check if watchdog caused last reset
 * @return true if watchdog reset occurred
 */
bool SecureWatchdog_WasResetSource(void);

/**
 * @brief Get watchdog status
 * @return Current status
 */
WatchdogStatus_t SecureWatchdog_GetStatus(void);

/**
 * @brief Erase secrets on watchdog timeout (security response)
 */
void SecureWatchdog_OnTimeoutCallback(void);

#endif /* SECURE_WATCHDOG_H */
```

**File: `secure_watchdog.c`**
```c
#include "secure_watchdog.h"
#include "psa/internal_trusted_storage.h"
#include <stdio.h>

static IWDG_HandleTypeDef hiwdg;
static bool watchdog_initialized = false;
static uint32_t last_refresh_tick = 0;

/**
 * @brief Initialize IWDG
 */
int SecureWatchdog_Init(const WatchdogConfig_t *config)
{
    printf("[Secure Watchdog] Initializing IWDG...\n");

    if (config == NULL) {
        return -1;
    }

    /* Calculate prescaler and reload value for target timeout */
    /* IWDG clock = LSI (32 kHz) */
    /* Timeout = (Prescaler * Reload) / LSI_Frequency */

    uint32_t prescaler_div;
    uint32_t reload_value;

    /* Target: config->timeout_ms */
    /* Available prescalers: 4, 8, 16, 32, 64, 128, 256 */

    if (config->timeout_ms <= 512) {
        hiwdg.Init.Prescaler = IWDG_PRESCALER_4;
        prescaler_div = 4;
    } else if (config->timeout_ms <= 1024) {
        hiwdg.Init.Prescaler = IWDG_PRESCALER_8;
        prescaler_div = 8;
    } else if (config->timeout_ms <= 2048) {
        hiwdg.Init.Prescaler = IWDG_PRESCALER_16;
        prescaler_div = 16;
    } else if (config->timeout_ms <= 4096) {
        hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
        prescaler_div = 32;
    } else if (config->timeout_ms <= 8192) {
        hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
        prescaler_div = 64;
    } else if (config->timeout_ms <= 16384) {
        hiwdg.Init.Prescaler = IWDG_PRESCALER_128;
        prescaler_div = 128;
    } else {
        hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
        prescaler_div = 256;
    }

    /* Calculate reload value (12-bit max = 4095) */
    reload_value = (config->timeout_ms * 32) / prescaler_div;
    if (reload_value > 4095) {
        reload_value = 4095;
    }

    hiwdg.Instance = IWDG;
    hiwdg.Init.Reload = reload_value;
    hiwdg.Init.Window = IWDG_WINDOW_DISABLE;

    /* Initialize IWDG */
    if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
        printf("[Secure Watchdog] ERROR: IWDG initialization failed\n");
        return -1;
    }

    watchdog_initialized = true;
    last_refresh_tick = HAL_GetTick();

    printf("[Secure Watchdog] IWDG initialized\n");
    printf("  Timeout: %lu ms (actual: %lu ms)\n",
           config->timeout_ms,
           (prescaler_div * reload_value) / 32);
    printf("  Prescaler: %lu, Reload: %lu\n", prescaler_div, reload_value);

    return 0;
}

/**
 * @brief Refresh watchdog
 */
int SecureWatchdog_Refresh(void)
{
    if (!watchdog_initialized) {
        return -1;
    }

    /* Refresh IWDG */
    HAL_IWDG_Refresh(&hiwdg);

    uint32_t current_tick = HAL_GetTick();
    uint32_t elapsed = current_tick - last_refresh_tick;
    last_refresh_tick = current_tick;

    printf("[Secure Watchdog] Refreshed (elapsed: %lu ms)\n", elapsed);

    return 0;
}

/**
 * @brief Check if watchdog caused last reset
 */
bool SecureWatchdog_WasResetSource(void)
{
    /* Check IWDG reset flag in RCC */
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
        printf("[Secure Watchdog] ⚠️  Last reset was caused by IWDG timeout!\n");

        /* Clear flag */
        __HAL_RCC_CLEAR_RESET_FLAGS();

        return true;
    }

    return false;
}

/**
 * @brief Get watchdog status
 */
WatchdogStatus_t SecureWatchdog_GetStatus(void)
{
    if (!watchdog_initialized) {
        return WDG_STATUS_OK;
    }

    uint32_t elapsed = HAL_GetTick() - last_refresh_tick;

    /* Calculate timeout (approximate) */
    uint32_t timeout_ms = (hiwdg.Init.Reload *
                           (1 << ((hiwdg.Init.Prescaler >> 2) + 2))) / 32;

    if (elapsed > timeout_ms * 0.9) {
        return WDG_STATUS_WARNING;  // >90% of timeout
    } else if (elapsed > timeout_ms) {
        return WDG_STATUS_EXPIRED;  // Should have reset by now
    }

    return WDG_STATUS_OK;
}

/**
 * @brief Watchdog timeout callback (called just before reset)
 */
void SecureWatchdog_OnTimeoutCallback(void)
{
    printf("\n");
    printf("═══════════════════════════════════════════\n");
    printf(" ⚠️  WATCHDOG TIMEOUT - ERASING SECRETS\n");
    printf("═══════════════════════════════════════════\n");

    /* Erase all sensitive data before reset */
    psa_its_remove(0x1234);  // Encryption keys
    psa_its_remove(0x5678);  // Attestation keys

    /* Erase backup registers */
    HAL_PWR_EnableBkUpAccess();
    for (int i = 0; i < 32; i++) {
        HAL_RTCEx_BKUPWrite(&hrtc, i, 0x00000000);
    }

    /* Zero cryptographic memory */
    /* (In production, zero all key buffers in SRAM) */

    printf("[Secure Watchdog] Secrets erased - device will reset\n");

    /* Device will reset shortly */
}
```

### Testing Exercise 1

```c
void test_secure_watchdog(void)
{
    printf("\n=== Exercise 1: Secure Watchdog ===\n");

    /* Check if last reset was from watchdog */
    if (SecureWatchdog_WasResetSource()) {
        printf("⚠️  Device recovered from watchdog reset!\n");
        LED_Red_Blink(5);
        HAL_Delay(2000);
    }

    /* Configure watchdog: 5 second timeout */
    WatchdogConfig_t config = {
        .timeout_ms = 5000,
        .enable_early_warning = false,
        .secure_only = true
    };

    SecureWatchdog_Init(&config);
    printf("✓ Watchdog initialized with 5s timeout\n");
    LED_Green_Blink(1);

    /* Test 1: Normal operation - refresh every 2 seconds */
    printf("\n[Test 1] Normal operation (refresh every 2s)...\n");
    for (int i = 0; i < 5; i++) {
        HAL_Delay(2000);
        SecureWatchdog_Refresh();
        LED_Blue_Toggle();

        WatchdogStatus_t status = SecureWatchdog_GetStatus();
        if (status == WDG_STATUS_WARNING) {
            printf("⚠️  WARNING: Watchdog timeout approaching!\n");
        }
    }
    printf("✓ Test 1 passed - watchdog refreshed correctly\n");

    /* Test 2: Simulate hang (DO NOT REFRESH) */
    printf("\n[Test 2] Simulating system hang...\n");
    printf("⚠️  Will NOT refresh watchdog - device will reset in 5s\n");
    printf("⚠️  Secrets will be erased before reset!\n");

    LED_Red_On();

    /* DO NOT CALL SecureWatchdog_Refresh() */
    /* Wait for watchdog to expire (5 seconds) */

    for (int i = 5; i > 0; i--) {
        printf("  Reset in %d seconds...\n", i);
        HAL_Delay(1000);
    }

    /* This code should NOT execute - device should reset */
    printf("❌ ERROR: Watchdog failed to reset device!\n");
    LED_Red_Blink(10);
}
```

**Expected Output:**
```
=== Exercise 1: Secure Watchdog ===

[Secure Watchdog] Initializing IWDG...
[Secure Watchdog] IWDG initialized
  Timeout: 5000 ms (actual: 5000 ms)
  Prescaler: 32, Reload: 5000
✓ Watchdog initialized with 5s timeout

[Test 1] Normal operation (refresh every 2s)...
[Secure Watchdog] Refreshed (elapsed: 2000 ms)
[Secure Watchdog] Refreshed (elapsed: 2001 ms)
[Secure Watchdog] Refreshed (elapsed: 2000 ms)
[Secure Watchdog] Refreshed (elapsed: 2001 ms)
[Secure Watchdog] Refreshed (elapsed: 2000 ms)
✓ Test 1 passed - watchdog refreshed correctly

[Test 2] Simulating system hang...
⚠️  Will NOT refresh watchdog - device will reset in 5s
⚠️  Secrets will be erased before reset!
  Reset in 5 seconds...
  Reset in 4 seconds...
  Reset in 3 seconds...
  Reset in 2 seconds...
  Reset in 1 seconds...

═══════════════════════════════════════════
 ⚠️  WATCHDOG TIMEOUT - ERASING SECRETS
═══════════════════════════════════════════
[Secure Watchdog] Secrets erased - device will reset

[DEVICE RESETS]

=== Exercise 1: Secure Watchdog ===

[Secure Watchdog] ⚠️  Last reset was caused by IWDG timeout!
⚠️  Device recovered from watchdog reset!
```

---

## Exercise 2: Window Watchdog (WWDG) for Timing Constraints

### Objective
Use WWDG to enforce timing constraints and detect timing attacks.

### Theory

**Window Watchdog:**
- Requires refresh within a specific time window
- Too early refresh = reset
- Too late refresh = reset
- Detects both hangs AND timing anomalies

### Implementation

**File: `secure_wwdg.h`**
```c
#ifndef SECURE_WWDG_H
#define SECURE_WWDG_H

#include <stdint.h>
#include "stm32u5xx_hal.h"

/**
 * @brief Initialize Window Watchdog
 * @param window_ms Window duration in milliseconds
 * @param counter_ms Counter reload value in milliseconds
 * @return 0 on success
 */
int SecureWWDG_Init(uint32_t window_ms, uint32_t counter_ms);

/**
 * @brief Refresh WWDG (must be called within window)
 * @return 0 on success, -1 if too early
 */
int SecureWWDG_Refresh(void);

/**
 * @brief WWDG early warning callback
 */
void SecureWWDG_EarlyWarningCallback(void);

#endif /* SECURE_WWDG_H */
```

**File: `secure_wwdg.c`**
```c
#include "secure_wwdg.h"
#include <stdio.h>

static WWDG_HandleTypeDef hwwdg;

int SecureWWDG_Init(uint32_t window_ms, uint32_t counter_ms)
{
    printf("[Secure WWDG] Initializing Window Watchdog...\n");

    /* WWDG clock = PCLK1 / 4096 / Prescaler */
    /* For PCLK1 = 160 MHz: WWDG clock = ~39 kHz (prescaler = 1) */

    hwwdg.Instance = WWDG;
    hwwdg.Init.Prescaler = WWDG_PRESCALER_1;
    hwwdg.Init.Window = 0x7F;    // Maximum window
    hwwdg.Init.Counter = 0x7F;    // Maximum counter
    hwwdg.Init.EWIMode = WWDG_EWI_ENABLE;  // Early warning interrupt

    if (HAL_WWDG_Init(&hwwdg) != HAL_OK) {
        printf("[Secure WWDG] ERROR: Initialization failed\n");
        return -1;
    }

    printf("[Secure WWDG] Initialized successfully\n");
    return 0;
}

int SecureWWDG_Refresh(void)
{
    HAL_WWDG_Refresh(&hwwdg);
    printf("[Secure WWDG] Refreshed\n");
    return 0;
}

void SecureWWDG_EarlyWarningCallback(void)
{
    printf("[Secure WWDG] ⚠️  Early Warning - refresh required!\n");
    LED_Blue_Toggle();
}

void HAL_WWDG_EarlyWakeupCallback(WWDG_HandleTypeDef *hwwdg)
{
    SecureWWDG_EarlyWarningCallback();
}
```

---

## Exercise 3: Timeout-Based Security Policies

### Objective
Implement security policies triggered by timeouts (e.g., authentication expiration).

### Implementation

```c
typedef struct {
    uint32_t auth_timeout_ms;      // Authentication session timeout
    uint32_t idle_timeout_ms;       // Idle timeout before lock
    uint32_t crypto_op_timeout_ms;  // Maximum cryptographic operation time
} SecurityTimeoutPolicy_t;

typedef enum {
    SESSION_ACTIVE,
    SESSION_IDLE,
    SESSION_EXPIRED,
    SESSION_LOCKED
} SessionState_t;

static uint32_t last_activity_tick = 0;
static SessionState_t current_session = SESSION_EXPIRED;

int SecurityTimeout_Init(const SecurityTimeoutPolicy_t *policy)
{
    printf("[Security Timeout] Initializing timeout policies...\n");
    printf("  Auth timeout: %lu ms\n", policy->auth_timeout_ms);
    printf("  Idle timeout: %lu ms\n", policy->idle_timeout_ms);
    printf("  Crypto timeout: %lu ms\n", policy->crypto_op_timeout_ms);
    return 0;
}

void SecurityTimeout_UpdateActivity(void)
{
    last_activity_tick = HAL_GetTick();

    if (current_session == SESSION_IDLE) {
        printf("[Security Timeout] Session resumed from idle\n");
        current_session = SESSION_ACTIVE;
    }
}

SessionState_t SecurityTimeout_CheckSession(const SecurityTimeoutPolicy_t *policy)
{
    uint32_t idle_time = HAL_GetTick() - last_activity_tick;

    if (idle_time > policy->auth_timeout_ms) {
        if (current_session != SESSION_EXPIRED) {
            printf("[Security Timeout] ⚠️  Session EXPIRED (idle: %lu ms)\n", idle_time);
            current_session = SESSION_EXPIRED;

            /* Erase session keys */
            psa_its_remove(0x9999);  // Session key

            LED_Red_Blink(3);
        }
        return SESSION_EXPIRED;
    } else if (idle_time > policy->idle_timeout_ms) {
        if (current_session == SESSION_ACTIVE) {
            printf("[Security Timeout] Session IDLE (idle: %lu ms)\n", idle_time);
            current_session = SESSION_IDLE;
            LED_Blue_On();
        }
        return SESSION_IDLE;
    }

    return SESSION_ACTIVE;
}

int SecurityTimeout_CheckCryptoOperation(uint32_t start_tick,
                                          uint32_t max_duration_ms)
{
    uint32_t elapsed = HAL_GetTick() - start_tick;

    if (elapsed > max_duration_ms) {
        printf("[Security Timeout] ❌ Crypto operation timeout! (%lu ms)\n", elapsed);
        printf("  Possible timing attack detected\n");
        return -1;
    }

    return 0;
}
```

### Testing Exercise 3

```c
void test_security_timeouts(void)
{
    printf("\n=== Exercise 3: Timeout-Based Security ===\n");

    SecurityTimeoutPolicy_t policy = {
        .auth_timeout_ms = 10000,   // 10 seconds
        .idle_timeout_ms = 5000,     // 5 seconds
        .crypto_op_timeout_ms = 100  // 100 ms
    };

    SecurityTimeout_Init(&policy);

    /* Test 1: Normal activity */
    printf("\n[Test 1] Normal activity...\n");
    current_session = SESSION_ACTIVE;
    SecurityTimeout_UpdateActivity();

    for (int i = 0; i < 3; i++) {
        HAL_Delay(2000);
        SecurityTimeout_UpdateActivity();
        printf("  Activity at %lu ms\n", HAL_GetTick());
    }

    SessionState_t state = SecurityTimeout_CheckSession(&policy);
    if (state == SESSION_ACTIVE) {
        printf("✓ Session remains ACTIVE\n");
        LED_Green_Blink(1);
    }

    /* Test 2: Idle timeout */
    printf("\n[Test 2] Idle timeout (no activity for 6s)...\n");
    HAL_Delay(6000);

    state = SecurityTimeout_CheckSession(&policy);
    if (state == SESSION_IDLE) {
        printf("✓ Session transitioned to IDLE\n");
        LED_Blue_Blink(2);
    }

    /* Test 3: Authentication expiration */
    printf("\n[Test 3] Authentication expiration (idle for 11s total)...\n");
    HAL_Delay(5000);

    state = SecurityTimeout_CheckSession(&policy);
    if (state == SESSION_EXPIRED) {
        printf("✓ Session EXPIRED and keys erased\n");
        LED_Red_Blink(3);
    }

    /* Test 4: Crypto operation timeout */
    printf("\n[Test 4] Crypto operation timeout detection...\n");
    uint32_t start = HAL_GetTick();

    /* Simulate slow crypto operation */
    HAL_Delay(150);  // 150 ms > 100 ms limit

    if (SecurityTimeout_CheckCryptoOperation(start, policy.crypto_op_timeout_ms) != 0) {
        printf("✓ Timeout detected (possible timing attack)\n");
        LED_Red_Blink(2);
    }

    printf("\n✓ Exercise 3: COMPLETE\n");
}
```

---

## Key Takeaways

1. ✅ **IWDG detects system hangs** and forces recovery
2. ✅ **WWDG enforces timing constraints** (window-based refresh)
3. ✅ **Erase secrets before watchdog reset** (security response)
4. ✅ **Timeout-based security policies** prevent session hijacking
5. ✅ **Detect timing attacks** with crypto operation timeouts
6. ✅ **Watchdog in Secure world** prevents Non-Secure manipulation
7. ✅ **Check reset source** to detect watchdog resets

---

## Best Practices

1. **Always erase secrets** before watchdog timeout
2. **Use IWDG for critical systems** (independent LSI clock)
3. **Use WWDG for timing constraints** (e.g., cryptographic operations)
4. **Implement session timeouts** for authentication
5. **Log watchdog resets** for debugging
6. **Test watchdog behavior** in low-power modes
7. **Configure watchdog early** in boot sequence

---

## References

- [STM32U5 Watchdog Guide (RM0456)](https://www.st.com/resource/en/reference_manual/rm0456-stm32u5-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [Watchdog Security Best Practices (AN4435)](https://www.st.com/resource/en/application_note/an4435-using-the-stm32-mcus-independent-watchdog-iwdg-stmicroelectronics.pdf)

---

**Lab 15 Complete! 🎉**

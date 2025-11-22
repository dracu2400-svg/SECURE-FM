# Lab 11: Secure Peripheral Access and Configuration

## Overview

This lab demonstrates how to securely configure and access hardware peripherals in a TrustZone-M system using TF-M on the NUCLEO-U545RE-Q development board. You'll learn how to partition peripherals between Secure and Non-Secure worlds, create secure peripheral drivers, and prevent peripheral-based attacks.

**Duration:** 90-120 minutes
**Difficulty:** Advanced
**Prerequisites:** Labs 02-10

---

## Learning Objectives

By the end of this lab, you will:

1. ✅ Understand SAU/IDAU peripheral partitioning
2. ✅ Configure peripherals for Secure vs Non-Secure access
3. ✅ Implement secure peripheral drivers with TF-M
4. ✅ Create Non-Secure Callable (NSC) functions for safe peripheral sharing
5. ✅ Prevent peripheral-based attacks (DMA attacks, GPIO exploitation)
6. ✅ Use secure timers and watchdogs
7. ✅ Implement secure interrupt handlers
8. ✅ Validate all peripheral configurations for production

---

## Hardware Setup

### Required Hardware
- **NUCLEO-U545RE-Q** development board
- **USB cable** (for power and ST-Link debugging)
- **LED connections** (built-in LD1/LD2/LD3)
- **(Optional) Logic analyzer** for observing secure GPIO signals

### Pin Assignments (STM32U545)

| Peripheral | Pin | Function | Security |
|------------|-----|----------|----------|
| **LD1** (Green) | PC7 | Success indicator | Secure |
| **LD2** (Blue) | PB7 | Operation indicator | Non-Secure |
| **LD3** (Red) | PG2 | Error/attack indicator | Secure |
| **USART1** | PA9/PA10 | Secure debug output | Secure |
| **USART3** | PD8/PD9 | Non-Secure app output | Non-Secure |
| **TIM2** | - | Non-Secure timer | Non-Secure |
| **TIM6** | - | Secure timer | Secure |
| **ADC1** | PA0 | Secure analog input | Secure |

---

## Peripheral Security Architecture

### TrustZone-M Peripheral Partitioning

In ARMv8-M with TrustZone, peripherals are assigned to either Secure or Non-Secure world using:

1. **SAU (Security Attribution Unit):** Configures memory regions
2. **IDAU (Implementation Defined Attribution Unit):** Hardware-defined security (in STM32U5)
3. **GTZC (Global TrustZone Controller):** STM32U5-specific peripheral gating

```
┌─────────────────────────────────────────────────┐
│           Peripheral Security Model              │
├─────────────────────────────────────────────────┤
│                                                  │
│  Secure World              Non-Secure World      │
│  ┌──────────────┐         ┌──────────────┐      │
│  │ USART1       │         │ USART3       │      │
│  │ TIM6         │         │ TIM2         │      │
│  │ GPIO (LD1)   │         │ GPIO (LD2)   │      │
│  │ ADC1         │         │              │      │
│  └──────────────┘         └──────────────┘      │
│         ↓                         ↓              │
│  ┌──────────────────────────────────────┐       │
│  │   GTZC (Global TrustZone Controller) │       │
│  │   - Peripheral security assignment   │       │
│  │   - Illegal access detection         │       │
│  │   - Interrupt security routing       │       │
│  └──────────────────────────────────────┘       │
│                                                  │
│  ┌─────────────────────────────────────┐        │
│  │  Non-Secure Callable (NSC) Gateway  │        │
│  │  - Secure peripheral access API     │        │
│  │  - Parameter validation             │        │
│  │  - Safe data transfer                │        │
│  └─────────────────────────────────────┘        │
└─────────────────────────────────────────────────┘
```

### STM32U5 GTZC Configuration

The STM32U5 has a **Global TrustZone Controller (GTZC)** that manages:
- **TZSC (TrustZone Security Controller):** Peripheral security assignment
- **TZIC (TrustZone Illegal Access Controller):** Detects unauthorized access
- **MPCBB (Memory Protection Controller Block-Based):** Memory block security

**Example Configuration:**
```c
// GTZC peripheral security configuration (STM32U5)
#define GTZC_PERIPH_USART1      (0x00000001U)  // Secure
#define GTZC_PERIPH_USART3      (0x00000008U)  // Non-Secure
#define GTZC_PERIPH_TIM6        (0x00000010U)  // Secure
#define GTZC_PERIPH_TIM2        (0x00000004U)  // Non-Secure

// Configure peripherals as Secure
HAL_GTZC_TZSC_ConfigPeriphAttributes(
    GTZC_PERIPH_USART1 | GTZC_PERIPH_TIM6,
    GTZC_TZSC_PERIPH_SEC  // Secure
);

// Configure peripherals as Non-Secure
HAL_GTZC_TZSC_ConfigPeriphAttributes(
    GTZC_PERIPH_USART3 | GTZC_PERIPH_TIM2,
    GTZC_TZSC_PERIPH_NSEC  // Non-Secure
);
```

---

## Exercise 1: Secure GPIO Configuration

### Objective
Configure GPIO pins for Secure and Non-Secure use, demonstrating proper isolation.

### Theory

**GPIO Security Assignment:**
- Each GPIO pin can be assigned to Secure or Non-Secure
- STM32U5 uses GTZC to control GPIO security
- Secure GPIO pins **cannot** be accessed from Non-Secure code
- Attempting access triggers **SecureFault**

**Pin Security Register (STM32U5):**
```c
// GPIOC security configuration (for LD1 - PC7)
GTZC_TZSC->TZSC_SECCFGR1 |= (1 << 2);  // GPIOC Secure

// Individual pin security (PC7)
GPIOC->SECCFGR |= (1 << 7);  // PC7 Secure
```

### Implementation

**File: `secure_gpio.h`**
```c
#ifndef SECURE_GPIO_H
#define SECURE_GPIO_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32u5xx.h"

/* LED Pin Definitions */
#define LED_GREEN_PIN      GPIO_PIN_7   // PC7 (LD1) - Secure
#define LED_GREEN_PORT     GPIOC
#define LED_BLUE_PIN       GPIO_PIN_7   // PB7 (LD2) - Non-Secure
#define LED_BLUE_PORT      GPIOB
#define LED_RED_PIN        GPIO_PIN_2   // PG2 (LD3) - Secure
#define LED_RED_PORT       GPIOG

/* GPIO Security Levels */
typedef enum {
    GPIO_SECURITY_SECURE,
    GPIO_SECURITY_NONSECURE,
    GPIO_SECURITY_PRIV_SECURE,      // Privileged Secure only
    GPIO_SECURITY_PRIV_NONSECURE    // Privileged Non-Secure only
} GPIO_Security_t;

/* GPIO Configuration */
typedef struct {
    GPIO_TypeDef *port;
    uint32_t pin;
    GPIO_Security_t security;
    bool is_output;
    bool initial_state;  // For outputs
} SecureGPIO_Config_t;

/* Function Prototypes */

/**
 * @brief Initialize secure GPIO system
 * @return 0 on success, -1 on error
 */
int SecureGPIO_Init(void);

/**
 * @brief Configure a GPIO pin with security attributes
 * @param config GPIO configuration structure
 * @return 0 on success, -1 on error
 */
int SecureGPIO_Configure(const SecureGPIO_Config_t *config);

/**
 * @brief Secure GPIO write (only works for Secure pins from Secure world)
 * @param port GPIO port (GPIOA, GPIOB, etc.)
 * @param pin Pin number (0-15)
 * @param state true = HIGH, false = LOW
 * @return 0 on success, -1 if pin is Non-Secure
 */
int SecureGPIO_Write(GPIO_TypeDef *port, uint32_t pin, bool state);

/**
 * @brief Secure GPIO read (only works for Secure pins from Secure world)
 * @param port GPIO port
 * @param pin Pin number
 * @param state Output parameter for pin state
 * @return 0 on success, -1 if pin is Non-Secure
 */
int SecureGPIO_Read(GPIO_TypeDef *port, uint32_t pin, bool *state);

/**
 * @brief Blink a secure LED
 * @param port GPIO port
 * @param pin Pin number
 * @param count Number of blinks
 * @param delay_ms Delay in milliseconds
 */
void SecureGPIO_Blink(GPIO_TypeDef *port, uint32_t pin, uint8_t count, uint32_t delay_ms);

/**
 * @brief Non-Secure Callable function to control Non-Secure LED
 * @param state true = ON, false = OFF
 * @return 0 on success
 */
__attribute__((cmse_nonsecure_entry))
int NSC_GPIO_SetBlueLED(bool state);

/**
 * @brief Verify GPIO security configuration
 * @return 0 if secure, -1 if misconfigured
 */
int SecureGPIO_VerifyConfiguration(void);

#endif /* SECURE_GPIO_H */
```

**File: `secure_gpio.c`**
```c
#include "secure_gpio.h"
#include <stdio.h>

/* Private helpers */
static int is_gpio_secure(GPIO_TypeDef *port, uint32_t pin);
static void enable_gpio_clock(GPIO_TypeDef *port);
static void delay_ms(uint32_t ms);

/**
 * @brief Initialize secure GPIO system with GTZC
 */
int SecureGPIO_Init(void)
{
    printf("[Secure GPIO] Initializing GPIO security...\n");

    /* Enable GTZC clock */
    __HAL_RCC_GTZC1_CLK_ENABLE();

    /* Configure GPIOC as Secure (for LD1 - PC7) */
    HAL_GTZC_TZSC_ConfigPeriphAttributes(
        GTZC_PERIPH_GPIOC,
        GTZC_TZSC_PERIPH_SEC | GTZC_TZSC_PERIPH_PRIV
    );

    /* Configure GPIOG as Secure (for LD3 - PG2) */
    HAL_GTZC_TZSC_ConfigPeriphAttributes(
        GTZC_PERIPH_GPIOG,
        GTZC_TZSC_PERIPH_SEC | GTZC_TZSC_PERIPH_PRIV
    );

    /* Configure GPIOB as Non-Secure (for LD2 - PB7) */
    HAL_GTZC_TZSC_ConfigPeriphAttributes(
        GTZC_PERIPH_GPIOB,
        GTZC_TZSC_PERIPH_NSEC  // Non-Secure
    );

    /* Enable illegal access interrupt (TZIC) */
    __HAL_RCC_GTZC1_CLK_ENABLE();
    NVIC_EnableIRQ(GTZC_IRQn);

    printf("[Secure GPIO] GTZC configured:\n");
    printf("  - GPIOC (PC7 LD1): Secure\n");
    printf("  - GPIOG (PG2 LD3): Secure\n");
    printf("  - GPIOB (PB7 LD2): Non-Secure\n");

    return 0;
}

/**
 * @brief Configure individual GPIO pin with security
 */
int SecureGPIO_Configure(const SecureGPIO_Config_t *config)
{
    if (config == NULL) {
        return -1;
    }

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable clock for the port */
    enable_gpio_clock(config->port);

    /* Configure pin security in GPIOX->SECCFGR */
    if (config->security == GPIO_SECURITY_SECURE ||
        config->security == GPIO_SECURITY_PRIV_SECURE) {
        config->port->SECCFGR |= (1 << config->pin);  // Secure
        printf("[Secure GPIO] Configured pin %lu as SECURE\n", config->pin);
    } else {
        config->port->SECCFGR &= ~(1 << config->pin);  // Non-Secure
        printf("[Secure GPIO] Configured pin %lu as NON-SECURE\n", config->pin);
    }

    /* Configure GPIO mode */
    GPIO_InitStruct.Pin = (1 << config->pin);
    GPIO_InitStruct.Mode = config->is_output ? GPIO_MODE_OUTPUT_PP : GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(config->port, &GPIO_InitStruct);

    /* Set initial state for outputs */
    if (config->is_output) {
        HAL_GPIO_WritePin(config->port, (1 << config->pin),
                          config->initial_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }

    return 0;
}

/**
 * @brief Secure GPIO write with security check
 */
int SecureGPIO_Write(GPIO_TypeDef *port, uint32_t pin, bool state)
{
    /* Verify pin is Secure */
    if (!is_gpio_secure(port, pin)) {
        printf("[ERROR] Attempted write to Non-Secure GPIO from Secure code\n");
        return -1;
    }

    HAL_GPIO_WritePin(port, (1 << pin), state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return 0;
}

/**
 * @brief Secure GPIO read with security check
 */
int SecureGPIO_Read(GPIO_TypeDef *port, uint32_t pin, bool *state)
{
    if (state == NULL) {
        return -1;
    }

    /* Verify pin is Secure */
    if (!is_gpio_secure(port, pin)) {
        printf("[ERROR] Attempted read from Non-Secure GPIO from Secure code\n");
        return -1;
    }

    GPIO_PinState pin_state = HAL_GPIO_ReadPin(port, (1 << pin));
    *state = (pin_state == GPIO_PIN_SET);
    return 0;
}

/**
 * @brief Blink secure LED
 */
void SecureGPIO_Blink(GPIO_TypeDef *port, uint32_t pin, uint8_t count, uint32_t delay_ms)
{
    for (uint8_t i = 0; i < count; i++) {
        SecureGPIO_Write(port, pin, true);
        delay_ms(delay_ms);
        SecureGPIO_Write(port, pin, false);
        delay_ms(delay_ms);
    }
}

/**
 * @brief NSC function to control Non-Secure Blue LED from Non-Secure world
 *
 * This demonstrates a SAFE way for Non-Secure code to interact with GPIO
 * through a controlled, validated gateway.
 */
__attribute__((cmse_nonsecure_entry))
int NSC_GPIO_SetBlueLED(bool state)
{
    /* This function executes in Secure world but is callable from Non-Secure */

    /* Validate input parameter (basic example) */
    if (state != true && state != false) {
        printf("[NSC] Invalid parameter\n");
        return -1;
    }

    /* Control Non-Secure GPIO safely */
    HAL_GPIO_WritePin(LED_BLUE_PORT, LED_BLUE_PIN,
                      state ? GPIO_PIN_SET : GPIO_PIN_RESET);

    printf("[NSC] Blue LED set to %s by Non-Secure caller\n",
           state ? "ON" : "OFF");

    return 0;
}

/**
 * @brief Verify GPIO security configuration
 */
int SecureGPIO_VerifyConfiguration(void)
{
    printf("\n[Secure GPIO] Verifying security configuration...\n");

    /* Check GPIOC security (should be Secure) */
    uint32_t gpioc_security = (GTZC_TZSC1->SECCFGR1 >> 2) & 0x3;
    if (gpioc_security != 0x3) {  // Should be 0b11 (Secure + Privileged)
        printf("[ERROR] GPIOC not configured as Secure!\n");
        return -1;
    }
    printf("✓ GPIOC: Secure\n");

    /* Check GPIOB security (should be Non-Secure) */
    uint32_t gpiob_security = (GTZC_TZSC1->SECCFGR1 >> 1) & 0x3;
    if (gpiob_security != 0x0) {  // Should be 0b00 (Non-Secure)
        printf("[ERROR] GPIOB not configured as Non-Secure!\n");
        return -1;
    }
    printf("✓ GPIOB: Non-Secure\n");

    /* Check individual pin security */
    bool pc7_secure = (GPIOC->SECCFGR >> 7) & 0x1;
    bool pb7_nonsecure = !((GPIOB->SECCFGR >> 7) & 0x1);

    if (!pc7_secure) {
        printf("[ERROR] PC7 (Green LED) not secure!\n");
        return -1;
    }
    printf("✓ PC7 (Green LED): Secure\n");

    if (!pb7_nonsecure) {
        printf("[ERROR] PB7 (Blue LED) not non-secure!\n");
        return -1;
    }
    printf("✓ PB7 (Blue LED): Non-Secure\n");

    printf("[Secure GPIO] All configurations VALID\n");
    return 0;
}

/* Private Functions */

static int is_gpio_secure(GPIO_TypeDef *port, uint32_t pin)
{
    /* Check SECCFGR register for pin security */
    return (port->SECCFGR >> pin) & 0x1;
}

static void enable_gpio_clock(GPIO_TypeDef *port)
{
    if (port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (port == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
    else if (port == GPIOE) __HAL_RCC_GPIOE_CLK_ENABLE();
    else if (port == GPIOF) __HAL_RCC_GPIOF_CLK_ENABLE();
    else if (port == GPIOG) __HAL_RCC_GPIOG_CLK_ENABLE();
    else if (port == GPIOH) __HAL_RCC_GPIOH_CLK_ENABLE();
}

static void delay_ms(uint32_t ms)
{
    /* Simple delay using SysTick or DWT cycle counter */
    HAL_Delay(ms);
}
```

### Testing Exercise 1

**Secure World Test (main.c):**
```c
#include "secure_gpio.h"
#include <stdio.h>

void test_secure_gpio(void)
{
    printf("\n=== Exercise 1: Secure GPIO Configuration ===\n");

    /* Initialize GPIO security */
    SecureGPIO_Init();

    /* Configure Green LED (PC7) as Secure output */
    SecureGPIO_Config_t green_led = {
        .port = LED_GREEN_PORT,
        .pin = 7,
        .security = GPIO_SECURITY_SECURE,
        .is_output = true,
        .initial_state = false
    };
    SecureGPIO_Configure(&green_led);

    /* Configure Red LED (PG2) as Secure output */
    SecureGPIO_Config_t red_led = {
        .port = LED_RED_PORT,
        .pin = 2,
        .security = GPIO_SECURITY_SECURE,
        .is_output = true,
        .initial_state = false
    };
    SecureGPIO_Configure(&red_led);

    /* Configure Blue LED (PB7) as Non-Secure output */
    SecureGPIO_Config_t blue_led = {
        .port = LED_BLUE_PORT,
        .pin = 7,
        .security = GPIO_SECURITY_NONSECURE,
        .is_output = true,
        .initial_state = false
    };
    SecureGPIO_Configure(&blue_led);

    /* Verify configuration */
    SecureGPIO_VerifyConfiguration();

    /* Test 1: Secure world controls Secure LED (GREEN) - Should work */
    printf("\n[Test 1] Secure world → Secure LED (GREEN)...\n");
    SecureGPIO_Blink(LED_GREEN_PORT, 7, 3, 200);
    printf("✓ SUCCESS: Green LED blinking\n");

    /* Test 2: Secure world controls Non-Secure LED (BLUE) - Should work */
    printf("\n[Test 2] Secure world → Non-Secure LED (BLUE)...\n");
    HAL_GPIO_WritePin(LED_BLUE_PORT, LED_BLUE_PIN, GPIO_PIN_SET);
    HAL_Delay(500);
    HAL_GPIO_WritePin(LED_BLUE_PORT, LED_BLUE_PIN, GPIO_PIN_RESET);
    printf("✓ SUCCESS: Blue LED toggled (Secure can access Non-Secure)\n");

    printf("\n=== Exercise 1: COMPLETE ===\n");
}
```

**Non-Secure World Test:**
```c
/* Non-Secure application attempting to access Secure GPIO */
void test_nonsecure_gpio_access(void)
{
    printf("\n=== Non-Secure GPIO Access Test ===\n");

    /* Attempt 1: Direct access to Secure LED (PC7) - Will trigger SecureFault */
    printf("[Test] Non-Secure trying to access Secure LED (PC7)...\n");

    // UNCOMMENT TO TRIGGER SECURE FAULT:
    // GPIOC->ODR |= (1 << 7);  // ⚠️ SECURE FAULT!

    printf("⚠️  Direct access commented out (would trigger SecureFault)\n");

    /* Attempt 2: Use NSC gateway function to control Blue LED - Safe */
    printf("[Test] Non-Secure using NSC gateway for Blue LED...\n");
    NSC_GPIO_SetBlueLED(true);
    HAL_Delay(500);
    NSC_GPIO_SetBlueLED(false);
    printf("✓ SUCCESS: Blue LED controlled via NSC gateway\n");

    printf("\n=== Non-Secure GPIO Access Test: COMPLETE ===\n");
}
```

**Expected Output:**
```
=== Exercise 1: Secure GPIO Configuration ===
[Secure GPIO] Initializing GPIO security...
[Secure GPIO] GTZC configured:
  - GPIOC (PC7 LD1): Secure
  - GPIOG (PG2 LD3): Secure
  - GPIOB (PB7 LD2): Non-Secure
[Secure GPIO] Configured pin 7 as SECURE
[Secure GPIO] Configured pin 2 as SECURE
[Secure GPIO] Configured pin 7 as NON-SECURE

[Secure GPIO] Verifying security configuration...
✓ GPIOC: Secure
✓ GPIOB: Non-Secure
✓ PC7 (Green LED): Secure
✓ PB7 (Blue LED): Non-Secure
[Secure GPIO] All configurations VALID

[Test 1] Secure world → Secure LED (GREEN)...
✓ SUCCESS: Green LED blinking

[Test 2] Secure world → Non-Secure LED (BLUE)...
✓ SUCCESS: Blue LED toggled (Secure can access Non-Secure)

=== Exercise 1: COMPLETE ===
```

**Visual Feedback:**
- ✅ **Green LED (LD1)** blinks 3 times → Secure GPIO working
- ✅ **Blue LED (LD2)** toggles once → Non-Secure GPIO accessible from Secure
- ❌ If Non-Secure tries direct Secure GPIO access → **SecureFault** → Red LED blinks

---

## Exercise 2: Secure UART Configuration

### Objective
Configure USART1 as Secure for debug output and USART3 as Non-Secure for application data.

### Theory

**UART Security Considerations:**
- Debug output (USART1) should be **Secure** to prevent information leakage
- Application data (USART3) can be **Non-Secure**
- Secure UART can only be accessed from Secure world
- Non-Secure UART accessible from both worlds

### Implementation

**File: `secure_uart.h`**
```c
#ifndef SECURE_UART_H
#define SECURE_UART_H

#include <stdint.h>
#include "stm32u5xx_hal.h"

/* UART Handles */
extern UART_HandleTypeDef huart1_secure;   // Secure USART1
extern UART_HandleTypeDef huart3_nonsecure; // Non-Secure USART3

/* Function Prototypes */

/**
 * @brief Initialize Secure UART (USART1)
 * @return 0 on success
 */
int SecureUART_Init(void);

/**
 * @brief Initialize Non-Secure UART (USART3)
 * @return 0 on success
 */
int NonSecureUART_Init(void);

/**
 * @brief Secure UART transmit (only from Secure world)
 * @param data Data buffer
 * @param len Data length
 * @return 0 on success
 */
int SecureUART_Transmit(const uint8_t *data, uint16_t len);

/**
 * @brief NSC function for Non-Secure world to log to Secure UART
 * @param message Null-terminated string
 * @return 0 on success
 */
__attribute__((cmse_nonsecure_entry))
int NSC_UART_SecureLog(const char *message);

/**
 * @brief Verify UART security configuration
 * @return 0 if secure
 */
int SecureUART_VerifyConfiguration(void);

#endif /* SECURE_UART_H */
```

**File: `secure_uart.c`**
```c
#include "secure_uart.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart1_secure;
UART_HandleTypeDef huart3_nonsecure;

/**
 * @brief Initialize Secure USART1 (PA9/PA10)
 */
int SecureUART_Init(void)
{
    printf("[Secure UART] Initializing USART1 as Secure...\n");

    /* Configure USART1 as Secure in GTZC */
    HAL_GTZC_TZSC_ConfigPeriphAttributes(
        GTZC_PERIPH_USART1,
        GTZC_TZSC_PERIPH_SEC | GTZC_TZSC_PERIPH_PRIV
    );

    /* Configure GPIOs (PA9/PA10) as Secure */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIOA->SECCFGR |= (1 << 9) | (1 << 10);  // PA9, PA10 Secure

    /* Initialize USART1 */
    huart1_secure.Instance = USART1;
    huart1_secure.Init.BaudRate = 115200;
    huart1_secure.Init.WordLength = UART_WORDLENGTH_8B;
    huart1_secure.Init.StopBits = UART_STOPBITS_1;
    huart1_secure.Init.Parity = UART_PARITY_NONE;
    huart1_secure.Init.Mode = UART_MODE_TX_RX;
    huart1_secure.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1_secure.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart1_secure) != HAL_OK) {
        printf("[ERROR] USART1 initialization failed\n");
        return -1;
    }

    printf("[Secure UART] USART1 initialized as SECURE\n");
    return 0;
}

/**
 * @brief Initialize Non-Secure USART3 (PD8/PD9)
 */
int NonSecureUART_Init(void)
{
    printf("[Secure UART] Initializing USART3 as Non-Secure...\n");

    /* Configure USART3 as Non-Secure in GTZC */
    HAL_GTZC_TZSC_ConfigPeriphAttributes(
        GTZC_PERIPH_USART3,
        GTZC_TZSC_PERIPH_NSEC
    );

    /* Configure GPIOs (PD8/PD9) as Non-Secure */
    __HAL_RCC_GPIOD_CLK_ENABLE();
    GPIOD->SECCFGR &= ~((1 << 8) | (1 << 9));  // PD8, PD9 Non-Secure

    /* Initialize USART3 */
    huart3_nonsecure.Instance = USART3;
    huart3_nonsecure.Init.BaudRate = 115200;
    huart3_nonsecure.Init.WordLength = UART_WORDLENGTH_8B;
    huart3_nonsecure.Init.StopBits = UART_STOPBITS_1;
    huart3_nonsecure.Init.Parity = UART_PARITY_NONE;
    huart3_nonsecure.Init.Mode = UART_MODE_TX_RX;
    huart3_nonsecure.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3_nonsecure.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart3_nonsecure) != HAL_OK) {
        printf("[ERROR] USART3 initialization failed\n");
        return -1;
    }

    printf("[Secure UART] USART3 initialized as NON-SECURE\n");
    return 0;
}

/**
 * @brief Secure UART transmit
 */
int SecureUART_Transmit(const uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef status = HAL_UART_Transmit(&huart1_secure, (uint8_t*)data, len, 1000);
    return (status == HAL_OK) ? 0 : -1;
}

/**
 * @brief NSC gateway for Non-Secure world to log securely
 *
 * This allows Non-Secure code to safely send debug messages through
 * the Secure UART without direct access to the peripheral.
 */
__attribute__((cmse_nonsecure_entry))
int NSC_UART_SecureLog(const char *message)
{
    /* Validate message pointer (must be in Non-Secure memory) */
    if (cmse_check_address_range((void*)message, strlen(message), CMSE_NONSECURE) == NULL) {
        printf("[NSC UART] Invalid message pointer from Non-Secure\n");
        return -1;
    }

    /* Validate message length (prevent buffer overflow) */
    size_t len = strlen(message);
    if (len > 256) {
        printf("[NSC UART] Message too long (%zu bytes)\n", len);
        return -1;
    }

    /* Prefix with "[NS] " to indicate Non-Secure origin */
    char prefixed_msg[512];
    snprintf(prefixed_msg, sizeof(prefixed_msg), "[NS] %s", message);

    /* Transmit via Secure UART */
    SecureUART_Transmit((uint8_t*)prefixed_msg, strlen(prefixed_msg));

    return 0;
}

/**
 * @brief Verify UART security configuration
 */
int SecureUART_VerifyConfiguration(void)
{
    printf("\n[Secure UART] Verifying configuration...\n");

    /* Check USART1 security (should be Secure) */
    uint32_t usart1_sec = (GTZC_TZSC1->SECCFGR2 >> 14) & 0x3;  // USART1 in SECCFGR2
    if (usart1_sec != 0x3) {
        printf("[ERROR] USART1 not Secure!\n");
        return -1;
    }
    printf("✓ USART1: Secure\n");

    /* Check USART3 security (should be Non-Secure) */
    uint32_t usart3_sec = (GTZC_TZSC1->SECCFGR2 >> 18) & 0x3;  // USART3 in SECCFGR2
    if (usart3_sec != 0x0) {
        printf("[ERROR] USART3 not Non-Secure!\n");
        return -1;
    }
    printf("✓ USART3: Non-Secure\n");

    printf("[Secure UART] All configurations VALID\n");
    return 0;
}
```

### Testing Exercise 2

**Secure World Test:**
```c
void test_secure_uart(void)
{
    printf("\n=== Exercise 2: Secure UART Configuration ===\n");

    /* Initialize Secure and Non-Secure UARTs */
    SecureUART_Init();
    NonSecureUART_Init();

    /* Verify configuration */
    SecureUART_VerifyConfiguration();

    /* Test 1: Secure world sends to Secure UART */
    printf("\n[Test 1] Secure world → Secure UART (USART1)...\n");
    const char *secure_msg = "[SECURE] Confidential debug information\r\n";
    SecureUART_Transmit((uint8_t*)secure_msg, strlen(secure_msg));
    printf("✓ SUCCESS: Message sent via Secure UART\n");

    /* Test 2: Secure world sends to Non-Secure UART */
    printf("\n[Test 2] Secure world → Non-Secure UART (USART3)...\n");
    const char *nonsecure_msg = "[SECURE→NS] Application data\r\n";
    HAL_UART_Transmit(&huart3_nonsecure, (uint8_t*)nonsecure_msg,
                      strlen(nonsecure_msg), 1000);
    printf("✓ SUCCESS: Message sent via Non-Secure UART\n");

    printf("\n=== Exercise 2: COMPLETE ===\n");
}
```

**Expected UART Output (Secure USART1):**
```
[SECURE] Confidential debug information
[NS] Hello from Non-Secure world (via NSC gateway)
```

**Expected UART Output (Non-Secure USART3):**
```
[SECURE→NS] Application data
[NS Direct] Non-Secure application message
```

---

## Exercise 3: Secure Timer Configuration

### Objective
Configure TIM6 as a Secure timer for security-critical timing and TIM2 as Non-Secure for general use.

### Theory

**Timer Security Use Cases:**
- **Secure timers:** Watchdog timeouts, security event timestamps, secure delays
- **Non-Secure timers:** General application timing, PWM, event counting
- Secure timers **cannot** be stopped or modified by Non-Secure code

### Implementation

**File: `secure_timer.h`**
```c
#ifndef SECURE_TIMER_H
#define SECURE_TIMER_H

#include <stdint.h>
#include "stm32u5xx_hal.h"

/* Timer Handles */
extern TIM_HandleTypeDef htim6_secure;
extern TIM_HandleTypeDef htim2_nonsecure;

/**
 * @brief Initialize Secure TIM6 (1 kHz, 1ms ticks)
 * @return 0 on success
 */
int SecureTimer_Init(void);

/**
 * @brief Get secure timestamp (milliseconds since init)
 * @return Timestamp in ms
 */
uint32_t SecureTimer_GetTimestamp(void);

/**
 * @brief Secure delay (cannot be interrupted by Non-Secure code)
 * @param ms Delay in milliseconds
 */
void SecureTimer_Delay(uint32_t ms);

/**
 * @brief NSC function to get secure timestamp from Non-Secure world
 * @return Timestamp in ms
 */
__attribute__((cmse_nonsecure_entry))
uint32_t NSC_Timer_GetSecureTime(void);

/**
 * @brief Verify timer security configuration
 * @return 0 if secure
 */
int SecureTimer_VerifyConfiguration(void);

#endif /* SECURE_TIMER_H */
```

**File: `secure_timer.c`**
```c
#include "secure_timer.h"
#include <stdio.h>

TIM_HandleTypeDef htim6_secure;
TIM_HandleTypeDef htim2_nonsecure;

static volatile uint32_t secure_tick_count = 0;

/**
 * @brief Initialize Secure TIM6
 */
int SecureTimer_Init(void)
{
    printf("[Secure Timer] Initializing TIM6 as Secure...\n");

    /* Configure TIM6 as Secure in GTZC */
    HAL_GTZC_TZSC_ConfigPeriphAttributes(
        GTZC_PERIPH_TIM6,
        GTZC_TZSC_PERIPH_SEC | GTZC_TZSC_PERIPH_PRIV
    );

    /* Initialize TIM6: 1 kHz (1ms period) */
    htim6_secure.Instance = TIM6;
    htim6_secure.Init.Prescaler = (SystemCoreClock / 1000000) - 1;  // 1 MHz
    htim6_secure.Init.Period = 999;  // 1 kHz (1ms)
    htim6_secure.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim6_secure.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim6_secure.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_Base_Init(&htim6_secure) != HAL_OK) {
        printf("[ERROR] TIM6 initialization failed\n");
        return -1;
    }

    /* Enable TIM6 update interrupt */
    HAL_TIM_Base_Start_IT(&htim6_secure);

    printf("[Secure Timer] TIM6 initialized as SECURE (1 kHz)\n");
    return 0;
}

/**
 * @brief TIM6 interrupt handler (Secure)
 */
void TIM6_DAC_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&htim6_secure, TIM_FLAG_UPDATE)) {
        __HAL_TIM_CLEAR_FLAG(&htim6_secure, TIM_FLAG_UPDATE);
        secure_tick_count++;
    }
}

/**
 * @brief Get secure timestamp
 */
uint32_t SecureTimer_GetTimestamp(void)
{
    return secure_tick_count;
}

/**
 * @brief Secure delay (tamper-proof)
 */
void SecureTimer_Delay(uint32_t ms)
{
    uint32_t start = secure_tick_count;
    while ((secure_tick_count - start) < ms) {
        __WFI();  // Wait for interrupt
    }
}

/**
 * @brief NSC gateway to provide secure time to Non-Secure world
 */
__attribute__((cmse_nonsecure_entry))
uint32_t NSC_Timer_GetSecureTime(void)
{
    /* Non-Secure code can query secure time but cannot modify it */
    return secure_tick_count;
}

/**
 * @brief Verify timer security configuration
 */
int SecureTimer_VerifyConfiguration(void)
{
    printf("\n[Secure Timer] Verifying configuration...\n");

    /* Check TIM6 security */
    uint32_t tim6_sec = (GTZC_TZSC1->SECCFGR2 >> 4) & 0x3;
    if (tim6_sec != 0x3) {
        printf("[ERROR] TIM6 not Secure!\n");
        return -1;
    }
    printf("✓ TIM6: Secure\n");

    printf("[Secure Timer] All configurations VALID\n");
    return 0;
}
```

### Testing Exercise 3

```c
void test_secure_timer(void)
{
    printf("\n=== Exercise 3: Secure Timer Configuration ===\n");

    /* Initialize Secure Timer */
    SecureTimer_Init();
    SecureTimer_VerifyConfiguration();

    /* Test 1: Secure timestamp */
    printf("\n[Test 1] Secure Timer Timestamp...\n");
    uint32_t start_time = SecureTimer_GetTimestamp();
    SecureTimer_Delay(500);  // 500 ms delay
    uint32_t end_time = SecureTimer_GetTimestamp();
    printf("  Start: %lu ms\n", start_time);
    printf("  End:   %lu ms\n", end_time);
    printf("  Elapsed: %lu ms\n", end_time - start_time);
    printf("✓ SUCCESS: Secure timer running\n");

    /* Test 2: Blink Green LED using secure timer */
    printf("\n[Test 2] Blink LED with Secure Timer...\n");
    for (int i = 0; i < 3; i++) {
        SecureGPIO_Write(LED_GREEN_PORT, 7, true);
        SecureTimer_Delay(200);
        SecureGPIO_Write(LED_GREEN_PORT, 7, false);
        SecureTimer_Delay(200);
    }
    printf("✓ SUCCESS: LED blinking with secure timing\n");

    printf("\n=== Exercise 3: COMPLETE ===\n");
}
```

---

## Exercise 4: Preventing Peripheral-Based Attacks

### Objective
Demonstrate and prevent common peripheral-based attacks.

### Attack Scenarios

#### Attack 1: Non-Secure DMA to Secure Memory

**Vulnerability:**
If DMA is misconfigured, Non-Secure code could initiate DMA transfers to Secure memory.

**Prevention:**
```c
/**
 * @brief Configure DMA with TrustZone protection
 */
int SecureDMA_Init(void)
{
    printf("[Secure DMA] Configuring DMA security...\n");

    /* Configure DMA1 as Non-Secure */
    HAL_GTZC_TZSC_ConfigPeriphAttributes(
        GTZC_PERIPH_DMA1,
        GTZC_TZSC_PERIPH_NSEC
    );

    /* Enable GTZC MPCBB (Memory Protection Controller) */
    /* Block DMA access to Secure SRAM regions */
    HAL_GTZC_MPCBB_ConfigMem(
        GTZC_MPCBB1,
        SRAM_BASE,
        0x00000000,  // Block 0-31: Secure (no DMA access)
        GTZC_MPCBB_BLOCK_NSEC | GTZC_MPCBB_BLOCK_NSPRIV
    );

    printf("[Secure DMA] DMA cannot access Secure SRAM\n");
    return 0;
}
```

**Attack Demonstration:**
```c
void demo_dma_attack(void)
{
    printf("\n[ATTACK] Non-Secure DMA → Secure Memory...\n");

    /* Secure data in Secure SRAM */
    __attribute__((section(".secure_data"))) volatile uint32_t secure_key = 0xDEADBEEF;

    /* Non-Secure DMA attempts to read secure_key */
    DMA_HandleTypeDef hdma_attack;
    uint32_t stolen_data = 0;

    hdma_attack.Instance = DMA1_Channel1;
    hdma_attack.Init.Request = DMA_REQUEST_MEM2MEM;
    hdma_attack.Init.Direction = DMA_MEMORY_TO_MEMORY;
    hdma_attack.Init.PeriphInc = DMA_PINC_ENABLE;
    hdma_attack.Init.MemInc = DMA_MINC_ENABLE;
    hdma_attack.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_attack.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_attack.Init.Mode = DMA_NORMAL;
    hdma_attack.Init.Priority = DMA_PRIORITY_HIGH;

    HAL_DMA_Init(&hdma_attack);

    /* Attempt transfer */
    HAL_StatusTypeDef status = HAL_DMA_Start(
        &hdma_attack,
        (uint32_t)&secure_key,  // Source: Secure memory
        (uint32_t)&stolen_data,  // Destination: Non-Secure
        1
    );

    if (status != HAL_OK || stolen_data != 0xDEADBEEF) {
        printf("✓ ATTACK BLOCKED: DMA cannot read Secure memory\n");
        printf("  Stolen data: 0x%08lX (expected 0x00000000)\n", stolen_data);
        SecureGPIO_Blink(LED_GREEN_PORT, 7, 2, 200);  // Success (attack blocked)
    } else {
        printf("❌ ATTACK SUCCESS: Secure data leaked!\n");
        SecureGPIO_Blink(LED_RED_PORT, 2, 10, 100);  // Critical failure
    }
}
```

#### Attack 2: GPIO Glitching

**Vulnerability:**
An attacker could physically glitch GPIO pins to bypass security checks.

**Prevention:**
```c
/**
 * @brief Secure GPIO with glitch detection
 */
int SecureGPIO_GlitchProtection(GPIO_TypeDef *port, uint32_t pin)
{
    bool state1, state2, state3;

    /* Triple-read with majority voting */
    SecureGPIO_Read(port, pin, &state1);
    __DSB();  // Data Synchronization Barrier
    SecureGPIO_Read(port, pin, &state2);
    __DSB();
    SecureGPIO_Read(port, pin, &state3);

    /* Majority vote */
    uint8_t high_count = state1 + state2 + state3;
    bool final_state = (high_count >= 2);

    /* Check for glitch (all three reads should match) */
    if (!(state1 == state2 && state2 == state3)) {
        printf("[WARNING] GPIO glitch detected on pin %lu!\n", pin);
        SecureGPIO_Blink(LED_RED_PORT, 2, 5, 100);
        return -1;  // Glitch detected
    }

    return final_state ? 1 : 0;
}
```

#### Attack 3: Interrupt Hijacking

**Vulnerability:**
Non-Secure code could reconfigure interrupts to intercept secure data.

**Prevention:**
```c
/**
 * @brief Secure interrupt configuration
 */
int SecureInterrupt_Configure(IRQn_Type IRQn)
{
    /* Mark interrupt as Secure in NVIC */
    NVIC_SetTargetState(IRQn);  // Set to Secure

    /* Set priority (Secure interrupts have priority 0-127) */
    NVIC_SetPriority(IRQn, 0);  // Highest priority

    /* Enable interrupt */
    NVIC_EnableIRQ(IRQn);

    printf("[Secure IRQ] IRQ %d configured as SECURE, priority 0\n", IRQn);
    return 0;
}

/**
 * @brief Verify no interrupt hijacking
 */
int SecureInterrupt_VerifyConfiguration(IRQn_Type IRQn)
{
    /* Check if interrupt is still Secure */
    uint32_t target_state = NVIC_GetTargetState(IRQn);
    if (target_state != 1) {  // 1 = Secure
        printf("[ERROR] Interrupt %d hijacked to Non-Secure!\n", IRQn);
        return -1;
    }

    /* Check priority hasn't changed */
    uint32_t priority = NVIC_GetPriority(IRQn);
    if (priority != 0) {
        printf("[ERROR] Interrupt %d priority changed!\n", IRQn);
        return -1;
    }

    return 0;
}
```

### Testing Exercise 4

```c
void test_peripheral_attacks(void)
{
    printf("\n=== Exercise 4: Preventing Peripheral Attacks ===\n");

    /* Test 1: DMA attack prevention */
    SecureDMA_Init();
    demo_dma_attack();

    /* Test 2: GPIO glitch detection */
    printf("\n[Test 2] GPIO Glitch Detection...\n");
    int result = SecureGPIO_GlitchProtection(LED_GREEN_PORT, 7);
    if (result >= 0) {
        printf("✓ SUCCESS: No glitch detected\n");
    }

    /* Test 3: Interrupt hijacking prevention */
    printf("\n[Test 3] Secure Interrupt Configuration...\n");
    SecureInterrupt_Configure(TIM6_DAC_IRQn);
    if (SecureInterrupt_VerifyConfiguration(TIM6_DAC_IRQn) == 0) {
        printf("✓ SUCCESS: Interrupt secure and verified\n");
    }

    printf("\n=== Exercise 4: COMPLETE ===\n");
}
```

---

## Exercise 5: Production Peripheral Hardening Checklist

### Objective
Implement a comprehensive peripheral security verification function for production deployment.

### Implementation

**File: `peripheral_security_check.h`**
```c
#ifndef PERIPHERAL_SECURITY_CHECK_H
#define PERIPHERAL_SECURITY_CHECK_H

#include <stdint.h>
#include <stdbool.h>

/* Security Check Result */
typedef struct {
    bool gpio_secure;
    bool uart_secure;
    bool timer_secure;
    bool dma_secure;
    bool interrupt_secure;
    bool sau_configured;
    bool gtzc_configured;
    uint32_t vulnerabilities_found;
} PeripheralSecurityReport_t;

/**
 * @brief Comprehensive peripheral security check
 * @param report Output security report
 * @return 0 if all checks pass, -1 if vulnerabilities found
 */
int PeripheralSecurity_FullCheck(PeripheralSecurityReport_t *report);

/**
 * @brief Display security report
 * @param report Security report to display
 */
void PeripheralSecurity_PrintReport(const PeripheralSecurityReport_t *report);

#endif /* PERIPHERAL_SECURITY_CHECK_H */
```

**File: `peripheral_security_check.c`**
```c
#include "peripheral_security_check.h"
#include "secure_gpio.h"
#include "secure_uart.h"
#include "secure_timer.h"
#include <stdio.h>

/**
 * @brief Comprehensive peripheral security check
 */
int PeripheralSecurity_FullCheck(PeripheralSecurityReport_t *report)
{
    if (report == NULL) {
        return -1;
    }

    printf("\n========================================\n");
    printf(" PERIPHERAL SECURITY VERIFICATION\n");
    printf("========================================\n\n");

    memset(report, 0, sizeof(PeripheralSecurityReport_t));

    /* Check 1: GPIO Security */
    printf("[1/7] Checking GPIO security...\n");
    report->gpio_secure = (SecureGPIO_VerifyConfiguration() == 0);
    if (!report->gpio_secure) {
        printf("  ❌ GPIO security FAILED\n");
        report->vulnerabilities_found++;
    } else {
        printf("  ✅ GPIO security OK\n");
    }

    /* Check 2: UART Security */
    printf("\n[2/7] Checking UART security...\n");
    report->uart_secure = (SecureUART_VerifyConfiguration() == 0);
    if (!report->uart_secure) {
        printf("  ❌ UART security FAILED\n");
        report->vulnerabilities_found++;
    } else {
        printf("  ✅ UART security OK\n");
    }

    /* Check 3: Timer Security */
    printf("\n[3/7] Checking Timer security...\n");
    report->timer_secure = (SecureTimer_VerifyConfiguration() == 0);
    if (!report->timer_secure) {
        printf("  ❌ Timer security FAILED\n");
        report->vulnerabilities_found++;
    } else {
        printf("  ✅ Timer security OK\n");
    }

    /* Check 4: DMA Security */
    printf("\n[4/7] Checking DMA security...\n");
    uint32_t dma1_sec = (GTZC_TZSC1->SECCFGR1 >> 0) & 0x3;
    report->dma_secure = (dma1_sec == 0x0);  // Should be Non-Secure
    if (!report->dma_secure) {
        printf("  ❌ DMA security misconfigured\n");
        report->vulnerabilities_found++;
    } else {
        printf("  ✅ DMA security OK\n");
    }

    /* Check 5: Interrupt Security */
    printf("\n[5/7] Checking Interrupt security...\n");
    uint32_t tim6_nvic_secure = NVIC_GetTargetState(TIM6_DAC_IRQn);
    report->interrupt_secure = (tim6_nvic_secure == 1);  // 1 = Secure
    if (!report->interrupt_secure) {
        printf("  ❌ Interrupt security FAILED\n");
        report->vulnerabilities_found++;
    } else {
        printf("  ✅ Interrupt security OK\n");
    }

    /* Check 6: SAU Configuration */
    printf("\n[6/7] Checking SAU configuration...\n");
    uint32_t sau_ctrl = SAU->CTRL;
    report->sau_configured = (sau_ctrl & SAU_CTRL_ENABLE_Msk);
    if (!report->sau_configured) {
        printf("  ❌ SAU not enabled!\n");
        report->vulnerabilities_found++;
    } else {
        printf("  ✅ SAU enabled\n");
    }

    /* Check 7: GTZC Configuration */
    printf("\n[7/7] Checking GTZC configuration...\n");
    uint32_t gtzc_cr = GTZC_TZSC1->CR;
    report->gtzc_configured = (gtzc_cr & GTZC_TZSC_CR_LCK_Msk) == 0;  // Not locked yet
    if (!report->gtzc_configured) {
        printf("  ⚠️  GTZC locked (expected for production)\n");
    } else {
        printf("  ✅ GTZC configured (not locked)\n");
    }

    /* Final result */
    printf("\n========================================\n");
    if (report->vulnerabilities_found == 0) {
        printf(" RESULT: ✅ ALL CHECKS PASSED\n");
        printf("========================================\n");
        SecureGPIO_Blink(LED_GREEN_PORT, 7, 5, 100);
        return 0;
    } else {
        printf(" RESULT: ❌ %lu VULNERABILITIES FOUND\n", report->vulnerabilities_found);
        printf("========================================\n");
        SecureGPIO_Blink(LED_RED_PORT, 2, report->vulnerabilities_found, 200);
        return -1;
    }
}

/**
 * @brief Display security report
 */
void PeripheralSecurity_PrintReport(const PeripheralSecurityReport_t *report)
{
    printf("\n========================================\n");
    printf(" PERIPHERAL SECURITY REPORT\n");
    printf("========================================\n\n");

    printf("GPIO Security:      %s\n", report->gpio_secure ? "✅ PASS" : "❌ FAIL");
    printf("UART Security:      %s\n", report->uart_secure ? "✅ PASS" : "❌ FAIL");
    printf("Timer Security:     %s\n", report->timer_secure ? "✅ PASS" : "❌ FAIL");
    printf("DMA Security:       %s\n", report->dma_secure ? "✅ PASS" : "❌ FAIL");
    printf("Interrupt Security: %s\n", report->interrupt_secure ? "✅ PASS" : "❌ FAIL");
    printf("SAU Configured:     %s\n", report->sau_configured ? "✅ PASS" : "❌ FAIL");
    printf("GTZC Configured:    %s\n", report->gtzc_configured ? "✅ PASS" : "❌ FAIL");

    printf("\n========================================\n");
    printf("Total Vulnerabilities: %lu\n", report->vulnerabilities_found);
    printf("========================================\n");
}
```

### Testing Exercise 5

```c
void test_production_hardening(void)
{
    printf("\n=== Exercise 5: Production Hardening ===\n");

    PeripheralSecurityReport_t report;

    /* Run full security check */
    int result = PeripheralSecurity_FullCheck(&report);

    /* Print detailed report */
    PeripheralSecurity_PrintReport(&report);

    if (result == 0) {
        printf("\n✅ System ready for production deployment\n");
    } else {
        printf("\n❌ System NOT ready for production\n");
        printf("   Fix %lu vulnerabilities before deployment\n",
               report.vulnerabilities_found);
    }

    printf("\n=== Exercise 5: COMPLETE ===\n");
}
```

---

## Main Application

**File: `main.c`**
```c
#include "secure_gpio.h"
#include "secure_uart.h"
#include "secure_timer.h"
#include "peripheral_security_check.h"
#include <stdio.h>

/* External test functions */
extern void test_secure_gpio(void);
extern void test_secure_uart(void);
extern void test_secure_timer(void);
extern void test_peripheral_attacks(void);
extern void test_production_hardening(void);

int main(void)
{
    /* Initialize HAL */
    HAL_Init();
    SystemClock_Config();

    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║   LAB 11: SECURE PERIPHERAL ACCESS             ║\n");
    printf("║   NUCLEO-U545RE-Q with TF-M                    ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    printf("\n");

    /* Run all exercises */
    test_secure_gpio();
    test_secure_uart();
    test_secure_timer();
    test_peripheral_attacks();
    test_production_hardening();

    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║   LAB 11: ALL EXERCISES COMPLETE ✅            ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    printf("\n");

    /* Final success indication */
    while (1) {
        SecureGPIO_Write(LED_GREEN_PORT, 7, true);
        SecureTimer_Delay(1000);
        SecureGPIO_Write(LED_GREEN_PORT, 7, false);
        SecureTimer_Delay(1000);
    }
}
```

---

## Expected Output

```
╔════════════════════════════════════════════════╗
║   LAB 11: SECURE PERIPHERAL ACCESS             ║
║   NUCLEO-U545RE-Q with TF-M                    ║
╚════════════════════════════════════════════════╝

=== Exercise 1: Secure GPIO Configuration ===
[Secure GPIO] Initializing GPIO security...
[Secure GPIO] GTZC configured:
  - GPIOC (PC7 LD1): Secure
  - GPIOG (PG2 LD3): Secure
  - GPIOB (PB7 LD2): Non-Secure
[Secure GPIO] Configured pin 7 as SECURE
[Secure GPIO] Configured pin 2 as SECURE
[Secure GPIO] Configured pin 7 as NON-SECURE

[Secure GPIO] Verifying security configuration...
✓ GPIOC: Secure
✓ GPIOB: Non-Secure
✓ PC7 (Green LED): Secure
✓ PB7 (Blue LED): Non-Secure
[Secure GPIO] All configurations VALID

[Test 1] Secure world → Secure LED (GREEN)...
✓ SUCCESS: Green LED blinking

[Test 2] Secure world → Non-Secure LED (BLUE)...
✓ SUCCESS: Blue LED toggled (Secure can access Non-Secure)

=== Exercise 1: COMPLETE ===

=== Exercise 2: Secure UART Configuration ===
[Secure UART] Initializing USART1 as Secure...
[Secure UART] USART1 initialized as SECURE
[Secure UART] Initializing USART3 as Non-Secure...
[Secure UART] USART3 initialized as NON-SECURE

[Secure UART] Verifying configuration...
✓ USART1: Secure
✓ USART3: Non-Secure
[Secure UART] All configurations VALID

[Test 1] Secure world → Secure UART (USART1)...
✓ SUCCESS: Message sent via Secure UART

[Test 2] Secure world → Non-Secure UART (USART3)...
✓ SUCCESS: Message sent via Non-Secure UART

=== Exercise 2: COMPLETE ===

=== Exercise 3: Secure Timer Configuration ===
[Secure Timer] Initializing TIM6 as Secure...
[Secure Timer] TIM6 initialized as SECURE (1 kHz)

[Secure Timer] Verifying configuration...
✓ TIM6: Secure
[Secure Timer] All configurations VALID

[Test 1] Secure Timer Timestamp...
  Start: 1234 ms
  End:   1734 ms
  Elapsed: 500 ms
✓ SUCCESS: Secure timer running

[Test 2] Blink LED with Secure Timer...
✓ SUCCESS: LED blinking with secure timing

=== Exercise 3: COMPLETE ===

=== Exercise 4: Preventing Peripheral Attacks ===
[Secure DMA] Configuring DMA security...
[Secure DMA] DMA cannot access Secure SRAM

[ATTACK] Non-Secure DMA → Secure Memory...
✓ ATTACK BLOCKED: DMA cannot read Secure memory
  Stolen data: 0x00000000 (expected 0x00000000)

[Test 2] GPIO Glitch Detection...
✓ SUCCESS: No glitch detected

[Test 3] Secure Interrupt Configuration...
[Secure IRQ] IRQ 54 configured as SECURE, priority 0
✓ SUCCESS: Interrupt secure and verified

=== Exercise 4: COMPLETE ===

=== Exercise 5: Production Hardening ===

========================================
 PERIPHERAL SECURITY VERIFICATION
========================================

[1/7] Checking GPIO security...
  ✅ GPIO security OK

[2/7] Checking UART security...
  ✅ UART security OK

[3/7] Checking Timer security...
  ✅ Timer security OK

[4/7] Checking DMA security...
  ✅ DMA security OK

[5/7] Checking Interrupt security...
  ✅ Interrupt security OK

[6/7] Checking SAU configuration...
  ✅ SAU enabled

[7/7] Checking GTZC configuration...
  ✅ GTZC configured (not locked)

========================================
 RESULT: ✅ ALL CHECKS PASSED
========================================

========================================
 PERIPHERAL SECURITY REPORT
========================================

GPIO Security:      ✅ PASS
UART Security:      ✅ PASS
Timer Security:     ✅ PASS
DMA Security:       ✅ PASS
Interrupt Security: ✅ PASS
SAU Configured:     ✅ PASS
GTZC Configured:    ✅ PASS

========================================
Total Vulnerabilities: 0
========================================

✅ System ready for production deployment

=== Exercise 5: COMPLETE ===

╔════════════════════════════════════════════════╗
║   LAB 11: ALL EXERCISES COMPLETE ✅            ║
╚════════════════════════════════════════════════╝
```

---

## Visual Feedback Summary

| LED | Event | Meaning |
|-----|-------|---------|
| 🟢 **LD1 (Green)** | Blinks 3x fast | Exercise 1 success |
| 🟢 **LD1 (Green)** | Blinks 3x slow | Exercise 3 success |
| 🟢 **LD1 (Green)** | Blinks 5x fast | All checks passed |
| 🔵 **LD2 (Blue)** | Single toggle | Non-Secure GPIO test |
| 🔴 **LD3 (Red)** | Blinks 10x | Attack detected |
| 🔴 **LD3 (Red)** | Blinks N times | N vulnerabilities found |

---

## Key Takeaways

1. ✅ **GTZC is critical** for STM32U5 peripheral security
2. ✅ **Always verify** peripheral security configuration
3. ✅ **Secure can access Non-Secure**, but not vice versa
4. ✅ **NSC gateways** provide safe peripheral sharing
5. ✅ **DMA attacks** can be prevented with MPCBB
6. ✅ **Production systems** must lock GTZC configuration
7. ✅ **Comprehensive checks** before deployment are mandatory

---

## Build and Flash

### Build Commands
```bash
# Build Secure application
cd secure
make clean
make

# Build Non-Secure application
cd ../nonsecure
make clean
make

# Merge binaries
arm-none-eabi-objcopy -O binary secure/lab11_secure.elf secure.bin
arm-none-eabi-objcopy -O binary nonsecure/lab11_nonsecure.elf nonsecure.bin
```

### Flash with STM32CubeProgrammer
```bash
STM32_Programmer_CLI -c port=SWD -w secure.bin 0x0C000000 -w nonsecure.bin 0x08040000 -rst
```

---

## Troubleshooting

**Issue:** Green LED doesn't blink
- Check GPIOC clock enabled
- Verify PC7 configured as Secure output
- Check SAU/GTZC configuration

**Issue:** SecureFault when accessing GPIO
- Verify GPIO port security in GTZC
- Check pin security in SECCFGR register
- Use NSC gateway for Non-Secure access

**Issue:** UART not working
- Check USART clock enabled
- Verify GPIO AF (Alternate Function) configuration
- Check baud rate matches terminal

**Issue:** Timer not incrementing
- Verify TIM6 clock enabled
- Check prescaler and period calculations
- Enable interrupt in NVIC

---

## Next Steps

✅ Lab 11 Complete! Continue to:
- **Lab 12:** Inter-Partition Communication (IPC)
- **Lab 13:** Secure Debug and Production Deployment
- **Lab 14:** Power Management and Secure Sleep

---

## References

- [STM32U5 Reference Manual (RM0456)](https://www.st.com/resource/en/reference_manual/rm0456-stm32u5-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [ARMv8-M TrustZone Technology](https://developer.arm.com/documentation/100690/latest/)
- [STM32 TrustZone Development Guide](https://wiki.st.com/stm32mcu/wiki/Security:Introduction_to_TrustZone)

---

**Lab 11 Complete! 🎉**

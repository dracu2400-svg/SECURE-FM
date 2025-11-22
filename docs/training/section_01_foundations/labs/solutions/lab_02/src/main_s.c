/**
 ******************************************************************************
 * @file    main_s.c
 * @brief   Secure World Application for Lab 02 - TrustZone Basics
 * @details This file implements the Secure side of the TrustZone demonstration.
 *          It provides secure services to the Non-Secure world through NSC
 *          (Non-Secure Callable) functions.
 *
 * Hardware: NUCLEO-U545RE-Q
 * LED Used: LD1 (Green LED on PA5) - Indicates Secure world execution
 *
 * Key Concepts Demonstrated:
 * - Secure world initialization
 * - Secure LED control (not accessible from Non-Secure)
 * - Secure memory access
 * - NSC veneer functions for secure services
 * - Context switching (Secure → Non-Secure)
 ******************************************************************************
 */

#include "stm32u5xx_hal.h"
#include "partition_stm32u545xx.h"
#include <stdio.h>
#include <string.h>
#include <arm_cmse.h>

/* ============================================================================
 * LED Configuration - Secure World
 * ============================================================================
 * LD1 (Green LED) - Shows Secure world execution
 */
#define LED_SECURE_GPIO_Port    GPIOA        /* LD1 - Green LED */
#define LED_SECURE_Pin          GPIO_PIN_5

/* ============================================================================
 * Secure Memory Regions
 * ============================================================================
 * These addresses match the SAU configuration for Secure-only memory
 */
#define SECURE_SRAM_BASE        0x30000000   /* Secure SRAM1 base */
#define SECURE_SRAM_SIZE        (64 * 1024)  /* 64KB reserved for Secure */
#define SECURE_DATA_ADDR        (SECURE_SRAM_BASE + 0x1000)

/* ============================================================================
 * Secure Data Storage
 * ============================================================================
 */
typedef struct {
    uint32_t magic;          /* Magic number for integrity check */
    uint32_t counter;        /* Secure counter */
    uint8_t  secret_key[32]; /* Secret encryption key */
    uint32_t checksum;       /* Data integrity checksum */
} SecureData_t;

/* Secure data stored in Secure SRAM */
__attribute__((section(".secure_data")))
static SecureData_t g_secure_data = {
    .magic = 0x5EC00000,
    .counter = 0,
    .secret_key = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
    },
    .checksum = 0
};

/* ============================================================================
 * HAL Handles
 * ============================================================================
 */
static UART_HandleTypeDef huart1;

/* ============================================================================
 * Private Function Prototypes
 * ============================================================================
 */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void LED_Secure_Init(void);
static void LED_Secure_On(void);
static void LED_Secure_Off(void);
static uint32_t calculate_checksum(const uint8_t *data, size_t len);
static bool verify_secure_data_integrity(void);

/* ============================================================================
 * NSC Function Prototypes (Exported to Non-Secure)
 * ============================================================================
 * These functions are defined in nsc_functions.c with __attribute__((cmse_nonsecure_entry))
 */
extern void __attribute__((cmse_nonsecure_entry)) secure_led_blink(uint32_t count);
extern uint32_t __attribute__((cmse_nonsecure_entry)) secure_read_counter(void);
extern int32_t __attribute__((cmse_nonsecure_entry)) secure_hash_data(const uint8_t *data, size_t len, uint8_t *hash);
extern void __attribute__((cmse_nonsecure_entry)) secure_increment_counter(void);

/* ============================================================================
 * Main Function - Secure World Entry Point
 * ============================================================================
 */
int main(void)
{
    /* Reset of all peripherals, Initializes the Flash interface and the Systick */
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Config();

    /* Initialize GPIOs */
    GPIO_Init();

    /* Initialize UART for debug output */
    UART1_Init();

    /* Initialize Secure LED (LD1 - Green) */
    LED_Secure_Init();

    /* Print Secure World banner */
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  🔒 SECURE WORLD INITIALIZED - Lab 02 TrustZone Basics\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("Hardware: NUCLEO-U545RE-Q (STM32U545RET6Q)\n");
    printf("Core:     ARM Cortex-M33 with TrustZone\n");
    printf("Security: Secure World Active\n");
    printf("─────────────────────────────────────────────────────────────\n\n");

    /* Verify Secure data integrity */
    if (verify_secure_data_integrity()) {
        printf("[SECURE] ✓ Secure data integrity verified\n");
    } else {
        printf("[SECURE] ⚠️  Secure data corrupted! Initializing...\n");
        g_secure_data.magic = 0x5EC00000;
        g_secure_data.counter = 0;
        g_secure_data.checksum = calculate_checksum((uint8_t*)&g_secure_data,
                                                     sizeof(g_secure_data) - 4);
    }

    /* Display secure memory layout */
    printf("[SECURE] Memory Configuration:\n");
    printf("  Secure SRAM:   0x%08X - 0x%08X (%d KB)\n",
           SECURE_SRAM_BASE,
           SECURE_SRAM_BASE + SECURE_SRAM_SIZE - 1,
           SECURE_SRAM_SIZE / 1024);
    printf("  Secure Data:   0x%08X\n", (uint32_t)&g_secure_data);
    printf("  Secret Key:    [REDACTED - 32 bytes]\n");
    printf("  Counter Value: %lu\n\n", g_secure_data.counter);

    /* Signal Secure world is running */
    printf("[SECURE] Blinking Green LED (LD1) 5 times to indicate Secure world...\n");
    for (int i = 0; i < 5; i++) {
        LED_Secure_On();
        HAL_Delay(200);
        LED_Secure_Off();
        HAL_Delay(200);
    }
    printf("[SECURE] LED sequence complete\n\n");

    /* Configure SAU (Security Attribution Unit) */
    printf("[SECURE] Configuring Security Attribution Unit (SAU)...\n");
    printf("  SAU Region 0: 0x0C03E000-0x0C040000 (NSC - Non-Secure Callable)\n");
    printf("  SAU Region 1: 0x08040000-0x08080000 (Non-Secure Flash)\n");
    printf("  SAU Region 2: 0x20000000-0x20040000 (Non-Secure SRAM)\n");
    printf("  SAU Region 3: 0x40000000-0x4FFFFFFF (Non-Secure Peripherals)\n");
    printf("  SAU_CTRL:     0x%08lX\n", SAU->CTRL);
    printf("[SECURE] ✓ SAU Configuration Complete\n\n");

    /* Jump to Non-Secure World */
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  🔓 Jumping to Non-Secure World...\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    printf(">>> Context Switch: S → NS <<<\n\n");

    /* Set Non-Secure main stack pointer (MSP_NS) */
    uint32_t *ns_vector_table = (uint32_t *)0x08040000;  /* Non-Secure Flash base */
    uint32_t ns_msp = ns_vector_table[0];
    uint32_t ns_reset_handler = ns_vector_table[1];

    __TZ_set_MSP_NS(ns_msp);

    /* Create function pointer to Non-Secure Reset_Handler */
    typedef void (*NonSecure_ResetHandler)(void) __attribute__((cmse_nonsecure_call));
    NonSecure_ResetHandler ns_reset = (NonSecure_ResetHandler)(ns_reset_handler);

    /* Jump to Non-Secure world */
    ns_reset();

    /* Should never reach here */
    while (1) {
        LED_Secure_On();
        HAL_Delay(100);
        LED_Secure_Off();
        HAL_Delay(100);
    }
}

/* ============================================================================
 * Secure LED Functions (Internal - Not exported to NS)
 * ============================================================================
 */

/**
 * @brief Initialize the Secure LED (LD1 - Green)
 */
static void LED_Secure_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable GPIOA clock */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* Configure PA5 (LD1 - Green LED) as output */
    GPIO_InitStruct.Pin = LED_SECURE_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_SECURE_GPIO_Port, &GPIO_InitStruct);

    /* Set LED off initially */
    HAL_GPIO_WritePin(LED_SECURE_GPIO_Port, LED_SECURE_Pin, GPIO_PIN_RESET);
}

/**
 * @brief Turn on the Secure LED (LD1 - Green)
 */
static void LED_Secure_On(void)
{
    HAL_GPIO_WritePin(LED_SECURE_GPIO_Port, LED_SECURE_Pin, GPIO_PIN_SET);
}

/**
 * @brief Turn off the Secure LED (LD1 - Green)
 */
static void LED_Secure_Off(void)
{
    HAL_GPIO_WritePin(LED_SECURE_GPIO_Port, LED_SECURE_Pin, GPIO_PIN_RESET);
}

/* ============================================================================
 * Public Secure Functions (Called by NSC veneers)
 * ============================================================================
 */

/**
 * @brief Blink the Secure LED a specified number of times
 * @note This function is called by the NSC veneer secure_led_blink()
 */
void secure_led_blink_impl(uint32_t count)
{
    printf("[SECURE] Blinking Green LED (LD1) %lu times\n", count);

    for (uint32_t i = 0; i < count; i++) {
        LED_Secure_On();
        HAL_Delay(150);
        LED_Secure_Off();
        HAL_Delay(150);
    }

    printf("[SECURE] LED blink sequence complete\n");
}

/**
 * @brief Read the secure counter value
 * @return Current counter value
 */
uint32_t secure_read_counter_impl(void)
{
    printf("[SECURE] Reading secure counter: %lu\n", g_secure_data.counter);
    return g_secure_data.counter;
}

/**
 * @brief Increment the secure counter
 */
void secure_increment_counter_impl(void)
{
    g_secure_data.counter++;

    /* Update checksum */
    g_secure_data.checksum = calculate_checksum((uint8_t*)&g_secure_data,
                                                 sizeof(g_secure_data) - 4);

    printf("[SECURE] Counter incremented to: %lu\n", g_secure_data.counter);
}

/**
 * @brief Compute a secure hash of data using the secret key
 * @param data Pointer to data to hash (must be Non-Secure addressable)
 * @param len Length of data
 * @param hash Output buffer for hash (must be Non-Secure addressable)
 * @return 0 on success, -1 on error
 */
int32_t secure_hash_data_impl(const uint8_t *data, size_t len, uint8_t *hash)
{
    /* Verify pointers are in Non-Secure memory using CMSE intrinsics */
    if (cmse_check_address_range((void*)data, len, CMSE_NONSECURE) == NULL) {
        printf("[SECURE] ⚠️  Error: Data pointer not in Non-Secure memory!\n");
        return -1;
    }

    if (cmse_check_address_range((void*)hash, 32, CMSE_NONSECURE | CMSE_MPU_READWRITE) == NULL) {
        printf("[SECURE] ⚠️  Error: Hash buffer not in Non-Secure memory!\n");
        return -1;
    }

    printf("[SECURE] Computing secure hash of %zu bytes...\n", len);

    /* Simple HMAC-like hash (for demonstration purposes) */
    /* In production, use hardware crypto (AES, SHA256, etc.) */
    uint32_t hash_state[8] = {0};

    for (size_t i = 0; i < len; i++) {
        hash_state[i % 8] ^= data[i];
        hash_state[i % 8] = (hash_state[i % 8] << 5) | (hash_state[i % 8] >> 27);
    }

    /* Mix with secret key */
    for (size_t i = 0; i < 32; i++) {
        hash_state[i % 8] ^= g_secure_data.secret_key[i];
    }

    /* Copy result to output */
    memcpy(hash, hash_state, 32);

    printf("[SECURE] ✓ Hash computed successfully\n");
    printf("[SECURE] Hash: ");
    for (int i = 0; i < 32; i++) {
        printf("%02X", hash[i]);
    }
    printf("\n");

    return 0;
}

/* ============================================================================
 * Helper Functions
 * ============================================================================
 */

/**
 * @brief Calculate simple checksum
 */
static uint32_t calculate_checksum(const uint8_t *data, size_t len)
{
    uint32_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum;
}

/**
 * @brief Verify secure data integrity
 */
static bool verify_secure_data_integrity(void)
{
    if (g_secure_data.magic != 0x5EC00000) {
        return false;
    }

    uint32_t expected = calculate_checksum((uint8_t*)&g_secure_data,
                                            sizeof(g_secure_data) - 4);
    return (g_secure_data.checksum == expected);
}

/* ============================================================================
 * System Configuration Functions
 * ============================================================================
 */

/**
 * @brief System Clock Configuration
 * @details Configure system clock to run at maximum frequency (160 MHz)
 */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Configure the main internal regulator output voltage */
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
        Error_Handler();
    }

    /* Initializes the CPU, AHB and APB buses clocks */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_4;  /* 4 MHz */
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 80;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /* Initializes the CPU, AHB and APB buses clocks */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
                                | RCC_CLOCKTYPE_PCLK3;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief GPIO Initialization Function
 */
static void GPIO_Init(void)
{
    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
}

/**
 * @brief UART1 Initialization Function
 * @details Configure UART1 for debug output (115200 baud, 8N1)
 */
static void UART1_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief Redirect printf to UART
 */
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/**
 * @brief Error Handler
 */
void Error_Handler(void)
{
    __disable_irq();
    while (1) {
        /* Blink LED rapidly to indicate error */
        LED_Secure_On();
        for (volatile int i = 0; i < 100000; i++);
        LED_Secure_Off();
        for (volatile int i = 0; i < 100000; i++);
    }
}

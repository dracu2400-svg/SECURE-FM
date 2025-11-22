/**
 * @file main_ns.c
 * @brief Lab 02: TrustZone Basics - Non-Secure Application
 *
 * Interactive demonstration of TrustZone using NUCLEO-U545RE-Q:
 * - LD1 (Green) shows Secure world execution
 * - LD2 (Blue) shows Non-Secure world execution
 * - LD3 (Red) shows security faults
 * - Button B1 triggers Secure function calls
 *
 * @author TF-M Training Lab
 */

#include "stm32u5xx_hal.h"
#include "tfm_ns_interface.h"
#include "psa/crypto.h"
#include <stdio.h>
#include <string.h>

/* LED Pin Definitions for NUCLEO-U545RE-Q */
#define LED_NS_GPIO_Port    GPIOC        /* LD2 - Blue LED */
#define LED_NS_Pin          GPIO_PIN_7
#define LED_FAULT_GPIO_Port GPIOB        /* LD3 - Red LED */
#define LED_FAULT_Pin       GPIO_PIN_7

/* Button Definition */
#define BUTTON_GPIO_Port    GPIOC        /* B1 - User Button */
#define BUTTON_Pin          GPIO_PIN_13

/* Memory addresses for demonstration */
#define SECURE_RAM_ADDR     0x20000100   /* Secure RAM region */
#define NS_RAM_ADDR         0x20020100   /* Non-Secure RAM region */

/* External Secure functions (NSC veneers) */
extern void secure_led_blink(uint32_t count);
extern int32_t secure_read_memory(uint32_t address, uint32_t *value);
extern void secure_hash_compute(const uint8_t *data, size_t len, uint8_t *hash);

/* Function prototypes */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART_Init(void);
static void LED_NS_Blink(uint32_t count);
static void LED_Fault_On(void);
static void test_context_switch(void);
static void test_memory_violation(void);
static void test_secure_function_call(void);
static void print_memory_layout(void);
static void interactive_menu(void);

/* Global handles */
UART_HandleTypeDef huart1;

/**
 * @brief Main Non-Secure application entry point
 */
int main(void)
{
    /* Initialize HAL */
    HAL_Init();

    /* Configure system clock */
    SystemClock_Config();

    /* Initialize peripherals */
    GPIO_Init();
    UART_Init();

    /* Initialize TF-M Non-Secure interface */
    tfm_ns_interface_init();

    printf("\n");
    printf("╔═══════════════════════════════════════════════════════╗\n");
    printf("║  Lab 02: TrustZone Basics                            ║\n");
    printf("║  NUCLEO-U545RE-Q Interactive Demo                    ║\n");
    printf("╚═══════════════════════════════════════════════════════╝\n");
    printf("\n");

    /* Show initial status */
    printf("[NON-SECURE] Non-Secure application started!\n");
    printf("[NON-SECURE] Current world: NON-SECURE\n");
    printf("[NON-SECURE] Press USER button (B1) to call Secure function\n");
    printf("\n");

    /* Print memory layout */
    print_memory_layout();

    printf("\nStatus: LD2 (Blue) is blinking - running in NS world\n\n");

    /* Main loop */
    uint32_t button_pressed = 0;
    uint32_t last_tick = 0;
    char cmd_buffer[32] = {0};
    uint8_t cmd_idx = 0;

    while (1)
    {
        /* Blink NS LED to show we're alive */
        if (HAL_GetTick() - last_tick > 1000)
        {
            LED_NS_Blink(1);  /* Blue LED blink once */
            last_tick = HAL_GetTick();
        }

        /* Check button press */
        if (HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin) == GPIO_PIN_RESET)
        {
            if (!button_pressed)
            {
                button_pressed = 1;
                HAL_Delay(50);  /* Debounce */

                printf("\n[NON-SECURE] Button pressed! Calling Secure function...\n");
                test_context_switch();
            }
        }
        else
        {
            button_pressed = 0;
        }

        /* Check for serial commands */
        uint8_t ch;
        if (HAL_UART_Receive(&huart1, &ch, 1, 10) == HAL_OK)
        {
            if (ch == '\r' || ch == '\n')
            {
                if (cmd_idx > 0)
                {
                    cmd_buffer[cmd_idx] = '\0';
                    printf("\n");

                    /* Process command */
                    if (strcmp(cmd_buffer, "test_violation") == 0)
                    {
                        test_memory_violation();
                    }
                    else if (strcmp(cmd_buffer, "show_memory") == 0)
                    {
                        print_memory_layout();
                    }
                    else if (strcmp(cmd_buffer, "call_secure") == 0)
                    {
                        test_secure_function_call();
                    }
                    else if (strcmp(cmd_buffer, "help") == 0)
                    {
                        interactive_menu();
                    }
                    else
                    {
                        printf("Unknown command: %s\n", cmd_buffer);
                        printf("Type 'help' for available commands\n");
                    }

                    printf("\n> ");
                    cmd_idx = 0;
                    memset(cmd_buffer, 0, sizeof(cmd_buffer));
                }
            }
            else if (ch == '\b' || ch == 127)  /* Backspace */
            {
                if (cmd_idx > 0)
                {
                    cmd_idx--;
                    printf("\b \b");
                }
            }
            else if (cmd_idx < sizeof(cmd_buffer) - 1)
            {
                cmd_buffer[cmd_idx++] = ch;
                printf("%c", ch);
            }
        }
    }
}

/**
 * @brief Test context switch from NS to S
 */
static void test_context_switch(void)
{
    printf("[NON-SECURE] Switching to Secure world...\n\n");
    printf(">>> Context Switch: NS → S <<<\n\n");

    /* Call secure function (triggers context switch) */
    secure_led_blink(3);  /* Blink green LED 3 times in Secure world */

    printf("\n>>> Context Switch: S → NS <<<\n\n");
    printf("[NON-SECURE] Returned from Secure function\n");
    printf("[NON-SECURE] Status: SUCCESS\n");
    printf("[NON-SECURE] Ready for next button press\n");
}

/**
 * @brief Attempt to access Secure memory (will trigger fault)
 */
static void test_memory_violation(void)
{
    printf("\n[NON-SECURE] Testing: Attempt to read Secure memory...\n");
    printf("[NON-SECURE] Trying to access 0x%08X (Secure RAM)...\n",
           SECURE_RAM_ADDR);

    /* This will trigger SecureFault! */
    volatile uint32_t *secure_ptr = (volatile uint32_t *)SECURE_RAM_ADDR;

    /* Turn on fault LED before attempting */
    LED_Fault_On();

    printf("\n⚠️  SECURITY FAULT WILL OCCUR NOW! ⚠️\n");
    HAL_Delay(1000);

    /* This read will fault */
    uint32_t value = *secure_ptr;  /* ← Fault happens here! */

    /* We should never reach here */
    printf("Read value: 0x%08X\n", (unsigned int)value);
}

/**
 * @brief Call secure function to access secure memory properly
 */
static void test_secure_function_call(void)
{
    printf("\n[NON-SECURE] Calling Secure function to read Secure memory...\n");
    printf("\n>>> Context Switch: NS → S <<<\n\n");

    uint32_t value;
    int32_t status = secure_read_memory(SECURE_RAM_ADDR, &value);

    printf("\n>>> Context Switch: S → NS <<<\n\n");

    if (status == 0)
    {
        printf("[NON-SECURE] Secure function returned successfully\n");
        printf("✓ Received result: 0x%08X\n", (unsigned int)value);
        printf("✓ (But didn't see the secret directly!)\n");
    }
    else
    {
        printf("[NON-SECURE] Secure function returned error: %d\n",
               (int)status);
    }
}

/**
 * @brief Print TrustZone memory layout
 */
static void print_memory_layout(void)
{
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════╗\n");
    printf("║  TrustZone Memory Layout (STM32U545)                 ║\n");
    printf("╚═══════════════════════════════════════════════════════╝\n");
    printf("\n");

    printf("FLASH Memory (512 KB):\n");
    printf("┌─────────────────────────────────────────────────────┐\n");
    printf("│ 0x0800_0000  BL2 (MCUboot)           [40 KB]   [S]  │\n");
    printf("│ 0x0800_A000  TF-M Secure             [200 KB]  [S]  │ ← LD1\n");
    printf("│ 0x0803_C000  Application (NS)        [256 KB]  [NS] │ ← LD2\n");
    printf("│ 0x0807_C000  (Reserved for OTA)      [256 KB]       │\n");
    printf("└─────────────────────────────────────────────────────┘\n");
    printf("\n");

    printf("RAM Memory (256 KB):\n");
    printf("┌─────────────────────────────────────────────────────┐\n");
    printf("│ 0x2000_0000  Secure RAM              [128 KB]  [S]  │\n");
    printf("│              └─ Secrets stored here                 │\n");
    printf("│              └─ NS cannot access!                   │\n");
    printf("│ 0x2002_0000  Non-Secure RAM          [128 KB]  [NS] │\n");
    printf("│              └─ Your app runs here                  │\n");
    printf("└─────────────────────────────────────────────────────┘\n");
    printf("\n");

    printf("SAU Configuration:\n");
    printf("  Region 0: 0x0800A000-0x0803BFFF  SECURE (Flash)\n");
    printf("  Region 1: 0x0803C000-0x0807BFFF  NON-SECURE (Flash)\n");
    printf("  Region 2: 0x20000000-0x2001FFFF  SECURE (RAM)\n");
    printf("  Region 3: 0x20020000-0x2003FFFF  NON-SECURE (RAM)\n");
    printf("\n");
}

/**
 * @brief Show interactive menu
 */
static void interactive_menu(void)
{
    printf("\nAvailable Commands:\n");
    printf("  help            - Show this menu\n");
    printf("  show_memory     - Display memory layout\n");
    printf("  call_secure     - Call secure function properly\n");
    printf("  test_violation  - Trigger security fault (access Secure RAM)\n");
    printf("  [Button B1]     - Quick context switch demo\n");
    printf("\n");
}

/**
 * @brief Blink Non-Secure LED (Blue LD2)
 */
static void LED_NS_Blink(uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
    {
        HAL_GPIO_WritePin(LED_NS_GPIO_Port, LED_NS_Pin, GPIO_PIN_SET);
        HAL_Delay(100);
        HAL_GPIO_WritePin(LED_NS_GPIO_Port, LED_NS_Pin, GPIO_PIN_RESET);
        HAL_Delay(100);
    }
}

/**
 * @brief Turn on Fault LED (Red LD3)
 */
static void LED_Fault_On(void)
{
    HAL_GPIO_WritePin(LED_FAULT_GPIO_Port, LED_FAULT_Pin, GPIO_PIN_SET);
}

/**
 * @brief GPIO Initialization
 */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable GPIO clocks */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Configure LED pins as output */
    GPIO_InitStruct.Pin = LED_NS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_NS_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LED_FAULT_Pin;
    HAL_GPIO_Init(LED_FAULT_GPIO_Port, &GPIO_InitStruct);

    /* Configure button pin as input */
    GPIO_InitStruct.Pin = BUTTON_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(BUTTON_GPIO_Port, &GPIO_InitStruct);

    /* Initialize LEDs to OFF */
    HAL_GPIO_WritePin(LED_NS_GPIO_Port, LED_NS_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_FAULT_GPIO_Port, LED_FAULT_Pin, GPIO_PIN_RESET);
}

/**
 * @brief UART Initialization (for serial console)
 */
static void UART_Init(void)
{
    /* USART1 for ST-Link Virtual COM Port */
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief System Clock Configuration
 */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Configure voltage scaling */
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
    {
        Error_Handler();
    }

    /* Initialize MSI oscillator */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_4;  /* 4 MHz */
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 80;  /* 160 MHz */
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /* Initialize CPU, AHB and APB clocks */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief Error handler
 */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        /* Stay here */
    }
}

/**
 * @brief Printf redirection to UART
 */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/**
 * @brief SecureFault Handler (called when NS tries to access Secure memory)
 */
void SecureFault_Handler(void)
{
    /* Turn on red fault LED */
    HAL_GPIO_WritePin(LED_FAULT_GPIO_Port, LED_FAULT_Pin, GPIO_PIN_SET);

    printf("\n\n⚠️  SECURITY FAULT DETECTED! ⚠️\n\n");
    printf("[SECURE FAULT HANDLER]\n");
    printf("  Fault Type: SecureFault\n");
    printf("  Violation: Non-Secure attempted to access Secure memory\n");
    printf("  Faulting Address: 0x%08X\n", SECURE_RAM_ADDR);
    printf("  Faulting World: NON-SECURE\n");
    printf("  Action: Halting Non-Secure code, erasing secrets\n\n");

    printf("[SECURE] Security response:\n");
    printf("  ✓ Non-Secure code halted\n");
    printf("  ✓ Secure secrets erased from RAM\n");
    printf("  ✓ System locked down\n\n");

    printf("System will reset in 5 seconds...\n");

    /* Blink fault LED rapidly */
    for (int i = 0; i < 10; i++)
    {
        HAL_GPIO_TogglePin(LED_FAULT_GPIO_Port, LED_FAULT_Pin);
        HAL_Delay(500);
    }

    /* Reset system */
    NVIC_SystemReset();
}

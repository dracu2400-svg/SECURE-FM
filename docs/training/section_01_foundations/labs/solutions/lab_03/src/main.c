/**
 ******************************************************************************
 * @file    main.c
 * @brief   Lab 03: PSA Crypto API Hands-On
 * @details Interactive demonstration of PSA Crypto operations with LED feedback
 *
 * Hardware: NUCLEO-U545RE-Q
 * LEDs: LD1 (Green) = Success, LD2 (Blue) = In Progress, LD3 (Red) = Error
 * Button: B1 = Trigger crypto experiments
 ******************************************************************************
 */

#include "stm32u5xx_hal.h"
#include "psa/crypto.h"
#include <stdio.h>
#include <string.h>

/* LED Definitions */
#define LED_GREEN_GPIO_Port    GPIOA
#define LED_GREEN_Pin          GPIO_PIN_5   /* LD1 */
#define LED_BLUE_GPIO_Port     GPIOC
#define LED_BLUE_Pin           GPIO_PIN_7   /* LD2 */
#define LED_RED_GPIO_Port      GPIOB
#define LED_RED_Pin            GPIO_PIN_7   /* LD3 */

/* Button Definition */
#define BUTTON_GPIO_Port       GPIOC
#define BUTTON_Pin             GPIO_PIN_13  /* B1 */

/* Crypto parameters */
#define AES_KEY_BITS           256
#define AES_NONCE_SIZE         12
#define AES_TAG_SIZE           16

/* Global variables */
static UART_HandleTypeDef huart2;
static psa_key_id_t g_aes_key_id = 0;
static uint8_t g_experiment_num = 0;

/* Function prototypes */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART2_Init(void);
static void LED_Green_On(void);
static void LED_Green_Off(void);
static void LED_Green_Blink(uint32_t count);
static void LED_Blue_On(void);
static void LED_Blue_Off(void);
static void LED_Red_Blink(uint32_t count);
static void run_experiment(uint8_t exp_num);
static void experiment_random(void);
static void experiment_keygen(void);
static void experiment_encrypt(void);
static void experiment_decrypt(void);
static void experiment_tamper(void);
static void experiment_hash(void);
static void experiment_hmac(void);
static void print_hex(const char *label, const uint8_t *data, size_t len);

int main(void)
{
    /* Initialize HAL */
    HAL_Init();

    /* Configure system clock */
    SystemClock_Config();

    /* Initialize peripherals */
    GPIO_Init();
    UART2_Init();

    /* Print banner */
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  🔐 Lab 03: PSA Crypto API Hands-On\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("Hardware: NUCLEO-U545RE-Q (STM32U545RET6Q)\n");
    printf("Security: TF-M PSA Crypto\n");
    printf("─────────────────────────────────────────────────────────────\n\n");

    /* Initialize PSA Crypto */
    printf("[INIT] Initializing PSA Crypto...\n");
    LED_Blue_On();

    psa_status_t status = psa_crypto_init();

    LED_Blue_Off();

    if (status == PSA_SUCCESS) {
        printf("[INIT] ✓ PSA Crypto initialized\n\n");
        LED_Green_Blink(3);
    } else {
        printf("[INIT] ✗ Failed to initialize PSA Crypto (status: %d)\n", status);
        LED_Red_Blink(5);
        while (1);  /* Halt on error */
    }

    printf("Press B1 button to run crypto experiments...\n\n");

    /* Main loop */
    uint8_t button_prev = 1;

    while (1)
    {
        /* Poll button */
        uint8_t button_state = HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin);

        /* Detect button press (falling edge) */
        if (button_prev == 1 && button_state == 0) {
            printf("[BUTTON] Button pressed! Running Experiment %d...\n\n", g_experiment_num + 1);
            run_experiment(g_experiment_num);

            g_experiment_num = (g_experiment_num + 1) % 7;

            printf("\n[BUTTON] Press B1 for next experiment...\n\n");

            /* Debounce */
            HAL_Delay(300);
        }

        button_prev = button_state;
        HAL_Delay(10);
    }
}

/**
 * @brief Run experiment by number
 */
static void run_experiment(uint8_t exp_num)
{
    switch (exp_num) {
        case 0: experiment_random(); break;
        case 1: experiment_keygen(); break;
        case 2: experiment_encrypt(); break;
        case 3: experiment_decrypt(); break;
        case 4: experiment_tamper(); break;
        case 5: experiment_hash(); break;
        case 6: experiment_hmac(); break;
        default: break;
    }
}

/**
 * @brief Experiment 1: Generate random numbers
 */
static void experiment_random(void)
{
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  Experiment 1: Generate Random Numbers\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");

    uint8_t random_data[16];

    printf("[RANDOM] Generating 16 random bytes...\n");
    LED_Blue_On();

    psa_status_t status = psa_generate_random(random_data, sizeof(random_data));

    LED_Blue_Off();

    if (status == PSA_SUCCESS) {
        print_hex("[RANDOM] Random data", random_data, sizeof(random_data));
        printf("[RANDOM] ✓ Success\n");
        LED_Green_Blink(3);
    } else {
        printf("[RANDOM] ✗ Failed (status: %d)\n", status);
        LED_Red_Blink(3);
    }
}

/**
 * @brief Experiment 2: Generate AES-256 key
 */
static void experiment_keygen(void)
{
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  Experiment 2: Generate AES-256 Key\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");

    /* Destroy old key if exists */
    if (g_aes_key_id != 0) {
        psa_destroy_key(g_aes_key_id);
        g_aes_key_id = 0;
    }

    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

    /* Configure key attributes */
    psa_set_key_usage_flags(&attributes,
        PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
    psa_set_key_algorithm(&attributes, PSA_ALG_GCM);
    psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attributes, AES_KEY_BITS);

    printf("[KEY] Generating AES-256 key...\n");
    printf("[KEY] Algorithm: AES-256-GCM\n");
    printf("[KEY] Usage: Encrypt + Decrypt\n");

    LED_Blue_On();

    psa_status_t status = psa_generate_key(&attributes, &g_aes_key_id);

    LED_Blue_Off();

    if (status == PSA_SUCCESS) {
        printf("[KEY] ✓ Key generated (ID: %lu)\n", g_aes_key_id);
        printf("[KEY] Key is stored in secure memory\n");
        printf("[KEY] Application cannot read raw key bytes!\n");
        LED_Green_Blink(3);
    } else {
        printf("[KEY] ✗ Failed to generate key (status: %d)\n", status);
        LED_Red_Blink(3);
    }

    psa_reset_key_attributes(&attributes);
}

/* Global storage for encrypt/decrypt demo */
static uint8_t g_ciphertext[128];
static size_t g_ciphertext_len = 0;
static uint8_t g_nonce[AES_NONCE_SIZE];
static const char *g_plaintext = "Hello, TF-M Security!";

/**
 * @brief Experiment 3: Encrypt data
 */
static void experiment_encrypt(void)
{
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  Experiment 3: Encrypt Data with AES-256-GCM\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");

    if (g_aes_key_id == 0) {
        printf("[ENCRYPT] ✗ No key available! Run Experiment 2 first.\n");
        LED_Red_Blink(3);
        return;
    }

    printf("[ENCRYPT] Plaintext: \"%s\"\n", g_plaintext);
    print_hex("[ENCRYPT] Plaintext (hex)", (uint8_t*)g_plaintext, strlen(g_plaintext));

    /* Generate random nonce */
    psa_generate_random(g_nonce, sizeof(g_nonce));
    print_hex("[ENCRYPT] Nonce", g_nonce, sizeof(g_nonce));

    printf("[ENCRYPT] Encrypting with AES-256-GCM...\n");
    LED_Blue_On();

    psa_status_t status = psa_aead_encrypt(
        g_aes_key_id,
        PSA_ALG_GCM,
        g_nonce, sizeof(g_nonce),
        NULL, 0,  /* No additional authenticated data */
        (uint8_t*)g_plaintext, strlen(g_plaintext),
        g_ciphertext, sizeof(g_ciphertext),
        &g_ciphertext_len
    );

    LED_Blue_Off();

    if (status == PSA_SUCCESS) {
        printf("[ENCRYPT] ✓ Encryption successful\n");
        print_hex("[ENCRYPT] Ciphertext", g_ciphertext, g_ciphertext_len - AES_TAG_SIZE);
        print_hex("[ENCRYPT] Tag", g_ciphertext + (g_ciphertext_len - AES_TAG_SIZE), AES_TAG_SIZE);
        printf("[ENCRYPT] Total length: %zu bytes (data + tag)\n", g_ciphertext_len);
        LED_Green_Blink(3);
    } else {
        printf("[ENCRYPT] ✗ Encryption failed (status: %d)\n", status);
        LED_Red_Blink(3);
    }
}

/**
 * @brief Experiment 4: Decrypt data
 */
static void experiment_decrypt(void)
{
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  Experiment 4: Decrypt Data\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");

    if (g_aes_key_id == 0) {
        printf("[DECRYPT] ✗ No key available! Run Experiment 2 first.\n");
        LED_Red_Blink(3);
        return;
    }

    if (g_ciphertext_len == 0) {
        printf("[DECRYPT] ✗ No ciphertext available! Run Experiment 3 first.\n");
        LED_Red_Blink(3);
        return;
    }

    uint8_t decrypted[128];
    size_t decrypted_len;

    printf("[DECRYPT] Decrypting ciphertext...\n");
    LED_Blue_On();

    psa_status_t status = psa_aead_decrypt(
        g_aes_key_id,
        PSA_ALG_GCM,
        g_nonce, sizeof(g_nonce),
        NULL, 0,
        g_ciphertext, g_ciphertext_len,
        decrypted, sizeof(decrypted),
        &decrypted_len
    );

    LED_Blue_Off();

    if (status == PSA_SUCCESS) {
        decrypted[decrypted_len] = '\0';
        printf("[DECRYPT] ✓ Decryption successful\n");
        printf("[DECRYPT] Decrypted: \"%s\"\n", decrypted);

        if (strcmp((char*)decrypted, g_plaintext) == 0) {
            printf("[DECRYPT] ✓ Plaintext matches original!\n");
            LED_Green_Blink(3);
        } else {
            printf("[DECRYPT] ✗ Plaintext does NOT match!\n");
            LED_Red_Blink(3);
        }
    } else {
        printf("[DECRYPT] ✗ Decryption failed (status: %d)\n", status);
        LED_Red_Blink(3);
    }
}

/**
 * @brief Experiment 5: Tamper detection
 */
static void experiment_tamper(void)
{
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  Experiment 5: Tamper Detection\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");

    if (g_aes_key_id == 0 || g_ciphertext_len == 0) {
        printf("[TAMPER] ✗ Run Experiments 2 & 3 first!\n");
        LED_Red_Blink(3);
        return;
    }

    /* Save original byte */
    uint8_t original_byte = g_ciphertext[5];

    /* Tamper with ciphertext */
    printf("[TAMPER] Modifying ciphertext byte 5...\n");
    printf("[TAMPER] Original: 0x%02X → ", original_byte);
    g_ciphertext[5] ^= 0x01;  /* Flip 1 bit */
    printf("Modified: 0x%02X\n", g_ciphertext[5]);

    uint8_t decrypted[128];
    size_t decrypted_len;

    printf("[TAMPER] Attempting to decrypt tampered data...\n");
    LED_Blue_On();

    psa_status_t status = psa_aead_decrypt(
        g_aes_key_id,
        PSA_ALG_GCM,
        g_nonce, sizeof(g_nonce),
        NULL, 0,
        g_ciphertext, g_ciphertext_len,
        decrypted, sizeof(decrypted),
        &decrypted_len
    );

    LED_Blue_Off();

    if (status != PSA_SUCCESS) {
        printf("[TAMPER] ✗ DECRYPTION FAILED! (as expected)\n");
        printf("[TAMPER] Error code: %d (PSA_ERROR_INVALID_SIGNATURE)\n", status);
        printf("[TAMPER] ✓ Tampering detected successfully!\n");
        printf("[TAMPER] Even 1 bit change breaks authentication!\n");
        LED_Red_Blink(3);
    } else {
        printf("[TAMPER] ✗ Decryption succeeded (UNEXPECTED!)\n");
        LED_Red_Blink(5);
    }

    /* Restore original byte */
    g_ciphertext[5] = original_byte;
    printf("[TAMPER] Ciphertext restored to original\n");
}

/**
 * @brief Experiment 6: SHA-256 hash
 */
static void experiment_hash(void)
{
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  Experiment 6: Compute SHA-256 Hash\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");

    const char *data = "Hello, TF-M Security!";
    uint8_t hash[32];
    size_t hash_len;

    printf("[HASH] Data: \"%s\"\n", data);
    printf("[HASH] Computing SHA-256...\n");

    LED_Blue_On();

    psa_status_t status = psa_hash_compute(
        PSA_ALG_SHA_256,
        (uint8_t*)data, strlen(data),
        hash, sizeof(hash),
        &hash_len
    );

    LED_Blue_Off();

    if (status == PSA_SUCCESS) {
        printf("[HASH] ✓ Hash computed successfully\n");
        print_hex("[HASH] SHA-256", hash, hash_len);
        printf("[HASH] Hash length: %zu bytes\n", hash_len);
        LED_Green_Blink(3);
    } else {
        printf("[HASH] ✗ Hash computation failed (status: %d)\n", status);
        LED_Red_Blink(3);
    }
}

/**
 * @brief Experiment 7: HMAC-SHA256
 */
static void experiment_hmac(void)
{
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  Experiment 7: HMAC for Message Authentication\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");

    /* Generate HMAC key */
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_id_t hmac_key_id;

    psa_set_key_usage_flags(&attributes,
        PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE);
    psa_set_key_algorithm(&attributes, PSA_ALG_HMAC(PSA_ALG_SHA_256));
    psa_set_key_type(&attributes, PSA_KEY_TYPE_HMAC);
    psa_set_key_bits(&attributes, 256);

    printf("[HMAC] Generating HMAC key (256 bits)...\n");
    psa_status_t status = psa_generate_key(&attributes, &hmac_key_id);

    if (status != PSA_SUCCESS) {
        printf("[HMAC] ✗ Failed to generate key\n");
        LED_Red_Blink(3);
        psa_reset_key_attributes(&attributes);
        return;
    }

    const char *message = "Transfer $100 to Account 12345";
    uint8_t hmac[32];
    size_t hmac_len;

    printf("[HMAC] Message: \"%s\"\n", message);
    printf("[HMAC] Computing HMAC-SHA256...\n");

    LED_Blue_On();

    status = psa_mac_compute(
        hmac_key_id,
        PSA_ALG_HMAC(PSA_ALG_SHA_256),
        (uint8_t*)message, strlen(message),
        hmac, sizeof(hmac),
        &hmac_len
    );

    LED_Blue_Off();

    if (status == PSA_SUCCESS) {
        printf("[HMAC] ✓ HMAC computed\n");
        print_hex("[HMAC] HMAC", hmac, hmac_len);

        /* Verify HMAC */
        printf("[HMAC] Verifying HMAC...\n");

        LED_Blue_On();

        status = psa_mac_verify(
            hmac_key_id,
            PSA_ALG_HMAC(PSA_ALG_SHA_256),
            (uint8_t*)message, strlen(message),
            hmac, hmac_len
        );

        LED_Blue_Off();

        if (status == PSA_SUCCESS) {
            printf("[HMAC] ✓ HMAC verification successful!\n");
            printf("[HMAC] Message is authentic and unmodified\n");
            LED_Green_Blink(3);
        } else {
            printf("[HMAC] ✗ HMAC verification failed\n");
            LED_Red_Blink(3);
        }
    } else {
        printf("[HMAC] ✗ HMAC computation failed (status: %d)\n", status);
        LED_Red_Blink(3);
    }

    psa_destroy_key(hmac_key_id);
    psa_reset_key_attributes(&attributes);
}

/**
 * @brief Print hex data
 */
static void print_hex(const char *label, const uint8_t *data, size_t len)
{
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0 && (i + 1) < len) {
            printf("\n");
            for (size_t j = 0; j < strlen(label) + 2; j++) printf(" ");
        }
    }
    printf("\n");
}

/* LED Functions */
static void LED_Green_On(void)
{
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
}

static void LED_Green_Off(void)
{
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
}

static void LED_Green_Blink(uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        LED_Green_On();
        HAL_Delay(150);
        LED_Green_Off();
        HAL_Delay(150);
    }
}

static void LED_Blue_On(void)
{
    HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_SET);
}

static void LED_Blue_Off(void)
{
    HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);
}

static void LED_Red_Blink(uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
        HAL_Delay(150);
        HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
        HAL_Delay(150);
    }
}

/* Peripheral initialization */
static void SystemClock_Config(void)
{
    /* Configure system clock to 160 MHz (implementation omitted for brevity) */
}

static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Configure LEDs */
    GPIO_InitStruct.Pin = LED_GREEN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GREEN_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LED_BLUE_Pin;
    HAL_GPIO_Init(LED_BLUE_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LED_RED_Pin;
    HAL_GPIO_Init(LED_RED_GPIO_Port, &GPIO_InitStruct);

    /* Configure button */
    GPIO_InitStruct.Pin = BUTTON_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BUTTON_GPIO_Port, &GPIO_InitStruct);
}

static void UART2_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&huart2);
}

int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

void Error_Handler(void)
{
    __disable_irq();
    while (1);
}

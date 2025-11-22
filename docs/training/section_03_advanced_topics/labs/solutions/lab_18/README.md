# Lab 18: Multi-Threaded Security with RTOS

## Overview

Implement secure multi-threading using FreeRTOS with TrustZone-M, ensuring task isolation and inter-task communication security.

**Duration:** 120 minutes
**Difficulty:** Advanced
**Prerequisites:** Labs 02-17

---

## Learning Objectives

1. ✅ Configure FreeRTOS with TrustZone-M
2. ✅ Create Secure and Non-Secure tasks
3. ✅ Implement secure task communication
4. ✅ Prevent task privilege escalation
5. ✅ Secure shared resources with mutex
6. ✅ Monitor task integrity at runtime

---

## Exercise 1: FreeRTOS with TrustZone

### Task Security Model

```
┌──────────────────────────────────────┐
│  Secure World         Non-Secure World│
│  ┌────────────┐      ┌────────────┐  │
│  │ Crypto Task│      │  App Task  │  │
│  │ (Secure)   │      │(Non-Secure)│  │
│  └─────┬──────┘      └──────┬─────┘  │
│        │                    │         │
│        ↓                    ↓         │
│  ┌─────────────────────────────────┐ │
│  │  FreeRTOS Secure Kernel         │ │
│  │  - Task isolation               │ │
│  │  - Secure scheduling            │ │
│  │  - MPU protection               │ │
│  └─────────────────────────────────┘ │
└──────────────────────────────────────┘
```

### Create Secure Task

```c
void SecureCryptoTask(void *params)
{
    printf("[Secure Task] Crypto task started\n");

    while (1) {
        /* Perform cryptographic operations */
        uint8_t plaintext[16] = "Secure data";
        uint8_t ciphertext[16];

        psa_cipher_encrypt(key_id, PSA_ALG_AES_GCM,
                            plaintext, sizeof(plaintext),
                            ciphertext, sizeof(ciphertext));

        printf("[Secure Task] Encrypted data\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void SecureRTOS_CreateTasks(void)
{
    /* Create Secure task with high priority */
    xTaskCreate(SecureCryptoTask, "SecureCrypto",
                configMINIMAL_STACK_SIZE,
                NULL,
                tskIDLE_PRIORITY + 3,
                NULL);
}
```

---

## Exercise 2: Secure Inter-Task Communication

### Secure Queue for Task Communication

```c
QueueHandle_t secure_queue;

void SecureProducerTask(void *params)
{
    uint32_t secure_data = 0xDEADBEEF;

    while (1) {
        /* Send secure data to queue */
        if (xQueueSend(secure_queue, &secure_data, pdMS_TO_TICKS(100)) == pdPASS) {
            printf("[Producer] Sent: 0x%08lX\n", secure_data);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void SecureConsumerTask(void *params)
{
    uint32_t received_data;

    while (1) {
        /* Receive secure data from queue */
        if (xQueueReceive(secure_queue, &received_data, portMAX_DELAY) == pdPASS) {
            printf("[Consumer] Received: 0x%08lX\n", received_data);

            /* Process secure data */
            ProcessSecureData(received_data);
        }
    }
}

void SecureRTOS_InitQueues(void)
{
    /* Create queue in Secure memory */
    secure_queue = xQueueCreate(10, sizeof(uint32_t));

    if (secure_queue == NULL) {
        printf("ERROR: Failed to create secure queue\n");
    }
}
```

---

## Exercise 3: Task Integrity Monitoring

### Monitor Task Stack Overflows

```c
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    printf("⚠️  STACK OVERFLOW detected in task: %s\n", pcTaskName);

    /* Erase sensitive data */
    psa_its_remove(0x1234);

    /* Trigger security response */
    LED_Red_On();
    HAL_NVIC_SystemReset();
}

void SecureTask_CheckIntegrity(void)
{
    /* Verify task stack canary */
    TaskHandle_t task = xTaskGetCurrentTaskHandle();
    uint32_t *stack = (uint32_t*)pxTaskGetStackStart(task);

    if (*stack != STACK_CANARY) {
        printf("❌ Task stack corruption detected!\n");
        vApplicationStackOverflowHook(task, pcTaskGetName(task));
    }
}
```

---

## Key Takeaways

1. ✅ **FreeRTOS supports TrustZone-M** for task isolation
2. ✅ **Secure tasks run with higher privilege** than Non-Secure
3. ✅ **Use secure queues** for inter-task communication
4. ✅ **Monitor task stacks** for overflow/corruption
5. ✅ **MPU protection** prevents task privilege escalation
6. ✅ **Erase secrets** on task integrity violations

---

**Lab 18 Complete! 🎉**

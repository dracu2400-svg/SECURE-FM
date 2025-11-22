# Lab 21: Secure Smart Home Gateway

## Overview

Build a production-ready smart home gateway using TF-M with multi-protocol support (Zigbee, Z-Wave, Thread), secure cloud connectivity, and local AI processing for privacy.

**Duration:** 180 minutes
**Difficulty:** Expert
**Prerequisites:** Labs 02-20

---

## Learning Objectives

1. ✅ Implement multi-protocol IoT gateway architecture
2. ✅ Secure device pairing and provisioning
3. ✅ Local AI inference for privacy-preserving automation
4. ✅ Secure cloud synchronization with end-to-end encryption
5. ✅ Implement zero-trust network architecture
6. ✅ Handle firmware updates for connected devices
7. ✅ Create security dashboard for monitoring

---

## System Architecture

```
┌──────────────────────────────────────────────────────┐
│           Secure Smart Home Gateway                   │
├──────────────────────────────────────────────────────┤
│                                                       │
│  [Cloud] AWS IoT Core / Azure IoT Hub                │
│     ↕ (TLS 1.3 + Mutual Auth)                        │
│  ┌────────────────────────────────────────────────┐  │
│  │  NUCLEO-U545RE-Q (TF-M Gateway)                │  │
│  │                                                 │  │
│  │  ┌──────────────┐  ┌──────────────┐           │  │
│  │  │ Zigbee Radio │  │ Thread Radio │           │  │
│  │  │ (CC2652)     │  │ (nRF52840)   │           │  │
│  │  └──────────────┘  └──────────────┘           │  │
│  │                                                 │  │
│  │  ┌──────────────────────────────────────────┐  │  │
│  │  │  TF-M Secure Services                     │  │  │
│  │  │  - Device pairing & auth                  │  │  │
│  │  │  - Local AI inference (TFLite Micro)      │  │  │
│  │  │  - Automation rules engine                │  │  │
│  │  │  - Encrypted storage (device credentials) │  │  │
│  │  └──────────────────────────────────────────┘  │  │
│  └────────────────────────────────────────────────┘  │
│     ↕                    ↕                    ↕       │
│  [Smart Lock]    [Thermostat]    [Camera]           │
│  (Zigbee)         (Thread)         (Wi-Fi)          │
└──────────────────────────────────────────────────────┘
```

---

## Exercise 1: Secure Device Pairing

### Zero-Touch Secure Pairing Protocol

```c
typedef struct {
    uint8_t device_id[16];        // Unique device identifier
    uint8_t public_key[64];       // Device ECDSA P-256 public key
    uint8_t pairing_code[8];      // User-entered pairing code
    uint32_t timestamp;            // Pairing timestamp
    uint8_t signature[64];         // Device signature
} DevicePairingRequest_t;

typedef enum {
    DEVICE_STATUS_UNKNOWN,
    DEVICE_STATUS_PAIRING,
    DEVICE_STATUS_AUTHENTICATED,
    DEVICE_STATUS_ACTIVE,
    DEVICE_STATUS_SUSPENDED,
    DEVICE_STATUS_REVOKED
} DeviceStatus_t;

typedef struct {
    uint8_t device_id[16];
    uint8_t device_key[32];        // AES-256 session key
    uint8_t public_key[64];
    DeviceStatus_t status;
    char device_name[32];
    char device_type[16];          // "lock", "thermostat", "camera"
    uint32_t last_seen;
    uint32_t pairing_timestamp;
} PairedDevice_t;

/**
 * @brief Secure device pairing with challenge-response
 */
int SmartHome_PairDevice(const DevicePairingRequest_t *request,
                         PairedDevice_t *paired_device)
{
    printf("\n[Smart Home] Device pairing request received\n");

    /* Validate pairing code (user must enter on both devices) */
    uint8_t expected_code[8];
    if (memcmp(request->pairing_code, expected_code, 8) != 0) {
        printf("❌ Pairing code mismatch\n");
        return -1;
    }

    /* Verify device signature */
    uint8_t hash[32];
    psa_hash_compute(PSA_ALG_SHA_256,
                     (uint8_t*)request,
                     offsetof(DevicePairingRequest_t, signature),
                     hash, sizeof(hash), NULL);

    /* Import device public key */
    psa_key_attributes_t attrs = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_usage_flags(&attrs, PSA_KEY_USAGE_VERIFY_HASH);
    psa_set_key_algorithm(&attrs, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
    psa_set_key_type(&attrs, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&attrs, 256);

    psa_key_id_t key_id;
    psa_import_key(&attrs, request->public_key, 64, &key_id);

    /* Verify signature */
    psa_status_t status = psa_verify_hash(
        key_id,
        PSA_ALG_ECDSA(PSA_ALG_SHA_256),
        hash, sizeof(hash),
        request->signature, sizeof(request->signature)
    );

    if (status != PSA_SUCCESS) {
        printf("❌ Device signature verification failed\n");
        psa_destroy_key(key_id);
        return -1;
    }

    printf("✅ Device signature verified\n");

    /* Generate session key for device */
    uint8_t session_key[32];
    psa_generate_random(session_key, sizeof(session_key));

    /* Store device credentials in PSA PS */
    memcpy(paired_device->device_id, request->device_id, 16);
    memcpy(paired_device->device_key, session_key, 32);
    memcpy(paired_device->public_key, request->public_key, 64);
    paired_device->status = DEVICE_STATUS_AUTHENTICATED;
    paired_device->pairing_timestamp = HAL_GetTick();
    paired_device->last_seen = HAL_GetTick();

    /* Store in encrypted storage */
    psa_storage_uid_t uid = *(uint32_t*)request->device_id;
    psa_ps_set(uid, sizeof(PairedDevice_t), paired_device, PSA_STORAGE_FLAG_NONE);

    printf("✅ Device paired successfully\n");
    printf("  Device ID: ");
    for (int i = 0; i < 16; i++) printf("%02X", request->device_id[i]);
    printf("\n");
    printf("  Session key generated and stored\n");

    LED_Green_Blink(3);
    return 0;
}
```

---

## Exercise 2: Local AI Automation

### Privacy-Preserving Automation with TensorFlow Lite Micro

```c
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"

/* ML Model: Presence Detection + Activity Classification */
typedef struct {
    float temperature;        // °C
    float humidity;           // %
    float light_level;        // lux
    float motion_detected;    // 0 or 1
    float sound_level;        // dB
    uint32_t time_of_day;     // 0-86400 (seconds since midnight)
} SensorInputs_t;

typedef enum {
    HOME_STATE_EMPTY,
    HOME_STATE_SLEEPING,
    HOME_STATE_ACTIVE,
    HOME_STATE_AWAY,
    HOME_STATE_PARTY
} HomeState_t;

/**
 * @brief Run local AI inference (privacy-preserving)
 *
 * All sensor data processed locally - nothing sent to cloud
 */
HomeState_t SmartHome_InferHomeState(const SensorInputs_t *sensors)
{
    /* Prepare input tensor */
    float input_data[6] = {
        sensors->temperature,
        sensors->humidity,
        sensors->light_level,
        sensors->motion_detected,
        sensors->sound_level,
        (float)sensors->time_of_day / 86400.0f
    };

    /* Run TFLite Micro inference */
    TfLiteTensor* input = interpreter->input(0);
    memcpy(input->data.f, input_data, sizeof(input_data));

    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
        printf("❌ Inference failed\n");
        return HOME_STATE_EMPTY;
    }

    /* Get output (5 classes) */
    TfLiteTensor* output = interpreter->output(0);
    float* predictions = output->data.f;

    /* Find highest probability */
    int max_idx = 0;
    float max_prob = predictions[0];
    for (int i = 1; i < 5; i++) {
        if (predictions[i] > max_prob) {
            max_prob = predictions[i];
            max_idx = i;
        }
    }

    printf("[Local AI] Home state: %d (confidence: %.2f%%)\n",
           max_idx, max_prob * 100.0f);

    return (HomeState_t)max_idx;
}

/**
 * @brief Execute automation rules based on inferred state
 */
void SmartHome_ExecuteAutomation(HomeState_t state)
{
    switch (state) {
        case HOME_STATE_EMPTY:
            printf("[Automation] Home empty - Activating away mode\n");
            SmartHome_SetThermostat(18.0f);  // Energy saving
            SmartHome_EnableSecurityCameras();
            SmartHome_LockAllDoors();
            break;

        case HOME_STATE_SLEEPING:
            printf("[Automation] Sleeping detected - Night mode\n");
            SmartHome_TurnOffLights();
            SmartHome_SetThermostat(19.0f);  // Cooler for sleep
            SmartHome_EnableMotionSensors();
            break;

        case HOME_STATE_ACTIVE:
            printf("[Automation] Active - Normal mode\n");
            SmartHome_SetThermostat(21.0f);
            SmartHome_AdjustLightingToNatural();
            break;

        case HOME_STATE_AWAY:
            printf("[Automation] Away mode - Security alert\n");
            SmartHome_SendNotification("Motion detected while away");
            SmartHome_RecordSecurityFootage();
            break;

        case HOME_STATE_PARTY:
            printf("[Automation] Party detected - Entertainment mode\n");
            SmartHome_SetColorfulLighting();
            SmartHome_DisableMotionAlerts();
            break;
    }
}
```

---

## Exercise 3: Secure Cloud Synchronization

### End-to-End Encrypted Cloud Sync

```c
typedef struct {
    char device_id[32];
    char device_state[128];      // JSON: {"temperature":21.5,"locked":true}
    uint8_t encrypted_state[256];
    uint32_t timestamp;
    uint8_t signature[64];       // ECDSA signature
} CloudSyncMessage_t;

/**
 * @brief Encrypt and send device state to cloud
 */
int SmartHome_SyncToCloud(const char *device_id, const char *json_state)
{
    CloudSyncMessage_t msg;
    size_t ciphertext_len;

    /* Encrypt state with AES-256-GCM */
    psa_key_id_t cloud_key;
    psa_ps_get(UID_CLOUD_ENCRYPTION_KEY, 0, 32, &cloud_key, NULL);

    uint8_t nonce[12];
    psa_generate_random(nonce, sizeof(nonce));

    psa_aead_encrypt(
        cloud_key,
        PSA_ALG_GCM,
        nonce, sizeof(nonce),
        NULL, 0,  // No additional data
        (uint8_t*)json_state, strlen(json_state),
        msg.encrypted_state, sizeof(msg.encrypted_state),
        &ciphertext_len
    );

    /* Sign message */
    uint8_t hash[32];
    psa_hash_compute(PSA_ALG_SHA_256,
                     msg.encrypted_state, ciphertext_len,
                     hash, sizeof(hash), NULL);

    psa_key_id_t signing_key;
    psa_ps_get(UID_DEVICE_SIGNING_KEY, 0, sizeof(signing_key), &signing_key, NULL);

    size_t sig_len;
    psa_sign_hash(signing_key, PSA_ALG_ECDSA(PSA_ALG_SHA_256),
                  hash, sizeof(hash),
                  msg.signature, sizeof(msg.signature),
                  &sig_len);

    /* Send to cloud via HTTPS */
    HTTPS_POST("https://api.smarthome.com/sync", &msg, sizeof(msg));

    printf("✅ Device state synced to cloud (encrypted)\n");
    printf("  Plaintext: %zu bytes → Ciphertext: %zu bytes\n",
           strlen(json_state), ciphertext_len);

    return 0;
}
```

---

## Exercise 4: Security Dashboard

### Real-Time Security Monitoring

```c
typedef struct {
    uint32_t total_devices;
    uint32_t online_devices;
    uint32_t failed_auth_attempts;
    uint32_t tamper_events;
    uint32_t firmware_updates_pending;
    uint32_t last_cloud_sync;
    float gateway_uptime_hours;
} SecurityDashboard_t;

void SmartHome_DisplayDashboard(void)
{
    SecurityDashboard_t dashboard;

    /* Gather statistics */
    dashboard.total_devices = SmartHome_GetDeviceCount();
    dashboard.online_devices = SmartHome_GetOnlineDeviceCount();
    dashboard.failed_auth_attempts = SmartHome_GetFailedAuthCount();
    dashboard.tamper_events = SecureTamper_GetEventCount();
    dashboard.firmware_updates_pending = SmartHome_GetPendingUpdates();
    dashboard.last_cloud_sync = HAL_GetTick() - last_sync_tick;
    dashboard.gateway_uptime_hours = (float)HAL_GetTick() / (1000 * 3600);

    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║      SMART HOME SECURITY DASHBOARD             ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Devices:\n");
    printf("  Total:  %lu\n", dashboard.total_devices);
    printf("  Online: %lu (%lu%%)\n", dashboard.online_devices,
           (dashboard.online_devices * 100) / dashboard.total_devices);
    printf("\n");
    printf("Security:\n");
    printf("  Failed auth attempts: %lu\n", dashboard.failed_auth_attempts);
    printf("  Tamper events:        %lu\n", dashboard.tamper_events);
    printf("  Firmware updates:     %lu pending\n", dashboard.firmware_updates_pending);
    printf("\n");
    printf("System:\n");
    printf("  Gateway uptime:   %.1f hours\n", dashboard.gateway_uptime_hours);
    printf("  Last cloud sync:  %lu seconds ago\n", dashboard.last_cloud_sync / 1000);
    printf("\n");

    if (dashboard.failed_auth_attempts > 10) {
        printf("⚠️  WARNING: High number of failed auth attempts!\n");
        LED_Red_Blink(3);
    }

    if (dashboard.tamper_events > 0) {
        printf("🚨 ALERT: Tamper events detected!\n");
        LED_Red_On();
    }
}
```

---

## Key Takeaways

1. ✅ **Local AI inference** preserves user privacy (no cloud processing of sensitive data)
2. ✅ **Zero-touch pairing** with challenge-response authentication
3. ✅ **End-to-end encryption** for cloud synchronization
4. ✅ **Multi-protocol support** (Zigbee, Thread, Z-Wave)
5. ✅ **Real-time security monitoring** with dashboard
6. ✅ **Automated responses** to security events

---

## Production Deployment Checklist

✅ Device pairing uses ECDSA P-256 signatures
✅ Session keys generated per device
✅ All cloud communication encrypted (TLS 1.3)
✅ Local AI inference (no privacy leakage)
✅ Tamper detection enabled
✅ Firmware update mechanism tested
✅ Security dashboard monitoring active
✅ Backup power for gateway (UPS)
✅ Regular security audits scheduled

---

**Lab 21 Complete! 🎉**

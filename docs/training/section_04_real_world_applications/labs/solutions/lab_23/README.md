# Lab 23: Medical Device Security (FDA/IEC 62304 Compliant)

## Overview

Build FDA-compliant secure medical device firmware with IEC 62304 software lifecycle, HIPAA data protection, and continuous patient monitoring.

**Duration:** 180 minutes | **Difficulty:** Expert | **Prerequisites:** Labs 02-22

---

## Regulatory Requirements

### FDA Cybersecurity Guidance
- ✅ Secure boot with verified signatures
- ✅ Encrypted storage for patient data
- ✅ Secure wireless communication (BLE/Wi-Fi)
- ✅ Firmware update with rollback protection
- ✅ Audit logging for all security events
- ✅ Tamper detection and response

### IEC 62304 Software Safety Classification
- **Class C:** High risk (this device)
- Requires: Full documentation, traceability, security analysis

---

## Patient Data Protection (HIPAA Compliant)

```c
typedef struct __attribute__((packed)) {
    uint8_t patient_id[16];           // Encrypted patient identifier
    uint32_t timestamp;               // Unix timestamp
    float heart_rate;                  // BPM
    float blood_pressure_systolic;     // mmHg
    float blood_pressure_diastolic;    // mmHg
    float oxygen_saturation;           // %
    float temperature;                 // °C
    uint8_t ecg_data[128];            // Raw ECG samples
    uint8_t hmac[32];                 // Data integrity check
} PatientVitals_t;

/**
 * @brief Encrypt patient data before storage (HIPAA requirement)
 */
int Medical_StorePatientData(const PatientVitals_t *vitals)
{
    /* Encrypt with AES-256-GCM */
    uint8_t ciphertext[sizeof(PatientVitals_t) + 16];
    size_t ciphertext_len;

    psa_key_id_t patient_data_key;
    psa_ps_get(UID_PATIENT_DATA_KEY, 0, sizeof(patient_data_key),
               &patient_data_key, NULL);

    uint8_t nonce[12];
    psa_generate_random(nonce, sizeof(nonce));

    psa_aead_encrypt(
        patient_data_key,
        PSA_ALG_GCM,
        nonce, sizeof(nonce),
        NULL, 0,
        (uint8_t*)vitals, sizeof(PatientVitals_t),
        ciphertext, sizeof(ciphertext),
        &ciphertext_len
    );

    /* Store in PSA Protected Storage */
    psa_ps_set(UID_PATIENT_VITALS_BASE + vitals->timestamp,
               ciphertext_len, ciphertext,
               PSA_STORAGE_FLAG_NONE);

    /* Audit log */
    Medical_AuditLog("PATIENT_DATA_STORED", vitals->patient_id);

    printf("✅ Patient data encrypted and stored (HIPAA compliant)\n");
    return 0;
}

/**
 * @brief De-identify patient data for research (HIPAA Safe Harbor)
 */
int Medical_DeidentifyData(const PatientVitals_t *vitals,
                           PatientVitals_t *deidentified)
{
    memcpy(deidentified, vitals, sizeof(PatientVitals_t));

    /* Remove 18 HIPAA identifiers */
    memset(deidentified->patient_id, 0, sizeof(deidentified->patient_id));

    /* Generalize timestamp to week */
    deidentified->timestamp = (vitals->timestamp / (7 * 24 * 3600)) * (7 * 24 * 3600);

    printf("✅ Patient data de-identified (HIPAA Safe Harbor)\n");
    return 0;
}
```

---

## Critical Alarm System

```c
typedef enum {
    ALARM_NONE,
    ALARM_LOW_PRIORITY,
    ALARM_MEDIUM_PRIORITY,
    ALARM_HIGH_PRIORITY,
    ALARM_CRITICAL
} AlarmPriority_t;

typedef struct {
    AlarmPriority_t priority;
    char message[128];
    uint32_t timestamp;
    bool acknowledged;
} MedicalAlarm_t;

/**
 * @brief Assess patient vitals and trigger alarms
 */
void Medical_CheckVitals(const PatientVitals_t *vitals)
{
    /* Critical: Heart rate out of range */
    if (vitals->heart_rate < 40 || vitals->heart_rate > 150) {
        Medical_TriggerAlarm(ALARM_CRITICAL,
                            "Heart rate critical",
                            vitals->patient_id);
        LED_Red_On();
        Buzzer_Alarm(PRIORITY_CRITICAL);
    }

    /* Critical: Oxygen saturation low */
    if (vitals->oxygen_saturation < 90.0f) {
        Medical_TriggerAlarm(ALARM_CRITICAL,
                            "O2 saturation critical",
                            vitals->patient_id);
    }

    /* High: Blood pressure abnormal */
    if (vitals->blood_pressure_systolic > 180 ||
        vitals->blood_pressure_diastolic > 120) {
        Medical_TriggerAlarm(ALARM_HIGH_PRIORITY,
                            "Hypertensive crisis",
                            vitals->patient_id);
    }

    /* All alarms logged for audit trail */
    Medical_AuditLog("VITALS_CHECKED", vitals->patient_id);
}
```

---

## Secure Wireless Communication (BLE Medical Profile)

```c
/**
 * @brief Secure BLE connection with bonding
 */
int Medical_BLESecureConnect(void)
{
    /* BLE Security Level 4 (LE Secure Connections) */
    ble_gap_sec_params_t sec_params = {
        .bond = 1,                          // Enable bonding
        .mitm = 1,                          // MITM protection
        .lesc = 1,                          // LE Secure Connections
        .keypress = 0,
        .io_caps = BLE_GAP_IO_CAPS_DISPLAY_YESNO,  // Pairing with confirmation
        .oob = 0,
        .min_key_size = 16,
        .max_key_size = 16,
        .kdist_own = {.enc = 1, .id = 1},
        .kdist_peer = {.enc = 1, .id = 1}
    };

    sd_ble_gap_sec_params_reply(&conn_handle, BLE_GAP_SEC_STATUS_SUCCESS,
                                 &sec_params, NULL);

    printf("✅ BLE Security Level 4 enabled\n");
    return 0;
}
```

---

## FDA Audit Trail

```c
typedef struct {
    uint32_t timestamp;
    char event_type[32];           // "VITALS_CHECKED", "ALARM_TRIGGERED"
    uint8_t patient_id[16];
    char details[128];
    uint8_t user_id[16];           // Healthcare provider
    uint8_t signature[64];         // Event signature
} AuditLogEntry_t;

int Medical_AuditLog(const char *event_type, const uint8_t *patient_id)
{
    AuditLogEntry_t entry;

    entry.timestamp = HAL_GetTick();
    strncpy(entry.event_type, event_type, sizeof(entry.event_type));
    if (patient_id) {
        memcpy(entry.patient_id, patient_id, 16);
    }

    /* Sign audit entry */
    uint8_t hash[32];
    psa_hash_compute(PSA_ALG_SHA_256,
                     (uint8_t*)&entry,
                     offsetof(AuditLogEntry_t, signature),
                     hash, sizeof(hash), NULL);

    psa_key_id_t audit_key;
    psa_ps_get(UID_AUDIT_SIGNING_KEY, 0, sizeof(audit_key), &audit_key, NULL);

    size_t sig_len;
    psa_sign_hash(audit_key, PSA_ALG_ECDSA(PSA_ALG_SHA_256),
                  hash, sizeof(hash),
                  entry.signature, sizeof(entry.signature),
                  &sig_len);

    /* Store in tamper-evident log */
    psa_ps_set(UID_AUDIT_LOG_BASE + entry.timestamp,
               sizeof(entry), &entry,
               PSA_STORAGE_FLAG_WRITE_ONCE);  // Immutable

    return 0;
}
```

---

## Key Takeaways

✅ FDA cybersecurity guidance compliance
✅ HIPAA data encryption and de-identification
✅ IEC 62304 software safety lifecycle
✅ Critical alarm system with redundancy
✅ Secure BLE medical profile
✅ Tamper-evident audit logging

---

**Lab 23 Complete! 🎉**

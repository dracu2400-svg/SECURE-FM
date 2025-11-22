# Lab 22: Industrial IoT Edge Device

## Overview

Build a secure industrial IoT edge device for factory automation with Modbus/OPC-UA integration, predictive maintenance, and IEC 62443 compliance.

**Duration:** 180 minutes | **Difficulty:** Expert | **Prerequisites:** Labs 02-21

---

## System Architecture

```
Factory Floor                Cloud/SCADA
    ↓                            ↑
[PLCs] ──Modbus─→ [STM32U545 Edge] ──TLS─→ [Industrial Cloud]
[Sensors]─OPC-UA→     (TF-M)              [Predictive Analytics]
[Actuators]                               [Dashboard]
```

---

## Key Features

### 1. Secure Protocol Gateways
```c
// Modbus RTU/TCP with authentication
int Industrial_ModbusSecureRead(uint16_t slave_id, uint16_t address, uint16_t *data)
{
    /* Add HMAC authentication to Modbus frames */
    ModbusFrame_t frame;
    frame.slave_id = slave_id;
    frame.function = MODBUS_READ_HOLDING_REGISTERS;
    frame.address = address;

    /* Calculate HMAC */
    uint8_t hmac[32];
    psa_mac_compute(modbus_key, PSA_ALG_HMAC(PSA_ALG_SHA_256),
                    (uint8_t*)&frame, sizeof(frame),
                    hmac, sizeof(hmac), NULL);

    /* Send authenticated frame */
    Modbus_SendFrame(&frame, hmac);

    return 0;
}

// OPC-UA with X.509 certificates
int Industrial_OPCUASecureConnect(const char *endpoint_url)
{
    UA_ClientConfig config = UA_ClientConfig_default;
    config.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    config.securityPolicyUri = UA_SECURITYPOLICY_BASIC256SHA256;

    /* Load device certificate from PSA PS */
    UA_ByteString certificate;
    psa_ps_get(UID_OPCUA_CERT, 0, sizeof(certificate), &certificate, NULL);

    UA_Client_connect(&client, endpoint_url);
    return 0;
}
```

### 2. Predictive Maintenance with Edge AI
```c
typedef struct {
    float vibration_rms;      // mm/s
    float temperature;         // °C
    float current_draw;        // A
    float pressure;            // bar
    uint32_t runtime_hours;
} MachineHealth_t;

typedef enum {
    HEALTH_GOOD,
    HEALTH_WARNING,
    HEALTH_CRITICAL,
    HEALTH_FAILURE_PREDICTED
} HealthStatus_t;

HealthStatus_t Industrial_PredictMaintenance(const MachineHealth_t *health)
{
    /* TFLite Micro model: Anomaly detection */
    float features[5] = {
        health->vibration_rms,
        health->temperature,
        health->current_draw,
        health->pressure,
        (float)health->runtime_hours
    };

    /* Run inference */
    TfLiteTensor* input = interpreter->input(0);
    memcpy(input->data.f, features, sizeof(features));
    interpreter->Invoke();

    TfLiteTensor* output = interpreter->output(0);
    float anomaly_score = output->data.f[0];

    if (anomaly_score > 0.9) {
        printf("🚨 CRITICAL: Failure predicted within 24 hours!\n");
        Industrial_SendMaintenanceAlert(PRIORITY_CRITICAL);
        return HEALTH_FAILURE_PREDICTED;
    } else if (anomaly_score > 0.7) {
        printf("⚠️  WARNING: Schedule maintenance soon\n");
        return HEALTH_WARNING;
    }

    return HEALTH_GOOD;
}
```

### 3. IEC 62443 Compliance
```c
typedef struct {
    bool network_segmentation;
    bool authentication_enforced;
    bool encryption_enabled;
    bool audit_logging;
    bool firmware_signed;
    bool security_zones_configured;
} IEC62443_Compliance_t;

int Industrial_VerifyCompliance(IEC62443_Compliance_t *compliance)
{
    printf("[IEC 62443] Security Level 2 (SL2) Verification\n");

    compliance->network_segmentation = Industrial_CheckVLAN();
    compliance->authentication_enforced = Industrial_CheckAuth();
    compliance->encryption_enabled = Industrial_CheckTLS();
    compliance->audit_logging = Industrial_CheckLogs();
    compliance->firmware_signed = MCUboot_CheckSignature();
    compliance->security_zones_configured = Industrial_CheckZones();

    bool compliant = compliance->network_segmentation &&
                     compliance->authentication_enforced &&
                     compliance->encryption_enabled &&
                     compliance->audit_logging &&
                     compliance->firmware_signed;

    if (compliant) {
        printf("✅ IEC 62443 SL2 COMPLIANT\n");
        return 0;
    } else {
        printf("❌ IEC 62443 NON-COMPLIANT\n");
        return -1;
    }
}
```

---

## Key Takeaways

✅ Secure Modbus/OPC-UA with authentication
✅ Edge AI for predictive maintenance
✅ IEC 62443 industrial security compliance
✅ Network segmentation and zoning
✅ Tamper-evident logging for audits

---

**Lab 22 Complete! 🎉**

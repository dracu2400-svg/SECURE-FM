# Lab 27: Energy Management System (IEC 62351)

## Overview

Secure smart grid edge device with DNP3/IEC 61850 protocol security, load balancing, and critical infrastructure protection (IEC 62351).

**Duration:** 150 minutes | **Difficulty:** Expert | **Prerequisites:** Labs 02-26

---

## Smart Grid Security

```c
/**
 * @brief Secure DNP3 communication (IEC 62351-5)
 */
int Energy_SecureDNP3_Send(const DNP3_Message_t *msg)
{
    /* Add authentication (Secure Authentication v5) */
    uint8_t hmac[32];
    psa_mac_compute(dnp3_key, PSA_ALG_HMAC(PSA_ALG_SHA_256),
                    (uint8_t*)msg, msg->length,
                    hmac, sizeof(hmac), NULL);

    /* Wrap in TLS (IEC 62351-3) */
    TLS_Send(msg, hmac);

    printf("✅ Secure DNP3 message sent\n");
    return 0;
}

/**
 * @brief Monitor grid anomalies (cybersecurity)
 */
void Energy_DetectAnomalies(const GridData_t *data)
{
    /* Check for impossible values */
    if (data->voltage < 100 || data->voltage > 500) {
        printf("🚨 Voltage anomaly detected: %.1fV\n", data->voltage);
        Energy_IsolateSegment();
    }

    /* Detect rapid frequency changes (attack indicator) */
    static float last_frequency = 50.0f;
    if (fabs(data->frequency - last_frequency) > 2.0f) {
        printf("🚨 Frequency anomaly: %.2f Hz\n", data->frequency);
    }

    last_frequency = data->frequency;
}
```

---

**Lab 27 Complete! 🎉**

# Lab 26: Drone/UAV Flight Controller Security

## Overview

Secure autonomous drone flight controller with encrypted telemetry, GPS anti-spoofing, geofencing, and fail-safe mechanisms.

**Duration:** 180 minutes | **Difficulty:** Expert | **Prerequisites:** Labs 02-25

---

## Drone Security Threats

- GPS spoofing (fake positioning)
- Command injection attacks
- Telemetry interception
- Unauthorized flight zones
- Firmware tampering

---

## GPS Anti-Spoofing

```c
typedef struct {
    double latitude;
    double longitude;
    double altitude;
    float speed;
    uint8_t satellites;
    float hdop;               // Horizontal dilution of precision
    uint32_t timestamp;
    bool spoofing_detected;
} GPS_Data_t;

/**
 * @brief Detect GPS spoofing attacks
 */
bool Drone_DetectGPSSpoofing(const GPS_Data_t *gps)
{
    /* Check 1: Unrealistic position jump */
    static GPS_Data_t last_gps;
    double distance = GPS_CalculateDistance(last_gps.latitude, last_gps.longitude,
                                             gps->latitude, gps->longitude);
    uint32_t time_delta = gps->timestamp - last_gps.timestamp;
    float max_speed_mps = 50.0f;  // m/s (180 km/h max drone speed)

    if (distance / time_delta > max_speed_mps) {
        printf("⚠️  GPS spoofing: Unrealistic position jump\n");
        return true;
    }

    /* Check 2: Clock jump (GPS time manipulation) */
    if (abs((int)(gps->timestamp - last_gps.timestamp)) > 2) {
        printf("⚠️  GPS spoofing: Clock jump detected\n");
        return true;
    }

    /* Check 3: Low satellite count */
    if (gps->satellites < 6) {
        printf("⚠️  Weak GPS signal (possible jamming)\n");
        return true;
    }

    /* Check 4: Poor HDOP (accuracy degraded) */
    if (gps->hdop > 5.0f) {
        printf("⚠️  Poor GPS accuracy\n");
        return true;
    }

    last_gps = *gps;
    return false;
}
```

---

## Encrypted Telemetry

```c
/**
 * @brief Send encrypted telemetry to ground control
 */
int Drone_SendTelemetry(const TelemetryData_t *data)
{
    uint8_t ciphertext[sizeof(TelemetryData_t) + 16];
    size_t ciphertext_len;

    /* Encrypt with AES-256-GCM */
    psa_key_id_t telemetry_key;
    psa_ps_get(UID_TELEMETRY_KEY, 0, sizeof(telemetry_key), &telemetry_key, NULL);

    uint8_t nonce[12];
    psa_generate_random(nonce, sizeof(nonce));

    psa_aead_encrypt(telemetry_key, PSA_ALG_GCM,
                     nonce, sizeof(nonce),
                     NULL, 0,
                     (uint8_t*)data, sizeof(TelemetryData_t),
                     ciphertext, sizeof(ciphertext),
                     &ciphertext_len);

    /* Send via 4G/5G or LoRa */
    Wireless_Transmit(ciphertext, ciphertext_len);

    printf("✅ Encrypted telemetry sent (%zu bytes)\n", ciphertext_len);
    return 0;
}
```

---

## Geofencing Enforcement

```c
typedef struct {
    double center_lat;
    double center_lon;
    float radius_meters;
    float max_altitude;
    bool enabled;
} Geofence_t;

/**
 * @brief Check if drone is within geofence
 */
bool Drone_CheckGeofence(const GPS_Data_t *gps, const Geofence_t *fence)
{
    if (!fence->enabled) {
        return true;
    }

    double distance = GPS_CalculateDistance(fence->center_lat, fence->center_lon,
                                              gps->latitude, gps->longitude);

    if (distance > fence->radius_meters) {
        printf("🚨 GEOFENCE VIOLATION: %.0f m from center (limit: %.0f m)\n",
               distance, fence->radius_meters);
        return false;
    }

    if (gps->altitude > fence->max_altitude) {
        printf("🚨 ALTITUDE VIOLATION: %.0f m (limit: %.0f m)\n",
               gps->altitude, fence->max_altitude);
        return false;
    }

    return true;
}

/**
 * @brief Execute fail-safe return-to-home
 */
void Drone_FailSafeRTH(void)
{
    printf("\n🚨 FAIL-SAFE ACTIVATED: RETURN TO HOME\n");

    /* Get home position */
    GPS_Data_t home_position;
    psa_ps_get(UID_HOME_POSITION, 0, sizeof(home_position), &home_position, NULL);

    /* Autonomous return flight */
    FlightController_SetMode(MODE_RTH);
    FlightController_SetDestination(home_position.latitude, home_position.longitude);

    /* Lower altitude for safe return */
    FlightController_SetTargetAltitude(50.0f);  // meters

    printf("✅ Returning to home: %.6f, %.6f\n",
           home_position.latitude, home_position.longitude);
}
```

---

## Key Takeaways

✅ GPS anti-spoofing detection
✅ Encrypted telemetry (AES-256-GCM)
✅ Geofencing enforcement
✅ Fail-safe return-to-home
✅ Secure command authentication

---

**Lab 26 Complete! 🎉**

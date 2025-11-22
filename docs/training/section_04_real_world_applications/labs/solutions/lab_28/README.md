# Lab 28: Agriculture IoT Sensor Network

## Overview

Secure agricultural IoT network with soil sensors, weather stations, and autonomous irrigation control.

**Duration:** 150 minutes | **Difficulty:** Expert | **Prerequisites:** Labs 02-27

---

## Sensor Security

```c
typedef struct {
    float soil_moisture;      // %
    float soil_temperature;   // °C
    float soil_ph;
    float air_temperature;    // °C
    float air_humidity;       // %
    float rainfall;           // mm
} AgriSensor_Data_t;

/**
 * @brief Secure sensor data aggregation
 */
int Agri_CollectSensorData(AgriSensor_Data_t *data)
{
    /* Collect from multiple sensors */
    data->soil_moisture = Sensor_ReadSoilMoisture();
    data->air_temperature = Sensor_ReadTemperature();

    /* Validate sensor readings */
    if (data->soil_moisture < 0 || data->soil_moisture > 100) {
        printf("⚠️  Invalid soil moisture reading\n");
        return -1;
    }

    /* Sign data for integrity */
    uint8_t signature[64];
    psa_sign_hash(sensor_key, PSA_ALG_ECDSA(PSA_ALG_SHA_256),
                  (uint8_t*)data, sizeof(AgriSensor_Data_t),
                  signature, sizeof(signature), NULL);

    /* Send to cloud via LoRaWAN */
    LoRaWAN_SendSecure(data, sizeof(AgriSensor_Data_t), signature);

    printf("✅ Sensor data sent securely\n");
    return 0;
}

/**
 * @brief Autonomous irrigation decision
 */
void Agri_AutoIrrigation(const AgriSensor_Data_t *data)
{
    if (data->soil_moisture < 30.0f) {
        printf("[Irrigation] Soil dry (%.1f%%) - Starting irrigation\n",
               data->soil_moisture);
        Valve_Open(IRRIGATION_ZONE_1);
    } else if (data->soil_moisture > 70.0f) {
        printf("[Irrigation] Soil saturated (%.1f%%) - Stopping\n",
               data->soil_moisture);
        Valve_Close(IRRIGATION_ZONE_1);
    }
}
```

---

**Lab 28 Complete! 🎉**

/**
 ******************************************************************************
 * @file    lps22hh_driver.h
 * @brief   LPS22HH Pressure/Temperature Sensor Driver Header
 * @details Driver for LPS22HH barometer on X-Nucleo-IQS4A1 board
 *
 * Features:
 * - Absolute pressure measurement (260-1260 hPa)
 * - Temperature measurement (-40°C to +85°C)
 * - Altitude calculation
 * - FIFO support (up to 128 samples)
 * - Low-power mode
 *
 * Use Cases for Fitness Tracker:
 * - Altitude tracking (climbing stairs, hiking)
 * - Floor detection (indoor positioning)
 * - Weather monitoring
 * - Activity classification enhancement (ML feature)
 *
 ******************************************************************************
 */

#ifndef LPS22HH_DRIVER_H
#define LPS22HH_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "nrf_drv_twi.h"

/* ═══════════════════════════════════════════════════════════════════════════
 * Type Definitions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Status codes
 */
typedef enum {
    LPS22HH_OK                  = 0,
    LPS22HH_ERROR_COMM          = -1,
    LPS22HH_ERROR_NOT_FOUND     = -2,
    LPS22HH_ERROR_INVALID_PARAM = -3,
    LPS22HH_ERROR_TIMEOUT       = -4,
    LPS22HH_ERROR_FIFO_FULL     = -5
} LPS22HH_Status_t;

/**
 * @brief Output Data Rate (ODR)
 */
typedef enum {
    LPS22HH_ODR_ONE_SHOT = 0x00,  /* Single measurement */
    LPS22HH_ODR_1HZ      = 0x01,  /* 1 Hz */
    LPS22HH_ODR_10HZ     = 0x02,  /* 10 Hz */
    LPS22HH_ODR_25HZ     = 0x03,  /* 25 Hz */
    LPS22HH_ODR_50HZ     = 0x04,  /* 50 Hz */
    LPS22HH_ODR_75HZ     = 0x05,  /* 75 Hz */
    LPS22HH_ODR_100HZ    = 0x06,  /* 100 Hz */
    LPS22HH_ODR_200HZ    = 0x07   /* 200 Hz */
} LPS22HH_ODR_t;

/**
 * @brief Low-pass filter configuration
 */
typedef enum {
    LPS22HH_LPF_ODR_DIV_2   = 0x00,  /* ODR/2 */
    LPS22HH_LPF_ODR_DIV_9   = 0x02,  /* ODR/9 */
    LPS22HH_LPF_ODR_DIV_20  = 0x03   /* ODR/20 */
} LPS22HH_LPF_t;

/**
 * @brief Configuration structure
 */
typedef struct {
    LPS22HH_ODR_t odr;             /* Output data rate */
    LPS22HH_LPF_t lpf;             /* Low-pass filter */
    bool          low_noise_mode;  /* Low-noise mode (higher power) */
    bool          bdu_enable;      /* Block data update */
} LPS22HH_Config_t;

/**
 * @brief Pressure and temperature data
 */
typedef struct {
    float pressure_hPa;      /* Pressure in hPa (millibar) */
    float temperature_c;     /* Temperature in °C */
    float altitude_m;        /* Altitude in meters (relative to reference) */
} LPS22HH_Data_t;

/**
 * @brief Raw sensor data (24-bit pressure, 16-bit temperature)
 */
typedef struct {
    int32_t pressure_raw;    /* 24-bit pressure value */
    int16_t temperature_raw; /* 16-bit temperature value */
} LPS22HH_RawData_t;

/**
 * @brief FIFO mode
 */
typedef enum {
    LPS22HH_FIFO_BYPASS    = 0x00,  /* FIFO disabled */
    LPS22HH_FIFO_FIFO      = 0x01,  /* FIFO mode (stop when full) */
    LPS22HH_FIFO_STREAM    = 0x02,  /* Stream mode (overwrite old) */
    LPS22HH_FIFO_STREAM_TO_FIFO = 0x03  /* Stream→FIFO on trigger */
} LPS22HH_FIFOMode_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * Public API Functions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Initialize LPS22HH sensor
 * @param p_twi Pointer to initialized TWI/I2C instance
 * @param config Pointer to configuration (NULL for defaults)
 * @return LPS22HH_OK on success
 *
 * @example
 * LPS22HH_Config_t config = {
 *     .odr = LPS22HH_ODR_10HZ,
 *     .lpf = LPS22HH_LPF_ODR_DIV_9,
 *     .low_noise_mode = true,
 *     .bdu_enable = true
 * };
 * LPS22HH_Init(&m_twi, &config);
 */
LPS22HH_Status_t LPS22HH_Init(nrf_drv_twi_t const *p_twi,
                               const LPS22HH_Config_t *config);

/**
 * @brief Read raw pressure and temperature
 * @param data Pointer to output structure
 * @return LPS22HH_OK on success
 */
LPS22HH_Status_t LPS22HH_ReadRaw(LPS22HH_RawData_t *data);

/**
 * @brief Read pressure and temperature
 * @param data Pointer to output structure
 * @return LPS22HH_OK on success
 *
 * @example
 * LPS22HH_Data_t sensor;
 * LPS22HH_ReadData(&sensor);
 * printf("Pressure: %.2f hPa\n", sensor.pressure_hPa);
 * printf("Altitude: %.1f m\n", sensor.altitude_m);
 */
LPS22HH_Status_t LPS22HH_ReadData(LPS22HH_Data_t *data);

/**
 * @brief Read pressure only
 * @param pressure_hPa Output: Pressure in hPa
 * @return LPS22HH_OK on success
 */
LPS22HH_Status_t LPS22HH_ReadPressure(float *pressure_hPa);

/**
 * @brief Read temperature only
 * @param temp_c Output: Temperature in °C
 * @return LPS22HH_OK on success
 */
LPS22HH_Status_t LPS22HH_ReadTemperature(float *temp_c);

/**
 * @brief Read altitude
 * @param altitude_m Output: Altitude in meters
 * @return LPS22HH_OK on success
 *
 * @note Uses barometric formula with reference pressure
 * @note Call LPS22HH_SetReferencePressure() first for accurate results
 */
LPS22HH_Status_t LPS22HH_ReadAltitude(float *altitude_m);

/**
 * @brief Set reference pressure for altitude calculation
 * @param reference_hPa Reference pressure at sea level (default: 1013.25 hPa)
 *
 * @example
 * // At known altitude (e.g., airport elevation)
 * LPS22HH_Data_t data;
 * LPS22HH_ReadData(&data);
 * LPS22HH_SetReferencePressure(data.pressure_hPa);
 * // Now altitude readings are relative to this point
 */
void LPS22HH_SetReferencePressure(float reference_hPa);

/**
 * @brief Calculate altitude difference
 * @param pressure_hPa Current pressure
 * @param reference_hPa Reference pressure
 * @return Altitude difference in meters
 *
 * @example
 * float start_pressure = 1013.25;  // Ground level
 * float current_pressure = 1000.0; // Current reading
 * float altitude = LPS22HH_PressureToAltitude(current_pressure, start_pressure);
 * printf("Climbed %.1f meters\n", altitude);
 */
float LPS22HH_PressureToAltitude(float pressure_hPa, float reference_hPa);

/**
 * @brief Check if new data is available
 * @param pressure_ready Output: true if pressure data ready
 * @param temp_ready Output: true if temperature data ready
 * @return LPS22HH_OK on success
 */
LPS22HH_Status_t LPS22HH_IsDataReady(bool *pressure_ready, bool *temp_ready);

/**
 * @brief Configure FIFO
 * @param mode FIFO mode
 * @param watermark Watermark threshold (0-127)
 * @return LPS22HH_OK on success
 *
 * @example
 * // Enable FIFO with watermark at 64 samples
 * LPS22HH_ConfigureFIFO(LPS22HH_FIFO_STREAM, 64);
 */
LPS22HH_Status_t LPS22HH_ConfigureFIFO(LPS22HH_FIFOMode_t mode, uint8_t watermark);

/**
 * @brief Read FIFO level
 * @param level Output: Number of samples in FIFO
 * @return LPS22HH_OK on success
 */
LPS22HH_Status_t LPS22HH_GetFIFOLevel(uint8_t *level);

/**
 * @brief Read all FIFO samples
 * @param data Array to store samples
 * @param max_samples Maximum number of samples to read
 * @param samples_read Output: Actual number of samples read
 * @return LPS22HH_OK on success
 *
 * @example
 * LPS22HH_Data_t samples[64];
 * uint8_t count;
 * LPS22HH_ReadFIFO(samples, 64, &count);
 * printf("Read %d samples from FIFO\n", count);
 */
LPS22HH_Status_t LPS22HH_ReadFIFO(LPS22HH_Data_t *data, uint8_t max_samples,
                                   uint8_t *samples_read);

/**
 * @brief Enable/disable low-power mode
 * @param enable true to enable low-power mode
 * @return LPS22HH_OK on success
 *
 * @note Low-power mode: 1 Hz, higher noise
 * @note Normal mode: 10 Hz, lower noise
 */
LPS22HH_Status_t LPS22HH_SetLowPowerMode(bool enable);

/**
 * @brief Trigger single measurement (one-shot mode)
 * @return LPS22HH_OK on success
 *
 * @note Only works when ODR is set to LPS22HH_ODR_ONE_SHOT
 */
LPS22HH_Status_t LPS22HH_TriggerMeasurement(void);

/**
 * @brief Perform software reset
 * @return LPS22HH_OK on success
 */
LPS22HH_Status_t LPS22HH_SoftwareReset(void);

/* ═══════════════════════════════════════════════════════════════════════════
 * Helper Functions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Calculate floor level from altitude
 * @param altitude_m Altitude in meters
 * @param floor_height_m Typical floor height (default: 3.5m)
 * @return Floor level (0 = ground)
 *
 * @example
 * float altitude = 14.2;  // meters
 * int floor = LPS22HH_AltitudeToFloor(altitude, 3.5);
 * printf("Floor: %d\n", floor);  // Output: "Floor: 4"
 */
int LPS22HH_AltitudeToFloor(float altitude_m, float floor_height_m);

/**
 * @brief Detect stair climbing from altitude change
 * @param altitude_start Starting altitude (m)
 * @param altitude_end Ending altitude (m)
 * @param duration_sec Duration of activity (seconds)
 * @return Number of stairs climbed (negative = descending)
 *
 * @example
 * float start_alt = 10.0;
 * float end_alt = 17.5;
 * int stairs = LPS22HH_DetectStairClimbing(start_alt, end_alt, 30);
 * printf("Climbed %d stairs\n", stairs);
 */
int LPS22HH_DetectStairClimbing(float altitude_start, float altitude_end,
                                 uint32_t duration_sec);

/* ═══════════════════════════════════════════════════════════════════════════
 * Usage Examples
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * Example 1: Basic Altitude Tracking
 * ──────────────────────────────────
 * ```c
 * // Initialize
 * LPS22HH_Config_t config = {
 *     .odr = LPS22HH_ODR_10HZ,
 *     .lpf = LPS22HH_LPF_ODR_DIV_9,
 *     .low_noise_mode = true,
 *     .bdu_enable = true
 * };
 * LPS22HH_Init(&m_twi, &config);
 *
 * // Set reference at ground level
 * LPS22HH_Data_t ground;
 * LPS22HH_ReadData(&ground);
 * LPS22HH_SetReferencePressure(ground.pressure_hPa);
 *
 * // Track altitude during hike
 * while (hiking) {
 *     LPS22HH_Data_t data;
 *     LPS22HH_ReadData(&data);
 *     printf("Altitude: %.1f m\n", data.altitude_m);
 *     nrf_delay_ms(1000);
 * }
 * ```
 *
 * Example 2: Stair Climbing Detection
 * ────────────────────────────────────
 * ```c
 * float start_altitude, end_altitude;
 * uint32_t start_time, end_time;
 *
 * // Start of activity
 * LPS22HH_ReadAltitude(&start_altitude);
 * start_time = get_timestamp_ms();
 *
 * // Wait for activity to complete
 * // ...
 *
 * // End of activity
 * LPS22HH_ReadAltitude(&end_altitude);
 * end_time = get_timestamp_ms();
 *
 * // Detect stairs
 * uint32_t duration_sec = (end_time - start_time) / 1000;
 * int stairs = LPS22HH_DetectStairClimbing(start_altitude, end_altitude, duration_sec);
 *
 * if (stairs > 0) {
 *     printf("Climbed %d stairs (%.1f m)\n", stairs, end_altitude - start_altitude);
 * } else if (stairs < 0) {
 *     printf("Descended %d stairs\n", -stairs);
 * }
 * ```
 *
 * Example 3: FIFO Batch Reading
 * ──────────────────────────────
 * ```c
 * // Configure FIFO for batch reading (save power)
 * LPS22HH_ConfigureFIFO(LPS22HH_FIFO_STREAM, 64);
 *
 * // Sleep until FIFO watermark
 * while (1) {
 *     uint8_t level;
 *     LPS22HH_GetFIFOLevel(&level);
 *
 *     if (level >= 64) {
 *         // Read all samples
 *         LPS22HH_Data_t samples[64];
 *         uint8_t count;
 *         LPS22HH_ReadFIFO(samples, 64, &count);
 *
 *         // Process batch
 *         for (int i = 0; i < count; i++) {
 *             printf("Sample %d: %.2f hPa, %.1f m\n",
 *                    i, samples[i].pressure_hPa, samples[i].altitude_m);
 *         }
 *     }
 *
 *     nrf_delay_ms(100);
 * }
 * ```
 *
 * Example 4: ML Feature Extraction
 * ─────────────────────────────────
 * ```c
 * // Use altitude as ML feature for activity classification
 * typedef struct {
 *     float altitude_change;     // Δaltitude over 5 seconds
 *     float altitude_variance;   // Variance of altitude (stairs?)
 *     float pressure_hPa;        // Current pressure
 * } AltitudeFeatures_t;
 *
 * void extract_altitude_features(AltitudeFeatures_t *features)
 * {
 *     #define WINDOW_SIZE 50
 *     static float altitude_history[WINDOW_SIZE];
 *     static int idx = 0;
 *
 *     // Read current altitude
 *     float altitude;
 *     LPS22HH_ReadAltitude(&altitude);
 *
 *     // Store in history
 *     altitude_history[idx] = altitude;
 *     idx = (idx + 1) % WINDOW_SIZE;
 *
 *     // Calculate features
 *     features->altitude_change = altitude - altitude_history[0];
 *
 *     float mean = 0, variance = 0;
 *     for (int i = 0; i < WINDOW_SIZE; i++) {
 *         mean += altitude_history[i];
 *     }
 *     mean /= WINDOW_SIZE;
 *
 *     for (int i = 0; i < WINDOW_SIZE; i++) {
 *         float diff = altitude_history[i] - mean;
 *         variance += diff * diff;
 *     }
 *     variance /= WINDOW_SIZE;
 *
 *     features->altitude_variance = variance;
 *     LPS22HH_ReadPressure(&features->pressure_hPa);
 * }
 *
 * // Feed to ML model
 * AltitudeFeatures_t features;
 * extract_altitude_features(&features);
 * ml_classify_activity(&imu_data, &features);
 * ```
 */

#ifdef __cplusplus
}
#endif

#endif /* LPS22HH_DRIVER_H */

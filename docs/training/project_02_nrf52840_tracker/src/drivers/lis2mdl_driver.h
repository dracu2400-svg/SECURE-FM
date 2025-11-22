/**
 ******************************************************************************
 * @file    lis2mdl_driver.h
 * @brief   LIS2MDL 3-Axis Magnetometer Driver Header
 * @details Driver for LIS2MDL magnetometer on X-Nucleo-IQS4A1 board
 *
 * Features:
 * - 3-axis magnetic field measurement
 * - Digital compass (heading calculation)
 * - Magnetic anomaly detection
 * - Temperature compensation
 * - Self-test capability
 *
 * Use Cases for Fitness Tracker:
 * - Direction of travel (compass heading)
 * - Route mapping
 * - Navigation assistance
 * - Magnetic anomaly detection (indoor/outdoor)
 *
 ******************************************************************************
 */

#ifndef LIS2MDL_DRIVER_H
#define LIS2MDL_DRIVER_H

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
    LIS2MDL_OK                  = 0,
    LIS2MDL_ERROR_COMM          = -1,
    LIS2MDL_ERROR_NOT_FOUND     = -2,
    LIS2MDL_ERROR_INVALID_PARAM = -3,
    LIS2MDL_ERROR_TIMEOUT       = -4
} LIS2MDL_Status_t;

/**
 * @brief Output Data Rate (ODR)
 */
typedef enum {
    LIS2MDL_ODR_10HZ   = 0x00,   /* 10 Hz */
    LIS2MDL_ODR_20HZ   = 0x01,   /* 20 Hz */
    LIS2MDL_ODR_50HZ   = 0x02,   /* 50 Hz */
    LIS2MDL_ODR_100HZ  = 0x03    /* 100 Hz */
} LIS2MDL_ODR_t;

/**
 * @brief Operating Mode
 */
typedef enum {
    LIS2MDL_MODE_CONTINUOUS  = 0x00,  /* Continuous measurement */
    LIS2MDL_MODE_SINGLE      = 0x01,  /* Single measurement */
    LIS2MDL_MODE_IDLE        = 0x02   /* Power-down */
} LIS2MDL_Mode_t;

/**
 * @brief Configuration structure
 */
typedef struct {
    LIS2MDL_ODR_t  odr;                /* Output data rate */
    LIS2MDL_Mode_t mode;               /* Operating mode */
    bool           temp_comp_enable;   /* Temperature compensation */
    bool           low_pass_filter;    /* Enable low-pass filter */
} LIS2MDL_Config_t;

/**
 * @brief Magnetic field data (mGauss)
 */
typedef struct {
    float mag_x;         /* X-axis magnetic field (mGauss) */
    float mag_y;         /* Y-axis magnetic field (mGauss) */
    float mag_z;         /* Z-axis magnetic field (mGauss) */
    float magnitude;     /* Total field magnitude (mGauss) */
    float heading;       /* Compass heading (0-360°, 0=North) */
} LIS2MDL_MagData_t;

/**
 * @brief Raw magnetometer data (16-bit)
 */
typedef struct {
    int16_t mag_x;
    int16_t mag_y;
    int16_t mag_z;
} LIS2MDL_RawData_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * Public API Functions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Initialize LIS2MDL magnetometer
 * @param p_twi Pointer to initialized TWI/I2C instance
 * @param config Pointer to configuration (NULL for defaults)
 * @return LIS2MDL_OK on success
 *
 * @example
 * LIS2MDL_Config_t config = {
 *     .odr = LIS2MDL_ODR_50HZ,
 *     .mode = LIS2MDL_MODE_CONTINUOUS,
 *     .temp_comp_enable = true,
 *     .low_pass_filter = true
 * };
 * LIS2MDL_Init(&m_twi, &config);
 */
LIS2MDL_Status_t LIS2MDL_Init(nrf_drv_twi_t const *p_twi,
                               const LIS2MDL_Config_t *config);

/**
 * @brief Read raw magnetometer data
 * @param data Pointer to output structure
 * @return LIS2MDL_OK on success
 */
LIS2MDL_Status_t LIS2MDL_ReadRaw(LIS2MDL_RawData_t *data);

/**
 * @brief Read magnetometer data in mGauss
 * @param data Pointer to output structure
 * @return LIS2MDL_OK on success
 *
 * @note Also calculates heading (compass direction)
 *
 * @example
 * LIS2MDL_MagData_t mag;
 * LIS2MDL_ReadMag(&mag);
 * printf("Heading: %.1f° (%s)\n", mag.heading,
 *        get_direction_name(mag.heading));
 */
LIS2MDL_Status_t LIS2MDL_ReadMag(LIS2MDL_MagData_t *data);

/**
 * @brief Read temperature from magnetometer
 * @param temp_c Output: Temperature in °C
 * @return LIS2MDL_OK on success
 */
LIS2MDL_Status_t LIS2MDL_ReadTemperature(float *temp_c);

/**
 * @brief Perform hard-iron calibration
 * @param samples Number of samples to collect (recommend 100+)
 * @return LIS2MDL_OK on success
 *
 * @note Rotate device in figure-8 pattern during calibration
 *
 * @example
 * printf("Rotate device in figure-8 pattern...\n");
 * LIS2MDL_Calibrate(200);
 * printf("Calibration complete!\n");
 */
LIS2MDL_Status_t LIS2MDL_Calibrate(uint16_t samples);

/**
 * @brief Check if new data is available
 * @param data_ready Output: true if new data ready
 * @return LIS2MDL_OK on success
 */
LIS2MDL_Status_t LIS2MDL_IsDataReady(bool *data_ready);

/**
 * @brief Perform self-test
 * @param test_passed Output: true if self-test passed
 * @return LIS2MDL_OK on success
 */
LIS2MDL_Status_t LIS2MDL_SelfTest(bool *test_passed);

/**
 * @brief Enable/disable low-power mode
 * @param enable true to enable low-power mode
 * @return LIS2MDL_OK on success
 */
LIS2MDL_Status_t LIS2MDL_SetLowPowerMode(bool enable);

/* ═══════════════════════════════════════════════════════════════════════════
 * Helper Functions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Get cardinal direction from heading
 * @param heading Heading in degrees (0-360)
 * @return Direction name (N, NE, E, SE, S, SW, W, NW)
 *
 * @example
 * const char *dir = LIS2MDL_GetDirectionName(45.0f);
 * printf("Direction: %s\n", dir);  // "NE"
 */
const char* LIS2MDL_GetDirectionName(float heading);

/**
 * @brief Calculate heading between two points
 * @param lat1 Start latitude
 * @param lon1 Start longitude
 * @param lat2 End latitude
 * @param lon2 End longitude
 * @return Bearing in degrees (0-360)
 *
 * @example
 * // Calculate direction to destination
 * float bearing = LIS2MDL_CalculateBearing(
 *     current_lat, current_lon,
 *     dest_lat, dest_lon
 * );
 * printf("Travel direction: %.1f°\n", bearing);
 */
float LIS2MDL_CalculateBearing(float lat1, float lon1, float lat2, float lon2);

/* ═══════════════════════════════════════════════════════════════════════════
 * Usage Examples
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * Example 1: Basic Compass
 * ────────────────────────
 * ```c
 * LIS2MDL_MagData_t mag;
 * LIS2MDL_ReadMag(&mag);
 *
 * printf("Heading: %.1f° (%s)\n",
 *        mag.heading,
 *        LIS2MDL_GetDirectionName(mag.heading));
 * // Output: "Heading: 45.0° (NE)"
 * ```
 *
 * Example 2: Navigation to Destination
 * ─────────────────────────────────────
 * ```c
 * // Current location
 * float my_lat = 37.7749, my_lon = -122.4194;
 *
 * // Destination
 * float dest_lat = 37.8651, dest_lon = -119.5383;
 *
 * // Calculate required heading
 * float bearing = LIS2MDL_CalculateBearing(
 *     my_lat, my_lon, dest_lat, dest_lon);
 *
 * // Get current heading
 * LIS2MDL_MagData_t mag;
 * LIS2MDL_ReadMag(&mag);
 *
 * // Calculate error
 * float error = bearing - mag.heading;
 * if (error < -180) error += 360;
 * if (error > 180) error -= 360;
 *
 * if (fabs(error) < 10) {
 *     printf("On course!\n");
 * } else if (error > 0) {
 *     printf("Turn right %.1f°\n", error);
 * } else {
 *     printf("Turn left %.1f°\n", -error);
 * }
 * ```
 *
 * Example 3: Calibration
 * ──────────────────────
 * ```c
 * printf("Calibrating magnetometer...\n");
 * printf("Rotate device in figure-8 pattern\n");
 *
 * LIS2MDL_Calibrate(200);
 *
 * printf("Calibration complete!\n");
 * // Calibration data saved to TF-M ITS
 * ```
 *
 * Example 4: Detect Magnetic Anomalies
 * ─────────────────────────────────────
 * ```c
 * LIS2MDL_MagData_t mag;
 * LIS2MDL_ReadMag(&mag);
 *
 * // Normal Earth's magnetic field: 250-650 mGauss
 * if (mag.magnitude < 250 || mag.magnitude > 650) {
 *     printf("Magnetic anomaly detected!\n");
 *     printf("Magnitude: %.1f mGauss (expected 250-650)\n",
 *            mag.magnitude);
 *     // Likely near metal objects or indoors
 * }
 * ```
 */

#ifdef __cplusplus
}
#endif

#endif /* LIS2MDL_DRIVER_H */

/**
 ******************************************************************************
 * @file    lsm6dso_driver.h
 * @brief   LSM6DSO 6-axis IMU Driver Header
 * @details Public API for LSM6DSO accelerometer + gyroscope sensor
 *
 * Quick Start Example:
 * ────────────────────
 * ```c
 * // 1. Initialize I2C peripheral (100kHz or 400kHz)
 * I2C_HandleTypeDef hi2c1;
 * // ... I2C initialization code ...
 *
 * // 2. Configure sensor
 * LSM6DSO_Config_t config = {
 *     .accel_odr = LSM6DSO_ACCEL_ODR_104HZ,
 *     .accel_range = LSM6DSO_ACCEL_RANGE_2G,
 *     .gyro_odr = LSM6DSO_GYRO_ODR_104HZ,
 *     .gyro_range = LSM6DSO_GYRO_RANGE_250DPS,
 *     .motion_detect_enable = true,
 *     .step_counter_enable = true
 * };
 *
 * // 3. Initialize sensor
 * if (LSM6DSO_Init(&hi2c1, &config) != LSM6DSO_OK) {
 *     printf("LSM6DSO init failed!\n");
 * }
 *
 * // 4. Calibrate (device must be stationary)
 * printf("Calibrating... keep device still\n");
 * LSM6DSO_Calibrate(100);  // Average 100 samples
 *
 * // 5. Read sensor data
 * LSM6DSO_Data_t data;
 * while (1) {
 *     LSM6DSO_ReadData(&data);
 *     printf("Accel: X=%.2f Y=%.2f Z=%.2f m/s²\n",
 *            data.accel_x, data.accel_y, data.accel_z);
 *     printf("Gyro:  X=%.2f Y=%.2f Z=%.2f dps\n",
 *            data.gyro_x, data.gyro_y, data.gyro_z);
 *
 *     // Detect activity
 *     LSM6DSO_ActivityState_t activity = LSM6DSO_DetectActivity();
 *     if (activity == LSM6DSO_ACTIVITY_WALKING) {
 *         printf("User is walking\n");
 *     }
 *
 *     HAL_Delay(100);
 * }
 * ```
 *
 ******************************************************************************
 */

#ifndef LSM6DSO_DRIVER_H
#define LSM6DSO_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "stm32u5xx_hal.h"

/* ═══════════════════════════════════════════════════════════════════════════
 * Type Definitions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Status codes
 */
typedef enum {
    LSM6DSO_OK                  = 0,   /* Success */
    LSM6DSO_ERROR_COMM          = -1,  /* I2C communication error */
    LSM6DSO_ERROR_NOT_FOUND     = -2,  /* Sensor not detected */
    LSM6DSO_ERROR_INVALID_PARAM = -3,  /* Invalid parameter */
    LSM6DSO_ERROR_NOT_CALIBRATED = -4, /* Sensor not calibrated */
    LSM6DSO_ERROR_STORAGE       = -5,  /* TF-M storage error */
    LSM6DSO_ERROR_TIMEOUT       = -6   /* Operation timeout */
} LSM6DSO_Status_t;

/**
 * @brief Accelerometer Output Data Rate (ODR)
 */
typedef enum {
    LSM6DSO_ACCEL_ODR_OFF    = 0x00,  /* Power-down */
    LSM6DSO_ACCEL_ODR_12_5HZ = 0x01,  /* 12.5 Hz */
    LSM6DSO_ACCEL_ODR_26HZ   = 0x02,  /* 26 Hz */
    LSM6DSO_ACCEL_ODR_52HZ   = 0x03,  /* 52 Hz */
    LSM6DSO_ACCEL_ODR_104HZ  = 0x04,  /* 104 Hz */
    LSM6DSO_ACCEL_ODR_208HZ  = 0x05,  /* 208 Hz */
    LSM6DSO_ACCEL_ODR_416HZ  = 0x06,  /* 416 Hz */
    LSM6DSO_ACCEL_ODR_833HZ  = 0x07,  /* 833 Hz */
    LSM6DSO_ACCEL_ODR_1666HZ = 0x08,  /* 1.666 kHz */
    LSM6DSO_ACCEL_ODR_3333HZ = 0x09,  /* 3.333 kHz */
    LSM6DSO_ACCEL_ODR_6666HZ = 0x0A   /* 6.666 kHz */
} LSM6DSO_AccelODR_t;

/**
 * @brief Accelerometer Full Scale Range
 */
typedef enum {
    LSM6DSO_ACCEL_RANGE_2G  = 0x00,   /* ±2g */
    LSM6DSO_ACCEL_RANGE_4G  = 0x02,   /* ±4g */
    LSM6DSO_ACCEL_RANGE_8G  = 0x03,   /* ±8g */
    LSM6DSO_ACCEL_RANGE_16G = 0x01    /* ±16g */
} LSM6DSO_AccelRange_t;

/**
 * @brief Gyroscope Output Data Rate (ODR)
 */
typedef enum {
    LSM6DSO_GYRO_ODR_OFF    = 0x00,   /* Power-down */
    LSM6DSO_GYRO_ODR_12_5HZ = 0x01,   /* 12.5 Hz */
    LSM6DSO_GYRO_ODR_26HZ   = 0x02,   /* 26 Hz */
    LSM6DSO_GYRO_ODR_52HZ   = 0x03,   /* 52 Hz */
    LSM6DSO_GYRO_ODR_104HZ  = 0x04,   /* 104 Hz */
    LSM6DSO_GYRO_ODR_208HZ  = 0x05,   /* 208 Hz */
    LSM6DSO_GYRO_ODR_416HZ  = 0x06,   /* 416 Hz */
    LSM6DSO_GYRO_ODR_833HZ  = 0x07,   /* 833 Hz */
    LSM6DSO_GYRO_ODR_1666HZ = 0x08,   /* 1.666 kHz */
    LSM6DSO_GYRO_ODR_3333HZ = 0x09,   /* 3.333 kHz */
    LSM6DSO_GYRO_ODR_6666HZ = 0x0A    /* 6.666 kHz */
} LSM6DSO_GyroODR_t;

/**
 * @brief Gyroscope Full Scale Range
 */
typedef enum {
    LSM6DSO_GYRO_RANGE_250DPS  = 0x00, /* ±250 dps */
    LSM6DSO_GYRO_RANGE_500DPS  = 0x01, /* ±500 dps */
    LSM6DSO_GYRO_RANGE_1000DPS = 0x02, /* ±1000 dps */
    LSM6DSO_GYRO_RANGE_2000DPS = 0x03  /* ±2000 dps */
} LSM6DSO_GyroRange_t;

/**
 * @brief FIFO Mode
 */
typedef enum {
    LSM6DSO_FIFO_MODE_BYPASS       = 0x00, /* FIFO disabled */
    LSM6DSO_FIFO_MODE_FIFO         = 0x01, /* FIFO mode (stop when full) */
    LSM6DSO_FIFO_MODE_CONTINUOUS   = 0x06, /* Continuous mode (overwrite old) */
    LSM6DSO_FIFO_MODE_BYPASS_TO_FIFO = 0x05 /* Bypass → FIFO on trigger */
} LSM6DSO_FIFOMode_t;

/**
 * @brief Activity State Detection
 */
typedef enum {
    LSM6DSO_ACTIVITY_UNKNOWN     = 0,  /* Unknown/initialization */
    LSM6DSO_ACTIVITY_STATIONARY  = 1,  /* Device not moving */
    LSM6DSO_ACTIVITY_WALKING     = 2,  /* Walking detected */
    LSM6DSO_ACTIVITY_RUNNING     = 3,  /* Running/fast movement */
    LSM6DSO_ACTIVITY_VEHICLE     = 4   /* In vehicle (smooth motion) */
} LSM6DSO_ActivityState_t;

/**
 * @brief Sensor Configuration
 */
typedef struct {
    LSM6DSO_AccelODR_t   accel_odr;           /* Accelerometer ODR */
    LSM6DSO_AccelRange_t accel_range;         /* Accelerometer range */
    LSM6DSO_GyroODR_t    gyro_odr;            /* Gyroscope ODR */
    LSM6DSO_GyroRange_t  gyro_range;          /* Gyroscope range */
    LSM6DSO_FIFOMode_t   fifo_mode;           /* FIFO mode */
    uint16_t             fifo_threshold;      /* FIFO watermark level */
    bool                 motion_detect_enable; /* Enable motion detection */
    bool                 step_counter_enable;  /* Enable step counter */
    bool                 tilt_detect_enable;   /* Enable tilt detection */
} LSM6DSO_Config_t;

/**
 * @brief Raw Sensor Data (16-bit ADC values)
 */
typedef struct {
    int16_t accel_x;  /* Accelerometer X-axis (LSB) */
    int16_t accel_y;  /* Accelerometer Y-axis (LSB) */
    int16_t accel_z;  /* Accelerometer Z-axis (LSB) */
    int16_t gyro_x;   /* Gyroscope X-axis (LSB) */
    int16_t gyro_y;   /* Gyroscope Y-axis (LSB) */
    int16_t gyro_z;   /* Gyroscope Z-axis (LSB) */
} LSM6DSO_RawData_t;

/**
 * @brief Sensor Data in Physical Units
 */
typedef struct {
    float accel_x;  /* Accelerometer X-axis (m/s²) */
    float accel_y;  /* Accelerometer Y-axis (m/s²) */
    float accel_z;  /* Accelerometer Z-axis (m/s²) */
    float gyro_x;   /* Gyroscope X-axis (dps - degrees per second) */
    float gyro_y;   /* Gyroscope Y-axis (dps) */
    float gyro_z;   /* Gyroscope Z-axis (dps) */
} LSM6DSO_Data_t;

/**
 * @brief Calibration Data (stored in TF-M ITS)
 */
typedef struct {
    int16_t accel_offset_x;  /* Accelerometer X bias */
    int16_t accel_offset_y;  /* Accelerometer Y bias */
    int16_t accel_offset_z;  /* Accelerometer Z bias */
    int16_t gyro_offset_x;   /* Gyroscope X bias */
    int16_t gyro_offset_y;   /* Gyroscope Y bias */
    int16_t gyro_offset_z;   /* Gyroscope Z bias */
    bool    is_calibrated;   /* Calibration valid flag */
} LSM6DSO_Calibration_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * Public API Functions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Initialize LSM6DSO sensor
 * @param hi2c Pointer to initialized I2C handle
 * @param config Pointer to configuration (NULL for defaults)
 * @return LSM6DSO_OK on success, error code otherwise
 *
 * @example
 * LSM6DSO_Config_t config = {
 *     .accel_odr = LSM6DSO_ACCEL_ODR_104HZ,
 *     .accel_range = LSM6DSO_ACCEL_RANGE_2G,
 *     .gyro_odr = LSM6DSO_GYRO_ODR_104HZ,
 *     .gyro_range = LSM6DSO_GYRO_RANGE_250DPS
 * };
 * LSM6DSO_Init(&hi2c1, &config);
 */
LSM6DSO_Status_t LSM6DSO_Init(I2C_HandleTypeDef *hi2c, const LSM6DSO_Config_t *config);

/**
 * @brief Read raw sensor data (16-bit ADC values)
 * @param data Pointer to output structure
 * @return LSM6DSO_OK on success
 *
 * @note Raw values require sensitivity conversion. Use LSM6DSO_ReadData()
 *       for physical units (m/s², dps)
 */
LSM6DSO_Status_t LSM6DSO_ReadRaw(LSM6DSO_RawData_t *data);

/**
 * @brief Read sensor data in physical units
 * @param data Pointer to output structure
 * @return LSM6DSO_OK on success
 *
 * @example
 * LSM6DSO_Data_t data;
 * LSM6DSO_ReadData(&data);
 * printf("Accel: %.2f m/s², Gyro: %.2f dps\n", data.accel_z, data.gyro_z);
 */
LSM6DSO_Status_t LSM6DSO_ReadData(LSM6DSO_Data_t *data);

/**
 * @brief Read temperature sensor
 * @param temp_c Output: Temperature in degrees Celsius
 * @return LSM6DSO_OK on success
 */
LSM6DSO_Status_t LSM6DSO_ReadTemperature(float *temp_c);

/**
 * @brief Configure motion detection (wake-up interrupt)
 * @param threshold_mg Threshold in milligravity (e.g., 62 mg)
 * @param duration_ms Duration in milliseconds
 * @return LSM6DSO_OK on success
 *
 * @note Connect INT1 pin to GPIO with interrupt enabled
 *
 * @example
 * // Trigger interrupt when acceleration > 62mg for > 0ms
 * LSM6DSO_ConfigureMotionDetect(62, 0);
 */
LSM6DSO_Status_t LSM6DSO_ConfigureMotionDetect(uint16_t threshold_mg, uint16_t duration_ms);

/**
 * @brief Enable/disable step counter (pedometer)
 * @param enable true to enable, false to disable
 * @return LSM6DSO_OK on success
 */
LSM6DSO_Status_t LSM6DSO_EnableStepCounter(bool enable);

/**
 * @brief Read step counter value
 * @param steps Output: Number of steps counted
 * @return LSM6DSO_OK on success
 *
 * @example
 * uint16_t steps;
 * LSM6DSO_ReadStepCount(&steps);
 * printf("Steps: %u\n", steps);
 */
LSM6DSO_Status_t LSM6DSO_ReadStepCount(uint16_t *steps);

/**
 * @brief Detect current activity state
 * @return Activity state (stationary, walking, running)
 *
 * @example
 * LSM6DSO_ActivityState_t state = LSM6DSO_DetectActivity();
 * if (state == LSM6DSO_ACTIVITY_WALKING) {
 *     printf("User is walking\n");
 * }
 */
LSM6DSO_ActivityState_t LSM6DSO_DetectActivity(void);

/**
 * @brief Perform sensor calibration
 * @param samples Number of samples to average (recommend 100+)
 * @return LSM6DSO_OK on success
 *
 * @warning Device MUST be stationary on a flat surface during calibration
 *
 * @note Calibration data is saved to TF-M ITS and automatically applied
 *       to all future readings
 *
 * @example
 * printf("Place device flat and keep still...\n");
 * HAL_Delay(2000);
 * if (LSM6DSO_Calibrate(100) == LSM6DSO_OK) {
 *     printf("Calibration complete!\n");
 * }
 */
LSM6DSO_Status_t LSM6DSO_Calibrate(uint16_t samples);

/* ═══════════════════════════════════════════════════════════════════════════
 * Helper Macros
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Convert milligravity to motion detection threshold register value
 * @param mg Threshold in milligravity (mg)
 */
#define LSM6DSO_MOTION_THRESHOLD_MG(mg)  ((uint16_t)(mg))

/**
 * @brief Convert milliseconds to duration register value
 * @param ms Duration in milliseconds
 */
#define LSM6DSO_DURATION_MS(ms)          ((uint16_t)(ms))

/**
 * @brief Check if accelerometer data is ready
 */
#define LSM6DSO_IS_ACCEL_DATA_READY(status)  ((status & 0x01) != 0)

/**
 * @brief Check if gyroscope data is ready
 */
#define LSM6DSO_IS_GYRO_DATA_READY(status)   ((status & 0x02) != 0)

/* ═══════════════════════════════════════════════════════════════════════════
 * Usage Examples
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * Example 1: Basic Accelerometer Reading
 * ────────────────────────────────────────
 * ```c
 * LSM6DSO_Data_t data;
 * LSM6DSO_ReadData(&data);
 * printf("Acceleration: X=%.2f Y=%.2f Z=%.2f m/s²\n",
 *        data.accel_x, data.accel_y, data.accel_z);
 * ```
 *
 * Example 2: Motion Detection for Power Saving
 * ──────────────────────────────────────────────
 * ```c
 * // Configure wake-up on motion
 * LSM6DSO_ConfigureMotionDetect(62, 0);  // Trigger at 62mg
 *
 * // Enter low-power mode
 * __WFI();  // Wait for interrupt
 *
 * // INT1 interrupt wakes system
 * void EXTI13_IRQHandler(void) {
 *     printf("Motion detected!\n");
 *     // Process motion event
 * }
 * ```
 *
 * Example 3: Step Counter for Activity Tracking
 * ───────────────────────────────────────────────
 * ```c
 * LSM6DSO_EnableStepCounter(true);
 *
 * while (1) {
 *     uint16_t steps;
 *     LSM6DSO_ReadStepCount(&steps);
 *     printf("Steps today: %u\n", steps);
 *     HAL_Delay(60000);  // Update every minute
 * }
 * ```
 *
 * Example 4: GPS Tracker Integration
 * ────────────────────────────────────
 * ```c
 * // Check if device is moving before GPS update
 * LSM6DSO_ActivityState_t activity = LSM6DSO_DetectActivity();
 *
 * if (activity == LSM6DSO_ACTIVITY_STATIONARY) {
 *     printf("Device stationary - skip GPS update\n");
 *     gps_sleep();
 * } else {
 *     printf("Motion detected - update GPS location\n");
 *     gps_update_location();
 * }
 * ```
 *
 * Example 5: TF-M Protected Storage Integration
 * ───────────────────────────────────────────────
 * ```c
 * // Calibration data is automatically saved to TF-M ITS
 * LSM6DSO_Calibrate(100);
 *
 * // Data persists across resets
 * // Next boot automatically loads calibration from secure storage
 * ```
 */

#ifdef __cplusplus
}
#endif

#endif /* LSM6DSO_DRIVER_H */

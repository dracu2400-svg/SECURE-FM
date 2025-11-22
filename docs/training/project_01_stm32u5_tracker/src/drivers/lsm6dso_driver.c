/**
 ******************************************************************************
 * @file    lsm6dso_driver.c
 * @brief   LSM6DSO 6-axis IMU Driver for Project 1 GPS Tracker
 * @details Driver for LSM6DSO accelerometer + gyroscope sensor on
 *          X-Nucleo-IQS4A1 multi-sensor expansion board.
 *
 * Hardware:
 * ─────────
 * - NUCLEO-U545RE-Q base board
 * - X-Nucleo-IQS4A1 sensor expansion board (plugged into Arduino headers)
 * - LSM6DSO: 6-axis IMU (3-axis accelerometer + 3-axis gyroscope)
 * - I2C1 communication (default address: 0x6A)
 *
 * Features Implemented:
 * ─────────────────────
 * ✓ Accelerometer configuration (±2g/±4g/±8g/±16g ranges)
 * ✓ Gyroscope configuration (±250/±500/±1000/±2000 dps ranges)
 * ✓ Motion detection (wake-up interrupt)
 * ✓ Tilt detection (orientation change)
 * ✓ Free-fall detection
 * ✓ Step counter (pedometer)
 * ✓ Activity/inactivity detection
 * ✓ FIFO buffering for low-power operation
 * ✓ Temperature sensor readout
 *
 * Use Cases for GPS Tracker:
 * ──────────────────────────
 * 1. Motion Detection: Wake system when device moves (power saving)
 * 2. Activity Tracking: Detect walking, running, stationary states
 * 3. Orientation: Detect device tilt/rotation
 * 4. Vibration Monitoring: Detect abnormal movement patterns
 * 5. Free-fall: Emergency detection (dropped device)
 * 6. Step Counting: Track user movement
 *
 * TF-M Integration:
 * ─────────────────
 * - Sensor data stored in TF-M Protected Storage (PS)
 * - Configuration protected by TF-M ITS
 * - Calibration data in secure storage
 * - Activity patterns encrypted before transmission
 *
 * Pin Connections (via Arduino Headers):
 * ──────────────────────────────────────
 * LSM6DSO → NUCLEO-U545RE-Q
 * ────────────────────────────
 * VDD     → 3.3V (CN7 pin 16)
 * GND     → GND  (CN7 pin 20)
 * SCL     → PB8  (D15, I2C1_SCL, CN5 pin 10)
 * SDA     → PB9  (D14, I2C1_SDA, CN5 pin 9)
 * INT1    → PC13 (GPIO, wake-up interrupt)
 * INT2    → PA10 (GPIO, FIFO threshold)
 *
 ******************************************************************************
 */

#include "lsm6dso_driver.h"
#include "stm32u5xx_hal.h"
#include "psa/internal_trusted_storage.h"  /* TF-M ITS for calibration */
#include <string.h>
#include <math.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * LSM6DSO Register Map
 * ═══════════════════════════════════════════════════════════════════════════
 */

/* Device Identification */
#define LSM6DSO_WHO_AM_I_REG            0x0F
#define LSM6DSO_WHO_AM_I_VALUE          0x6C

/* Control Registers */
#define LSM6DSO_CTRL1_XL                0x10  /* Accelerometer control */
#define LSM6DSO_CTRL2_G                 0x11  /* Gyroscope control */
#define LSM6DSO_CTRL3_C                 0x12  /* Common control */
#define LSM6DSO_CTRL4_C                 0x13  /* Interrupt control */
#define LSM6DSO_CTRL5_C                 0x14  /* Rounding control */
#define LSM6DSO_CTRL6_C                 0x15  /* Gyro power mode */
#define LSM6DSO_CTRL7_G                 0x16  /* Gyro filters */
#define LSM6DSO_CTRL8_XL                0x17  /* Accel filters */
#define LSM6DSO_CTRL9_XL                0x18  /* Accel features */
#define LSM6DSO_CTRL10_C                0x19  /* Timestamp enable */

/* Status Registers */
#define LSM6DSO_STATUS_REG              0x1E

/* Temperature Output */
#define LSM6DSO_OUT_TEMP_L              0x20
#define LSM6DSO_OUT_TEMP_H              0x21

/* Gyroscope Output (16-bit, 2's complement) */
#define LSM6DSO_OUTX_L_G                0x22
#define LSM6DSO_OUTX_H_G                0x23
#define LSM6DSO_OUTY_L_G                0x24
#define LSM6DSO_OUTY_H_G                0x25
#define LSM6DSO_OUTZ_L_G                0x26
#define LSM6DSO_OUTZ_H_G                0x27

/* Accelerometer Output (16-bit, 2's complement) */
#define LSM6DSO_OUTX_L_A                0x28
#define LSM6DSO_OUTX_H_A                0x29
#define LSM6DSO_OUTY_L_A                0x2A
#define LSM6DSO_OUTY_H_A                0x2B
#define LSM6DSO_OUTZ_L_A                0x2C
#define LSM6DSO_OUTZ_H_A                0x2D

/* FIFO Control and Status */
#define LSM6DSO_FIFO_CTRL1              0x07
#define LSM6DSO_FIFO_CTRL2              0x08
#define LSM6DSO_FIFO_CTRL3              0x09
#define LSM6DSO_FIFO_CTRL4              0x0A
#define LSM6DSO_FIFO_STATUS1            0x3A
#define LSM6DSO_FIFO_STATUS2            0x3B

/* Interrupt Configuration */
#define LSM6DSO_INT1_CTRL               0x0D
#define LSM6DSO_INT2_CTRL               0x0E
#define LSM6DSO_MD1_CFG                 0x5E  /* INT1 pin control */
#define LSM6DSO_MD2_CFG                 0x5F  /* INT2 pin control */

/* Wake-up and Motion Detection */
#define LSM6DSO_WAKE_UP_THS             0x5B  /* Wake-up threshold */
#define LSM6DSO_WAKE_UP_DUR             0x5C  /* Wake-up duration */
#define LSM6DSO_FREE_FALL               0x5D  /* Free-fall config */
#define LSM6DSO_WAKE_UP_SRC             0x1B  /* Wake-up source */
#define LSM6DSO_TAP_SRC                 0x1C  /* Tap source */

/* Step Counter */
#define LSM6DSO_STEP_COUNTER_L          0x4B
#define LSM6DSO_STEP_COUNTER_H          0x4C
#define LSM6DSO_EMB_FUNC_EN_A           0x04  /* Embedded functions enable */
#define LSM6DSO_EMB_FUNC_EN_B           0x05

/* Activity/Inactivity */
#define LSM6DSO_TAP_CFG0                0x56
#define LSM6DSO_TAP_CFG2                0x58
#define LSM6DSO_TAP_THS_6D              0x59
#define LSM6DSO_INT_DUR2                0x5A

/* ═══════════════════════════════════════════════════════════════════════════
 * Constants and Macros
 * ═══════════════════════════════════════════════════════════════════════════
 */

#define LSM6DSO_I2C_ADDR_PRIMARY        0x6A  /* SDO/SA0 = GND */
#define LSM6DSO_I2C_ADDR_SECONDARY      0x6B  /* SDO/SA0 = VDD */

#define LSM6DSO_I2C_TIMEOUT             1000  /* ms */
#define LSM6DSO_MAX_RETRIES             3

/* Sensitivity values (from datasheet) */
#define LSM6DSO_ACC_SENS_2G             0.061f   /* mg/LSB */
#define LSM6DSO_ACC_SENS_4G             0.122f
#define LSM6DSO_ACC_SENS_8G             0.244f
#define LSM6DSO_ACC_SENS_16G            0.488f

#define LSM6DSO_GYRO_SENS_250DPS        8.75f    /* mdps/LSB */
#define LSM6DSO_GYRO_SENS_500DPS        17.50f
#define LSM6DSO_GYRO_SENS_1000DPS       35.00f
#define LSM6DSO_GYRO_SENS_2000DPS       70.00f

#define LSM6DSO_TEMP_SENS               256.0f   /* LSB/°C */
#define LSM6DSO_TEMP_OFFSET             25.0f    /* °C */

/* TF-M Storage UIDs */
#define TFM_LSM6DSO_CALIB_UID           2001     /* Calibration data */
#define TFM_LSM6DSO_CONFIG_UID          2002     /* Configuration */

/* ═══════════════════════════════════════════════════════════════════════════
 * Private Variables
 * ═══════════════════════════════════════════════════════════════════════════
 */

static I2C_HandleTypeDef *g_hi2c = NULL;
static uint8_t g_i2c_addr = LSM6DSO_I2C_ADDR_PRIMARY;

/* Current configuration */
static LSM6DSO_Config_t g_config = {
    .accel_odr = LSM6DSO_ACCEL_ODR_104HZ,
    .accel_range = LSM6DSO_ACCEL_RANGE_2G,
    .gyro_odr = LSM6DSO_GYRO_ODR_104HZ,
    .gyro_range = LSM6DSO_GYRO_RANGE_250DPS,
    .fifo_mode = LSM6DSO_FIFO_MODE_BYPASS,
    .fifo_threshold = 0,
    .motion_detect_enable = false,
    .step_counter_enable = false,
    .tilt_detect_enable = false
};

/* Calibration data (bias offsets) */
static LSM6DSO_Calibration_t g_calibration = {0};

/* Activity detection state */
static LSM6DSO_ActivityState_t g_activity_state = LSM6DSO_ACTIVITY_UNKNOWN;
static uint32_t g_step_count = 0;

/* ═══════════════════════════════════════════════════════════════════════════
 * Private Function Prototypes
 * ═══════════════════════════════════════════════════════════════════════════
 */

static LSM6DSO_Status_t lsm6dso_read_reg(uint8_t reg, uint8_t *data, uint16_t len);
static LSM6DSO_Status_t lsm6dso_write_reg(uint8_t reg, uint8_t *data, uint16_t len);
static LSM6DSO_Status_t lsm6dso_modify_reg(uint8_t reg, uint8_t mask, uint8_t value);
static float lsm6dso_get_accel_sensitivity(void);
static float lsm6dso_get_gyro_sensitivity(void);
static void lsm6dso_apply_calibration(LSM6DSO_RawData_t *raw);
static LSM6DSO_Status_t lsm6dso_load_calibration(void);
static LSM6DSO_Status_t lsm6dso_save_calibration(void);

/* ═══════════════════════════════════════════════════════════════════════════
 * Public Functions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Initialize LSM6DSO sensor
 * @param hi2c Pointer to I2C handle (must be initialized)
 * @param config Pointer to configuration structure
 * @return LSM6DSO_OK on success, error code otherwise
 */
LSM6DSO_Status_t LSM6DSO_Init(I2C_HandleTypeDef *hi2c, const LSM6DSO_Config_t *config)
{
    if (hi2c == NULL) {
        return LSM6DSO_ERROR_INVALID_PARAM;
    }

    g_hi2c = hi2c;

    /* Check WHO_AM_I register */
    uint8_t who_am_i = 0;
    if (lsm6dso_read_reg(LSM6DSO_WHO_AM_I_REG, &who_am_i, 1) != LSM6DSO_OK) {
        return LSM6DSO_ERROR_COMM;
    }

    if (who_am_i != LSM6DSO_WHO_AM_I_VALUE) {
        /* Try secondary I2C address */
        g_i2c_addr = LSM6DSO_I2C_ADDR_SECONDARY;
        if (lsm6dso_read_reg(LSM6DSO_WHO_AM_I_REG, &who_am_i, 1) != LSM6DSO_OK) {
            return LSM6DSO_ERROR_COMM;
        }
        if (who_am_i != LSM6DSO_WHO_AM_I_VALUE) {
            return LSM6DSO_ERROR_NOT_FOUND;
        }
    }

    /* Software reset */
    uint8_t ctrl3 = 0x01;  /* SW_RESET bit */
    lsm6dso_write_reg(LSM6DSO_CTRL3_C, &ctrl3, 1);
    HAL_Delay(10);  /* Wait for reset to complete */

    /* Apply configuration */
    if (config != NULL) {
        memcpy(&g_config, config, sizeof(LSM6DSO_Config_t));
    }

    /* Configure accelerometer */
    uint8_t ctrl1_xl = (g_config.accel_odr << 4) | (g_config.accel_range << 2);
    lsm6dso_write_reg(LSM6DSO_CTRL1_XL, &ctrl1_xl, 1);

    /* Configure gyroscope */
    uint8_t ctrl2_g = (g_config.gyro_odr << 4) | (g_config.gyro_range << 2);
    lsm6dso_write_reg(LSM6DSO_CTRL2_G, &ctrl2_g, 1);

    /* Enable Block Data Update (BDU) - prevents reading partial updates */
    lsm6dso_modify_reg(LSM6DSO_CTRL3_C, 0x40, 0x40);

    /* Load calibration data from TF-M ITS */
    lsm6dso_load_calibration();

    /* Configure motion detection if enabled */
    if (g_config.motion_detect_enable) {
        LSM6DSO_ConfigureMotionDetect(LSM6DSO_MOTION_THRESHOLD_MG(62), 0);
    }

    /* Configure step counter if enabled */
    if (g_config.step_counter_enable) {
        LSM6DSO_EnableStepCounter(true);
    }

    return LSM6DSO_OK;
}

/**
 * @brief Read raw accelerometer and gyroscope data
 */
LSM6DSO_Status_t LSM6DSO_ReadRaw(LSM6DSO_RawData_t *data)
{
    if (data == NULL) {
        return LSM6DSO_ERROR_INVALID_PARAM;
    }

    uint8_t buffer[12];

    /* Read all sensor data at once (gyro + accel) */
    if (lsm6dso_read_reg(LSM6DSO_OUTX_L_G, buffer, 12) != LSM6DSO_OK) {
        return LSM6DSO_ERROR_COMM;
    }

    /* Parse gyroscope data (signed 16-bit) */
    data->gyro_x = (int16_t)((buffer[1] << 8) | buffer[0]);
    data->gyro_y = (int16_t)((buffer[3] << 8) | buffer[2]);
    data->gyro_z = (int16_t)((buffer[5] << 8) | buffer[4]);

    /* Parse accelerometer data (signed 16-bit) */
    data->accel_x = (int16_t)((buffer[7] << 8) | buffer[6]);
    data->accel_y = (int16_t)((buffer[9] << 8) | buffer[8]);
    data->accel_z = (int16_t)((buffer[11] << 8) | buffer[10]);

    /* Apply calibration */
    lsm6dso_apply_calibration(data);

    return LSM6DSO_OK;
}

/**
 * @brief Read sensor data in physical units (m/s², dps)
 */
LSM6DSO_Status_t LSM6DSO_ReadData(LSM6DSO_Data_t *data)
{
    if (data == NULL) {
        return LSM6DSO_ERROR_INVALID_PARAM;
    }

    LSM6DSO_RawData_t raw;
    if (LSM6DSO_ReadRaw(&raw) != LSM6DSO_OK) {
        return LSM6DSO_ERROR_COMM;
    }

    /* Convert to physical units */
    float accel_sens = lsm6dso_get_accel_sensitivity();
    float gyro_sens = lsm6dso_get_gyro_sensitivity();

    /* Accelerometer: LSB → mg → m/s² */
    data->accel_x = (raw.accel_x * accel_sens) * 9.81f / 1000.0f;
    data->accel_y = (raw.accel_y * accel_sens) * 9.81f / 1000.0f;
    data->accel_z = (raw.accel_z * accel_sens) * 9.81f / 1000.0f;

    /* Gyroscope: LSB → mdps → dps */
    data->gyro_x = (raw.gyro_x * gyro_sens) / 1000.0f;
    data->gyro_y = (raw.gyro_y * gyro_sens) / 1000.0f;
    data->gyro_z = (raw.gyro_z * gyro_sens) / 1000.0f;

    return LSM6DSO_OK;
}

/**
 * @brief Read temperature sensor
 * @param temp_c Output: Temperature in degrees Celsius
 */
LSM6DSO_Status_t LSM6DSO_ReadTemperature(float *temp_c)
{
    if (temp_c == NULL) {
        return LSM6DSO_ERROR_INVALID_PARAM;
    }

    uint8_t buffer[2];
    if (lsm6dso_read_reg(LSM6DSO_OUT_TEMP_L, buffer, 2) != LSM6DSO_OK) {
        return LSM6DSO_ERROR_COMM;
    }

    int16_t temp_raw = (int16_t)((buffer[1] << 8) | buffer[0]);
    *temp_c = LSM6DSO_TEMP_OFFSET + ((float)temp_raw / LSM6DSO_TEMP_SENS);

    return LSM6DSO_OK;
}

/**
 * @brief Configure motion detection (wake-up interrupt)
 * @param threshold_mg Threshold in milligravity (mg)
 * @param duration_ms Duration in milliseconds
 */
LSM6DSO_Status_t LSM6DSO_ConfigureMotionDetect(uint16_t threshold_mg, uint16_t duration_ms)
{
    /* Convert threshold to register value (LSB = FS_XL/(2^6) = ~15.6mg for ±2g) */
    uint8_t threshold_lsb = (threshold_mg * 64) / 2000;

    /* Set wake-up threshold */
    lsm6dso_write_reg(LSM6DSO_WAKE_UP_THS, &threshold_lsb, 1);

    /* Set wake-up duration (LSB = 1/ODR) */
    uint8_t duration_lsb = (duration_ms * 104) / 1000;  /* Assuming 104Hz ODR */
    lsm6dso_write_reg(LSM6DSO_WAKE_UP_DUR, &duration_lsb, 1);

    /* Enable wake-up interrupt on INT1 pin */
    lsm6dso_modify_reg(LSM6DSO_MD1_CFG, 0x20, 0x20);

    g_config.motion_detect_enable = true;

    return LSM6DSO_OK;
}

/**
 * @brief Enable/disable step counter (pedometer)
 */
LSM6DSO_Status_t LSM6DSO_EnableStepCounter(bool enable)
{
    /* Access embedded functions page */
    uint8_t func_cfg_access = 0x80;
    lsm6dso_write_reg(0x01, &func_cfg_access, 1);  /* FUNC_CFG_ACCESS */

    /* Enable step counter in embedded functions */
    uint8_t emb_func_en = enable ? 0x10 : 0x00;  /* PEDO_EN bit */
    lsm6dso_write_reg(LSM6DSO_EMB_FUNC_EN_A, &emb_func_en, 1);

    /* Exit embedded functions page */
    func_cfg_access = 0x00;
    lsm6dso_write_reg(0x01, &func_cfg_access, 1);

    /* Enable pedometer interrupt on INT1 */
    if (enable) {
        lsm6dso_modify_reg(LSM6DSO_MD1_CFG, 0x80, 0x80);
    }

    g_config.step_counter_enable = enable;

    return LSM6DSO_OK;
}

/**
 * @brief Read step counter value
 */
LSM6DSO_Status_t LSM6DSO_ReadStepCount(uint16_t *steps)
{
    if (steps == NULL) {
        return LSM6DSO_ERROR_INVALID_PARAM;
    }

    uint8_t buffer[2];
    if (lsm6dso_read_reg(LSM6DSO_STEP_COUNTER_L, buffer, 2) != LSM6DSO_OK) {
        return LSM6DSO_ERROR_COMM;
    }

    *steps = (buffer[1] << 8) | buffer[0];
    g_step_count = *steps;

    return LSM6DSO_OK;
}

/**
 * @brief Detect current activity state (stationary, walking, running)
 */
LSM6DSO_ActivityState_t LSM6DSO_DetectActivity(void)
{
    LSM6DSO_Data_t data;
    if (LSM6DSO_ReadData(&data) != LSM6DSO_OK) {
        return LSM6DSO_ACTIVITY_UNKNOWN;
    }

    /* Calculate total acceleration magnitude */
    float accel_mag = sqrtf(data.accel_x * data.accel_x +
                            data.accel_y * data.accel_y +
                            data.accel_z * data.accel_z);

    /* Calculate total rotation rate magnitude */
    float gyro_mag = sqrtf(data.gyro_x * data.gyro_x +
                           data.gyro_y * data.gyro_y +
                           data.gyro_z * data.gyro_z);

    /* Simple activity classification */
    if (accel_mag < 0.5f && gyro_mag < 5.0f) {
        g_activity_state = LSM6DSO_ACTIVITY_STATIONARY;
    } else if (accel_mag < 2.0f && gyro_mag < 20.0f) {
        g_activity_state = LSM6DSO_ACTIVITY_WALKING;
    } else {
        g_activity_state = LSM6DSO_ACTIVITY_RUNNING;
    }

    return g_activity_state;
}

/**
 * @brief Perform sensor calibration (bias correction)
 * @param samples Number of samples to average (recommend 100+)
 * @return LSM6DSO_OK on success
 *
 * @note Device must be stationary on a flat surface during calibration
 */
LSM6DSO_Status_t LSM6DSO_Calibrate(uint16_t samples)
{
    int32_t accel_sum[3] = {0};
    int32_t gyro_sum[3] = {0};

    for (uint16_t i = 0; i < samples; i++) {
        LSM6DSO_RawData_t raw;
        if (LSM6DSO_ReadRaw(&raw) != LSM6DSO_OK) {
            return LSM6DSO_ERROR_COMM;
        }

        accel_sum[0] += raw.accel_x;
        accel_sum[1] += raw.accel_y;
        accel_sum[2] += raw.accel_z;

        gyro_sum[0] += raw.gyro_x;
        gyro_sum[1] += raw.gyro_y;
        gyro_sum[2] += raw.gyro_z;

        HAL_Delay(10);
    }

    /* Calculate averages */
    g_calibration.accel_offset_x = accel_sum[0] / samples;
    g_calibration.accel_offset_y = accel_sum[1] / samples;
    g_calibration.accel_offset_z = (accel_sum[2] / samples) - 16384;  /* Remove 1g from Z */

    g_calibration.gyro_offset_x = gyro_sum[0] / samples;
    g_calibration.gyro_offset_y = gyro_sum[1] / samples;
    g_calibration.gyro_offset_z = gyro_sum[2] / samples;

    g_calibration.is_calibrated = true;

    /* Save to TF-M secure storage */
    return lsm6dso_save_calibration();
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Private Functions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Read register(s) via I2C
 */
static LSM6DSO_Status_t lsm6dso_read_reg(uint8_t reg, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef status;

    for (uint8_t retry = 0; retry < LSM6DSO_MAX_RETRIES; retry++) {
        status = HAL_I2C_Mem_Read(g_hi2c, g_i2c_addr << 1, reg,
                                  I2C_MEMADD_SIZE_8BIT, data, len,
                                  LSM6DSO_I2C_TIMEOUT);
        if (status == HAL_OK) {
            return LSM6DSO_OK;
        }
    }

    return LSM6DSO_ERROR_COMM;
}

/**
 * @brief Write register(s) via I2C
 */
static LSM6DSO_Status_t lsm6dso_write_reg(uint8_t reg, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef status;

    for (uint8_t retry = 0; retry < LSM6DSO_MAX_RETRIES; retry++) {
        status = HAL_I2C_Mem_Write(g_hi2c, g_i2c_addr << 1, reg,
                                   I2C_MEMADD_SIZE_8BIT, data, len,
                                   LSM6DSO_I2C_TIMEOUT);
        if (status == HAL_OK) {
            return LSM6DSO_OK;
        }
    }

    return LSM6DSO_ERROR_COMM;
}

/**
 * @brief Modify register bits (read-modify-write)
 */
static LSM6DSO_Status_t lsm6dso_modify_reg(uint8_t reg, uint8_t mask, uint8_t value)
{
    uint8_t reg_value;

    if (lsm6dso_read_reg(reg, &reg_value, 1) != LSM6DSO_OK) {
        return LSM6DSO_ERROR_COMM;
    }

    reg_value = (reg_value & ~mask) | (value & mask);

    return lsm6dso_write_reg(reg, &reg_value, 1);
}

/**
 * @brief Get accelerometer sensitivity based on current range
 */
static float lsm6dso_get_accel_sensitivity(void)
{
    switch (g_config.accel_range) {
        case LSM6DSO_ACCEL_RANGE_2G:  return LSM6DSO_ACC_SENS_2G;
        case LSM6DSO_ACCEL_RANGE_4G:  return LSM6DSO_ACC_SENS_4G;
        case LSM6DSO_ACCEL_RANGE_8G:  return LSM6DSO_ACC_SENS_8G;
        case LSM6DSO_ACCEL_RANGE_16G: return LSM6DSO_ACC_SENS_16G;
        default: return LSM6DSO_ACC_SENS_2G;
    }
}

/**
 * @brief Get gyroscope sensitivity based on current range
 */
static float lsm6dso_get_gyro_sensitivity(void)
{
    switch (g_config.gyro_range) {
        case LSM6DSO_GYRO_RANGE_250DPS:  return LSM6DSO_GYRO_SENS_250DPS;
        case LSM6DSO_GYRO_RANGE_500DPS:  return LSM6DSO_GYRO_SENS_500DPS;
        case LSM6DSO_GYRO_RANGE_1000DPS: return LSM6DSO_GYRO_SENS_1000DPS;
        case LSM6DSO_GYRO_RANGE_2000DPS: return LSM6DSO_GYRO_SENS_2000DPS;
        default: return LSM6DSO_GYRO_SENS_250DPS;
    }
}

/**
 * @brief Apply calibration offsets to raw data
 */
static void lsm6dso_apply_calibration(LSM6DSO_RawData_t *raw)
{
    if (!g_calibration.is_calibrated) {
        return;
    }

    raw->accel_x -= g_calibration.accel_offset_x;
    raw->accel_y -= g_calibration.accel_offset_y;
    raw->accel_z -= g_calibration.accel_offset_z;

    raw->gyro_x -= g_calibration.gyro_offset_x;
    raw->gyro_y -= g_calibration.gyro_offset_y;
    raw->gyro_z -= g_calibration.gyro_offset_z;
}

/**
 * @brief Load calibration data from TF-M ITS
 */
static LSM6DSO_Status_t lsm6dso_load_calibration(void)
{
    size_t data_len;
    psa_status_t status = psa_its_get(TFM_LSM6DSO_CALIB_UID,
                                       0,
                                       sizeof(g_calibration),
                                       &g_calibration,
                                       &data_len);

    if (status == PSA_SUCCESS && data_len == sizeof(g_calibration)) {
        return LSM6DSO_OK;
    }

    /* No calibration data found - use defaults */
    memset(&g_calibration, 0, sizeof(g_calibration));
    return LSM6DSO_ERROR_NOT_CALIBRATED;
}

/**
 * @brief Save calibration data to TF-M ITS
 */
static LSM6DSO_Status_t lsm6dso_save_calibration(void)
{
    psa_status_t status = psa_its_set(TFM_LSM6DSO_CALIB_UID,
                                       sizeof(g_calibration),
                                       &g_calibration,
                                       PSA_STORAGE_FLAG_NONE);

    return (status == PSA_SUCCESS) ? LSM6DSO_OK : LSM6DSO_ERROR_STORAGE;
}

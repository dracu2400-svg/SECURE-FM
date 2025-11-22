/**
 ******************************************************************************
 * @file    lis2mdl_driver.c
 * @brief   LIS2MDL 3-Axis Magnetometer Driver Implementation
 * @details Complete driver for LIS2MDL on X-Nucleo-IQS4A1 board
 *
 * Features:
 * - 3-axis magnetic field measurement
 * - Digital compass (heading calculation)
 * - Hard-iron calibration
 * - Self-test capability
 * - Temperature compensation
 *
 * Hardware:
 * - Communication: I2C
 * - Address: 0x1E (7-bit)
 * - Voltage: 1.9V - 3.6V
 * - Range: ±50 gauss
 *
 * TF-M Integration:
 * - Calibration data stored in PSA ITS
 * - Secure random for self-test
 *
 ******************************************************************************
 */

#include "lis2mdl_driver.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 * Register Definitions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/* Device address (7-bit) */
#define LIS2MDL_I2C_ADDR            0x1E

/* Register addresses */
#define LIS2MDL_REG_WHO_AM_I        0x4F
#define LIS2MDL_REG_CFG_REG_A       0x60
#define LIS2MDL_REG_CFG_REG_B       0x61
#define LIS2MDL_REG_CFG_REG_C       0x62
#define LIS2MDL_REG_INT_CTRL_REG    0x63
#define LIS2MDL_REG_STATUS_REG      0x67
#define LIS2MDL_REG_OUTX_L_REG      0x68
#define LIS2MDL_REG_OUTX_H_REG      0x69
#define LIS2MDL_REG_OUTY_L_REG      0x6A
#define LIS2MDL_REG_OUTY_H_REG      0x6B
#define LIS2MDL_REG_OUTZ_L_REG      0x6C
#define LIS2MDL_REG_OUTZ_H_REG      0x6D
#define LIS2MDL_REG_TEMP_OUT_L_REG  0x6E
#define LIS2MDL_REG_TEMP_OUT_H_REG  0x6F

/* WHO_AM_I value */
#define LIS2MDL_WHO_AM_I_VALUE      0x40

/* Configuration bits */
#define LIS2MDL_CFG_A_COMP_TEMP_EN  0x80  /* Temperature compensation enable */
#define LIS2MDL_CFG_A_ODR_10HZ      0x00
#define LIS2MDL_CFG_A_ODR_20HZ      0x04
#define LIS2MDL_CFG_A_ODR_50HZ      0x08
#define LIS2MDL_CFG_A_ODR_100HZ     0x0C
#define LIS2MDL_CFG_A_MD_CONTINUOUS 0x00
#define LIS2MDL_CFG_A_MD_SINGLE     0x01
#define LIS2MDL_CFG_A_MD_IDLE       0x03

#define LIS2MDL_CFG_B_OFF_CANC      0x02  /* Offset cancellation */
#define LIS2MDL_CFG_B_LPF           0x01  /* Low-pass filter enable */

#define LIS2MDL_CFG_C_BDU           0x10  /* Block data update */
#define LIS2MDL_CFG_C_SELF_TEST     0x02  /* Self-test enable */

#define LIS2MDL_STATUS_ZYXDA        0x08  /* New data available */

/* Sensitivity: 1.5 mGauss/LSB */
#define LIS2MDL_SENSITIVITY         1.5f

/* ═══════════════════════════════════════════════════════════════════════════
 * Private Variables
 * ═══════════════════════════════════════════════════════════════════════════
 */

static nrf_drv_twi_t const *m_p_twi = NULL;

/* Calibration offsets (hard-iron correction) */
static struct {
    float offset_x;
    float offset_y;
    float offset_z;
    bool  valid;
} g_calibration = {0};

/* TF-M ITS UID for calibration data */
#define TFM_LIS2MDL_CALIB_UID  0x00000008

/* ═══════════════════════════════════════════════════════════════════════════
 * Private Function Prototypes
 * ═══════════════════════════════════════════════════════════════════════════
 */

static LIS2MDL_Status_t lis2mdl_read_reg(uint8_t reg, uint8_t *data, uint8_t len);
static LIS2MDL_Status_t lis2mdl_write_reg(uint8_t reg, uint8_t data);
static void load_calibration_from_tfm(void);
static void save_calibration_to_tfm(void);

/* ═══════════════════════════════════════════════════════════════════════════
 * I2C Communication Functions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Read register(s) from LIS2MDL
 */
static LIS2MDL_Status_t lis2mdl_read_reg(uint8_t reg, uint8_t *data, uint8_t len)
{
    ret_code_t err_code;

    /* Write register address */
    err_code = nrf_drv_twi_tx(m_p_twi, LIS2MDL_I2C_ADDR, &reg, 1, true);
    if (err_code != NRF_SUCCESS) {
        return LIS2MDL_ERROR_COMM;
    }

    /* Read data */
    err_code = nrf_drv_twi_rx(m_p_twi, LIS2MDL_I2C_ADDR, data, len);
    if (err_code != NRF_SUCCESS) {
        return LIS2MDL_ERROR_COMM;
    }

    return LIS2MDL_OK;
}

/**
 * @brief Write register to LIS2MDL
 */
static LIS2MDL_Status_t lis2mdl_write_reg(uint8_t reg, uint8_t data)
{
    uint8_t buffer[2] = {reg, data};
    ret_code_t err_code;

    err_code = nrf_drv_twi_tx(m_p_twi, LIS2MDL_I2C_ADDR, buffer, 2, false);
    if (err_code != NRF_SUCCESS) {
        return LIS2MDL_ERROR_COMM;
    }

    return LIS2MDL_OK;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * TF-M Calibration Storage
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifdef USE_TFM_STORAGE
#include "psa/internal_trusted_storage.h"

static void load_calibration_from_tfm(void)
{
    psa_status_t status;
    size_t data_len;

    status = psa_its_get(TFM_LIS2MDL_CALIB_UID, 0, sizeof(g_calibration),
                         &g_calibration, &data_len);

    if (status == PSA_SUCCESS && data_len == sizeof(g_calibration)) {
        printf("[LIS2MDL] ✓ Loaded calibration from TF-M ITS\n");
        printf("[LIS2MDL]   Offsets: X=%.2f, Y=%.2f, Z=%.2f mG\n",
               g_calibration.offset_x, g_calibration.offset_y, g_calibration.offset_z);
    } else {
        printf("[LIS2MDL] No calibration found, using defaults\n");
        g_calibration.valid = false;
    }
}

static void save_calibration_to_tfm(void)
{
    psa_status_t status;

    status = psa_its_set(TFM_LIS2MDL_CALIB_UID, sizeof(g_calibration),
                         &g_calibration, PSA_STORAGE_FLAG_NONE);

    if (status == PSA_SUCCESS) {
        printf("[LIS2MDL] ✓ Saved calibration to TF-M ITS\n");
    } else {
        printf("[LIS2MDL] ⚠️  Failed to save calibration (0x%08lX)\n", status);
    }
}

#else
/* No TF-M - use RAM only */
static void load_calibration_from_tfm(void) { /* No-op */ }
static void save_calibration_to_tfm(void) { /* No-op */ }
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 * Public API Functions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Initialize LIS2MDL magnetometer
 */
LIS2MDL_Status_t LIS2MDL_Init(nrf_drv_twi_t const *p_twi,
                               const LIS2MDL_Config_t *config)
{
    LIS2MDL_Status_t status;
    uint8_t who_am_i;

    if (p_twi == NULL) {
        return LIS2MDL_ERROR_INVALID_PARAM;
    }

    m_p_twi = p_twi;

    printf("[LIS2MDL] Initializing magnetometer...\n");

    /* Check WHO_AM_I */
    status = lis2mdl_read_reg(LIS2MDL_REG_WHO_AM_I, &who_am_i, 1);
    if (status != LIS2MDL_OK) {
        printf("[LIS2MDL] ✗ Communication failed\n");
        return LIS2MDL_ERROR_COMM;
    }

    if (who_am_i != LIS2MDL_WHO_AM_I_VALUE) {
        printf("[LIS2MDL] ✗ Device not found (WHO_AM_I=0x%02X, expected 0x40)\n", who_am_i);
        return LIS2MDL_ERROR_NOT_FOUND;
    }

    printf("[LIS2MDL] ✓ Device found (WHO_AM_I=0x%02X)\n", who_am_i);

    /* Use defaults if no config provided */
    LIS2MDL_Config_t default_config = {
        .odr = LIS2MDL_ODR_50HZ,
        .mode = LIS2MDL_MODE_CONTINUOUS,
        .temp_comp_enable = true,
        .low_pass_filter = true
    };

    const LIS2MDL_Config_t *cfg = (config != NULL) ? config : &default_config;

    /* Configure CFG_REG_A */
    uint8_t cfg_a = 0;
    if (cfg->temp_comp_enable) {
        cfg_a |= LIS2MDL_CFG_A_COMP_TEMP_EN;
    }
    cfg_a |= (cfg->odr << 2);
    cfg_a |= cfg->mode;

    status = lis2mdl_write_reg(LIS2MDL_REG_CFG_REG_A, cfg_a);
    if (status != LIS2MDL_OK) {
        return status;
    }

    /* Configure CFG_REG_B */
    uint8_t cfg_b = LIS2MDL_CFG_B_OFF_CANC;  /* Enable offset cancellation */
    if (cfg->low_pass_filter) {
        cfg_b |= LIS2MDL_CFG_B_LPF;
    }

    status = lis2mdl_write_reg(LIS2MDL_REG_CFG_REG_B, cfg_b);
    if (status != LIS2MDL_OK) {
        return status;
    }

    /* Configure CFG_REG_C */
    uint8_t cfg_c = LIS2MDL_CFG_C_BDU;  /* Block data update */
    status = lis2mdl_write_reg(LIS2MDL_REG_CFG_REG_C, cfg_c);
    if (status != LIS2MDL_OK) {
        return status;
    }

    /* Load calibration from TF-M */
    load_calibration_from_tfm();

    printf("[LIS2MDL] ✓ Initialization complete\n");
    printf("[LIS2MDL]   ODR: %d Hz\n", (cfg->odr == LIS2MDL_ODR_10HZ) ? 10 :
                                        (cfg->odr == LIS2MDL_ODR_20HZ) ? 20 :
                                        (cfg->odr == LIS2MDL_ODR_50HZ) ? 50 : 100);
    printf("[LIS2MDL]   Mode: %s\n", (cfg->mode == LIS2MDL_MODE_CONTINUOUS) ? "Continuous" :
                                      (cfg->mode == LIS2MDL_MODE_SINGLE) ? "Single-shot" : "Idle");
    printf("[LIS2MDL]   Temp compensation: %s\n", cfg->temp_comp_enable ? "ON" : "OFF");
    printf("[LIS2MDL]   Low-pass filter: %s\n", cfg->low_pass_filter ? "ON" : "OFF");

    return LIS2MDL_OK;
}

/**
 * @brief Read raw magnetometer data
 */
LIS2MDL_Status_t LIS2MDL_ReadRaw(LIS2MDL_RawData_t *data)
{
    LIS2MDL_Status_t status;
    uint8_t buffer[6];

    if (data == NULL) {
        return LIS2MDL_ERROR_INVALID_PARAM;
    }

    /* Read all 6 bytes (X, Y, Z - 16-bit each) */
    status = lis2mdl_read_reg(LIS2MDL_REG_OUTX_L_REG, buffer, 6);
    if (status != LIS2MDL_OK) {
        return status;
    }

    /* Combine low and high bytes (little-endian) */
    data->mag_x = (int16_t)((buffer[1] << 8) | buffer[0]);
    data->mag_y = (int16_t)((buffer[3] << 8) | buffer[2]);
    data->mag_z = (int16_t)((buffer[5] << 8) | buffer[4]);

    return LIS2MDL_OK;
}

/**
 * @brief Read magnetometer data in mGauss with heading
 */
LIS2MDL_Status_t LIS2MDL_ReadMag(LIS2MDL_MagData_t *data)
{
    LIS2MDL_Status_t status;
    LIS2MDL_RawData_t raw;

    if (data == NULL) {
        return LIS2MDL_ERROR_INVALID_PARAM;
    }

    /* Read raw data */
    status = LIS2MDL_ReadRaw(&raw);
    if (status != LIS2MDL_OK) {
        return status;
    }

    /* Convert to mGauss */
    data->mag_x = (float)raw.mag_x * LIS2MDL_SENSITIVITY;
    data->mag_y = (float)raw.mag_y * LIS2MDL_SENSITIVITY;
    data->mag_z = (float)raw.mag_z * LIS2MDL_SENSITIVITY;

    /* Apply hard-iron calibration */
    if (g_calibration.valid) {
        data->mag_x -= g_calibration.offset_x;
        data->mag_y -= g_calibration.offset_y;
        data->mag_z -= g_calibration.offset_z;
    }

    /* Calculate total magnitude */
    data->magnitude = sqrtf(data->mag_x * data->mag_x +
                           data->mag_y * data->mag_y +
                           data->mag_z * data->mag_z);

    /* Calculate heading (0-360°, 0=North) */
    /* Note: Assumes device is held flat (X=forward, Y=right) */
    float heading_rad = atan2f(data->mag_y, data->mag_x);
    data->heading = heading_rad * (180.0f / M_PI);

    /* Normalize to 0-360° */
    if (data->heading < 0) {
        data->heading += 360.0f;
    }

    return LIS2MDL_OK;
}

/**
 * @brief Read temperature from magnetometer
 */
LIS2MDL_Status_t LIS2MDL_ReadTemperature(float *temp_c)
{
    LIS2MDL_Status_t status;
    uint8_t buffer[2];
    int16_t temp_raw;

    if (temp_c == NULL) {
        return LIS2MDL_ERROR_INVALID_PARAM;
    }

    /* Read temperature registers */
    status = lis2mdl_read_reg(LIS2MDL_REG_TEMP_OUT_L_REG, buffer, 2);
    if (status != LIS2MDL_OK) {
        return status;
    }

    /* Combine bytes */
    temp_raw = (int16_t)((buffer[1] << 8) | buffer[0]);

    /* Convert to °C (8 LSB/°C, offset at 25°C) */
    *temp_c = 25.0f + ((float)temp_raw / 8.0f);

    return LIS2MDL_OK;
}

/**
 * @brief Perform hard-iron calibration
 */
LIS2MDL_Status_t LIS2MDL_Calibrate(uint16_t samples)
{
    LIS2MDL_Status_t status;
    float min_x = 999999.0f, max_x = -999999.0f;
    float min_y = 999999.0f, max_y = -999999.0f;
    float min_z = 999999.0f, max_z = -999999.0f;

    if (samples < 50) {
        return LIS2MDL_ERROR_INVALID_PARAM;
    }

    printf("[LIS2MDL] Starting calibration (%d samples)...\n", samples);
    printf("[LIS2MDL] Rotate device in figure-8 pattern!\n");

    /* Collect min/max values */
    for (uint16_t i = 0; i < samples; i++) {
        LIS2MDL_RawData_t raw;
        float mag_x, mag_y, mag_z;

        status = LIS2MDL_ReadRaw(&raw);
        if (status != LIS2MDL_OK) {
            return status;
        }

        mag_x = (float)raw.mag_x * LIS2MDL_SENSITIVITY;
        mag_y = (float)raw.mag_y * LIS2MDL_SENSITIVITY;
        mag_z = (float)raw.mag_z * LIS2MDL_SENSITIVITY;

        if (mag_x < min_x) min_x = mag_x;
        if (mag_x > max_x) max_x = mag_x;
        if (mag_y < min_y) min_y = mag_y;
        if (mag_y > max_y) max_y = mag_y;
        if (mag_z < min_z) min_z = mag_z;
        if (mag_z > max_z) max_z = mag_z;

        /* Progress indicator */
        if (i % (samples / 10) == 0) {
            printf("[LIS2MDL] Progress: %d%%\n", (i * 100) / samples);
        }

        nrf_delay_ms(20);  /* 50 Hz sampling */
    }

    /* Calculate offsets (midpoint between min and max) */
    g_calibration.offset_x = (min_x + max_x) / 2.0f;
    g_calibration.offset_y = (min_y + max_y) / 2.0f;
    g_calibration.offset_z = (min_z + max_z) / 2.0f;
    g_calibration.valid = true;

    printf("[LIS2MDL] ✓ Calibration complete!\n");
    printf("[LIS2MDL]   X offset: %.2f mG (range: %.2f to %.2f)\n",
           g_calibration.offset_x, min_x, max_x);
    printf("[LIS2MDL]   Y offset: %.2f mG (range: %.2f to %.2f)\n",
           g_calibration.offset_y, min_y, max_y);
    printf("[LIS2MDL]   Z offset: %.2f mG (range: %.2f to %.2f)\n",
           g_calibration.offset_z, min_z, max_z);

    /* Save to TF-M ITS */
    save_calibration_to_tfm();

    return LIS2MDL_OK;
}

/**
 * @brief Check if new data is available
 */
LIS2MDL_Status_t LIS2MDL_IsDataReady(bool *data_ready)
{
    LIS2MDL_Status_t status;
    uint8_t status_reg;

    if (data_ready == NULL) {
        return LIS2MDL_ERROR_INVALID_PARAM;
    }

    status = lis2mdl_read_reg(LIS2MDL_REG_STATUS_REG, &status_reg, 1);
    if (status != LIS2MDL_OK) {
        return status;
    }

    *data_ready = (status_reg & LIS2MDL_STATUS_ZYXDA) != 0;

    return LIS2MDL_OK;
}

/**
 * @brief Perform self-test
 */
LIS2MDL_Status_t LIS2MDL_SelfTest(bool *test_passed)
{
    LIS2MDL_Status_t status;
    LIS2MDL_RawData_t normal, self_test;
    int32_t diff_x, diff_y, diff_z;

    if (test_passed == NULL) {
        return LIS2MDL_ERROR_INVALID_PARAM;
    }

    printf("[LIS2MDL] Running self-test...\n");

    /* Read normal mode */
    status = lis2mdl_write_reg(LIS2MDL_REG_CFG_REG_C, LIS2MDL_CFG_C_BDU);
    if (status != LIS2MDL_OK) return status;
    nrf_delay_ms(100);

    status = LIS2MDL_ReadRaw(&normal);
    if (status != LIS2MDL_OK) return status;

    /* Enable self-test */
    status = lis2mdl_write_reg(LIS2MDL_REG_CFG_REG_C,
                                LIS2MDL_CFG_C_BDU | LIS2MDL_CFG_C_SELF_TEST);
    if (status != LIS2MDL_OK) return status;
    nrf_delay_ms(100);

    status = LIS2MDL_ReadRaw(&self_test);
    if (status != LIS2MDL_OK) return status;

    /* Disable self-test */
    status = lis2mdl_write_reg(LIS2MDL_REG_CFG_REG_C, LIS2MDL_CFG_C_BDU);
    if (status != LIS2MDL_OK) return status;

    /* Calculate differences */
    diff_x = abs(self_test.mag_x - normal.mag_x);
    diff_y = abs(self_test.mag_y - normal.mag_y);
    diff_z = abs(self_test.mag_z - normal.mag_z);

    printf("[LIS2MDL] Self-test differences (LSB):\n");
    printf("[LIS2MDL]   X: %ld\n", diff_x);
    printf("[LIS2MDL]   Y: %ld\n", diff_y);
    printf("[LIS2MDL]   Z: %ld\n", diff_z);

    /* Pass criteria: difference should be significant (>15 LSB) */
    *test_passed = (diff_x > 15 && diff_y > 15 && diff_z > 15);

    if (*test_passed) {
        printf("[LIS2MDL] ✓ Self-test PASSED\n");
    } else {
        printf("[LIS2MDL] ✗ Self-test FAILED\n");
    }

    return LIS2MDL_OK;
}

/**
 * @brief Enable/disable low-power mode
 */
LIS2MDL_Status_t LIS2MDL_SetLowPowerMode(bool enable)
{
    uint8_t cfg_a;
    LIS2MDL_Status_t status;

    /* Read current config */
    status = lis2mdl_read_reg(LIS2MDL_REG_CFG_REG_A, &cfg_a, 1);
    if (status != LIS2MDL_OK) {
        return status;
    }

    if (enable) {
        /* Low-power mode: 10 Hz */
        cfg_a = (cfg_a & ~0x0C) | LIS2MDL_CFG_A_ODR_10HZ;
        printf("[LIS2MDL] Low-power mode enabled (10 Hz)\n");
    } else {
        /* Normal mode: 50 Hz */
        cfg_a = (cfg_a & ~0x0C) | LIS2MDL_CFG_A_ODR_50HZ;
        printf("[LIS2MDL] Normal mode enabled (50 Hz)\n");
    }

    return lis2mdl_write_reg(LIS2MDL_REG_CFG_REG_A, cfg_a);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Helper Functions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Get cardinal direction from heading
 */
const char* LIS2MDL_GetDirectionName(float heading)
{
    /* Normalize to 0-360 */
    while (heading < 0) heading += 360.0f;
    while (heading >= 360.0f) heading -= 360.0f;

    /* Determine direction (8-point compass) */
    if (heading >= 337.5f || heading < 22.5f) {
        return "N";
    } else if (heading >= 22.5f && heading < 67.5f) {
        return "NE";
    } else if (heading >= 67.5f && heading < 112.5f) {
        return "E";
    } else if (heading >= 112.5f && heading < 157.5f) {
        return "SE";
    } else if (heading >= 157.5f && heading < 202.5f) {
        return "S";
    } else if (heading >= 202.5f && heading < 247.5f) {
        return "SW";
    } else if (heading >= 247.5f && heading < 292.5f) {
        return "W";
    } else {
        return "NW";
    }
}

/**
 * @brief Calculate bearing between two GPS points
 */
float LIS2MDL_CalculateBearing(float lat1, float lon1, float lat2, float lon2)
{
    /* Convert to radians */
    float lat1_rad = lat1 * (M_PI / 180.0f);
    float lat2_rad = lat2 * (M_PI / 180.0f);
    float lon_diff = (lon2 - lon1) * (M_PI / 180.0f);

    /* Calculate bearing using haversine formula */
    float y = sinf(lon_diff) * cosf(lat2_rad);
    float x = cosf(lat1_rad) * sinf(lat2_rad) -
              sinf(lat1_rad) * cosf(lat2_rad) * cosf(lon_diff);

    float bearing_rad = atan2f(y, x);
    float bearing_deg = bearing_rad * (180.0f / M_PI);

    /* Normalize to 0-360 */
    if (bearing_deg < 0) {
        bearing_deg += 360.0f;
    }

    return bearing_deg;
}

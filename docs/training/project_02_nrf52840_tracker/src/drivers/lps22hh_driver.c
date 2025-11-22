/**
 ******************************************************************************
 * @file    lps22hh_driver.c
 * @brief   LPS22HH Pressure/Temperature Sensor Driver Implementation
 * @details Complete driver for LPS22HH barometer on X-Nucleo-IQS4A1 board
 *
 * Hardware:
 * - Communication: I2C
 * - Address: 0x5D (7-bit)
 * - Pressure range: 260-1260 hPa
 * - Temperature range: -40°C to +85°C
 * - Pressure accuracy: ±0.025 hPa
 * - Altitude resolution: ~20 cm
 *
 * TF-M Integration:
 * - Reference pressure stored in PSA PS
 * - FIFO data can be encrypted before transmission
 *
 ******************************************************************************
 */

#include "lps22hh_driver.h"
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
#define LPS22HH_I2C_ADDR            0x5D

/* Register addresses */
#define LPS22HH_REG_WHO_AM_I        0x0F
#define LPS22HH_REG_CTRL_REG1       0x10
#define LPS22HH_REG_CTRL_REG2       0x11
#define LPS22HH_REG_CTRL_REG3       0x12
#define LPS22HH_REG_FIFO_CTRL       0x13
#define LPS22HH_REG_FIFO_WTM        0x14
#define LPS22HH_REG_REF_P_L         0x15
#define LPS22HH_REG_REF_P_H         0x16
#define LPS22HH_REG_INT_SOURCE      0x24
#define LPS22HH_REG_FIFO_STATUS1    0x25
#define LPS22HH_REG_FIFO_STATUS2    0x26
#define LPS22HH_REG_STATUS          0x27
#define LPS22HH_REG_PRESS_OUT_XL    0x28
#define LPS22HH_REG_PRESS_OUT_L     0x29
#define LPS22HH_REG_PRESS_OUT_H     0x2A
#define LPS22HH_REG_TEMP_OUT_L      0x2B
#define LPS22HH_REG_TEMP_OUT_H      0x2C
#define LPS22HH_REG_FIFO_DATA_OUT_PRESS_XL  0x78
#define LPS22HH_REG_FIFO_DATA_OUT_PRESS_L   0x79
#define LPS22HH_REG_FIFO_DATA_OUT_PRESS_H   0x7A

/* WHO_AM_I value */
#define LPS22HH_WHO_AM_I_VALUE      0xB3

/* CTRL_REG1 bits */
#define LPS22HH_CTRL1_ODR_MASK      0x70
#define LPS22HH_CTRL1_ODR_SHIFT     4
#define LPS22HH_CTRL1_EN_LPFP       0x08
#define LPS22HH_CTRL1_LPFP_CFG_MASK 0x06
#define LPS22HH_CTRL1_BDU           0x02

/* CTRL_REG2 bits */
#define LPS22HH_CTRL2_BOOT          0x80
#define LPS22HH_CTRL2_SWRESET       0x04
#define LPS22HH_CTRL2_ONE_SHOT      0x01
#define LPS22HH_CTRL2_LOW_NOISE_EN  0x02

/* CTRL_REG3 bits */
#define LPS22HH_CTRL3_DRDY          0x04

/* FIFO_CTRL bits */
#define LPS22HH_FIFO_MODE_MASK      0xE0
#define LPS22HH_FIFO_MODE_SHIFT     5
#define LPS22HH_FIFO_STOP_ON_WTM    0x08

/* STATUS bits */
#define LPS22HH_STATUS_P_DA         0x01  /* Pressure data available */
#define LPS22HH_STATUS_T_DA         0x02  /* Temperature data available */

/* ═══════════════════════════════════════════════════════════════════════════
 * Constants
 * ═══════════════════════════════════════════════════════════════════════════
 */

/* Pressure sensitivity: 4096 LSB/hPa */
#define LPS22HH_PRESSURE_SENSITIVITY    4096.0f

/* Temperature sensitivity: 100 LSB/°C */
#define LPS22HH_TEMPERATURE_SENSITIVITY 100.0f

/* Standard atmosphere at sea level (hPa) */
#define STANDARD_PRESSURE_HPA           1013.25f

/* Altitude calculation constant */
#define ALTITUDE_CONSTANT               44330.0f
#define PRESSURE_EXPONENT               0.1903f

/* Typical stair height (meters) */
#define TYPICAL_STAIR_HEIGHT            0.18f

/* Typical floor height (meters) */
#define TYPICAL_FLOOR_HEIGHT            3.5f

/* ═══════════════════════════════════════════════════════════════════════════
 * Private Variables
 * ═══════════════════════════════════════════════════════════════════════════
 */

static nrf_drv_twi_t const *m_p_twi = NULL;

/* Reference pressure for altitude calculation */
static float g_reference_pressure_hPa = STANDARD_PRESSURE_HPA;

/* TF-M PS UID for reference pressure */
#define TFM_LPS22HH_REFPRESSURE_UID  0x00000009

/* ═══════════════════════════════════════════════════════════════════════════
 * Private Function Prototypes
 * ═══════════════════════════════════════════════════════════════════════════
 */

static LPS22HH_Status_t lps22hh_read_reg(uint8_t reg, uint8_t *data, uint8_t len);
static LPS22HH_Status_t lps22hh_write_reg(uint8_t reg, uint8_t data);
static void load_reference_pressure_from_tfm(void);
static void save_reference_pressure_to_tfm(void);

/* ═══════════════════════════════════════════════════════════════════════════
 * I2C Communication Functions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Read register(s) from LPS22HH
 */
static LPS22HH_Status_t lps22hh_read_reg(uint8_t reg, uint8_t *data, uint8_t len)
{
    ret_code_t err_code;

    /* Write register address */
    err_code = nrf_drv_twi_tx(m_p_twi, LPS22HH_I2C_ADDR, &reg, 1, true);
    if (err_code != NRF_SUCCESS) {
        return LPS22HH_ERROR_COMM;
    }

    /* Read data */
    err_code = nrf_drv_twi_rx(m_p_twi, LPS22HH_I2C_ADDR, data, len);
    if (err_code != NRF_SUCCESS) {
        return LPS22HH_ERROR_COMM;
    }

    return LPS22HH_OK;
}

/**
 * @brief Write register to LPS22HH
 */
static LPS22HH_Status_t lps22hh_write_reg(uint8_t reg, uint8_t data)
{
    uint8_t buffer[2] = {reg, data};
    ret_code_t err_code;

    err_code = nrf_drv_twi_tx(m_p_twi, LPS22HH_I2C_ADDR, buffer, 2, false);
    if (err_code != NRF_SUCCESS) {
        return LPS22HH_ERROR_COMM;
    }

    return LPS22HH_OK;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * TF-M Reference Pressure Storage
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifdef USE_TFM_STORAGE
#include "psa/protected_storage.h"

static void load_reference_pressure_from_tfm(void)
{
    psa_status_t status;
    size_t data_len;

    status = psa_ps_get(TFM_LPS22HH_REFPRESSURE_UID, 0, sizeof(g_reference_pressure_hPa),
                        &g_reference_pressure_hPa, &data_len);

    if (status == PSA_SUCCESS && data_len == sizeof(g_reference_pressure_hPa)) {
        printf("[LPS22HH] ✓ Loaded reference pressure from TF-M PS: %.2f hPa\n",
               g_reference_pressure_hPa);
    } else {
        printf("[LPS22HH] Using default reference pressure: %.2f hPa\n",
               STANDARD_PRESSURE_HPA);
        g_reference_pressure_hPa = STANDARD_PRESSURE_HPA;
    }
}

static void save_reference_pressure_to_tfm(void)
{
    psa_status_t status;

    status = psa_ps_set(TFM_LPS22HH_REFPRESSURE_UID, sizeof(g_reference_pressure_hPa),
                        &g_reference_pressure_hPa, PSA_STORAGE_FLAG_NONE);

    if (status == PSA_SUCCESS) {
        printf("[LPS22HH] ✓ Saved reference pressure to TF-M PS\n");
    } else {
        printf("[LPS22HH] ⚠️  Failed to save reference pressure (0x%08lX)\n", status);
    }
}

#else
/* No TF-M - use RAM only */
static void load_reference_pressure_from_tfm(void) { /* No-op */ }
static void save_reference_pressure_to_tfm(void) { /* No-op */ }
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 * Public API Functions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Initialize LPS22HH sensor
 */
LPS22HH_Status_t LPS22HH_Init(nrf_drv_twi_t const *p_twi,
                               const LPS22HH_Config_t *config)
{
    LPS22HH_Status_t status;
    uint8_t who_am_i;

    if (p_twi == NULL) {
        return LPS22HH_ERROR_INVALID_PARAM;
    }

    m_p_twi = p_twi;

    printf("[LPS22HH] Initializing barometer...\n");

    /* Check WHO_AM_I */
    status = lps22hh_read_reg(LPS22HH_REG_WHO_AM_I, &who_am_i, 1);
    if (status != LPS22HH_OK) {
        printf("[LPS22HH] ✗ Communication failed\n");
        return LPS22HH_ERROR_COMM;
    }

    if (who_am_i != LPS22HH_WHO_AM_I_VALUE) {
        printf("[LPS22HH] ✗ Device not found (WHO_AM_I=0x%02X, expected 0xB3)\n", who_am_i);
        return LPS22HH_ERROR_NOT_FOUND;
    }

    printf("[LPS22HH] ✓ Device found (WHO_AM_I=0x%02X)\n", who_am_i);

    /* Software reset */
    status = lps22hh_write_reg(LPS22HH_REG_CTRL_REG2, LPS22HH_CTRL2_SWRESET);
    if (status != LPS22HH_OK) return status;
    nrf_delay_ms(10);

    /* Use defaults if no config provided */
    LPS22HH_Config_t default_config = {
        .odr = LPS22HH_ODR_10HZ,
        .lpf = LPS22HH_LPF_ODR_DIV_9,
        .low_noise_mode = true,
        .bdu_enable = true
    };

    const LPS22HH_Config_t *cfg = (config != NULL) ? config : &default_config;

    /* Configure CTRL_REG1 */
    uint8_t ctrl1 = 0;
    ctrl1 |= (cfg->odr << LPS22HH_CTRL1_ODR_SHIFT);
    ctrl1 |= LPS22HH_CTRL1_EN_LPFP;  /* Enable low-pass filter */
    ctrl1 |= (cfg->lpf & 0x03) << 1;
    if (cfg->bdu_enable) {
        ctrl1 |= LPS22HH_CTRL1_BDU;
    }

    status = lps22hh_write_reg(LPS22HH_REG_CTRL_REG1, ctrl1);
    if (status != LPS22HH_OK) return status;

    /* Configure CTRL_REG2 */
    uint8_t ctrl2 = 0;
    if (cfg->low_noise_mode) {
        ctrl2 |= LPS22HH_CTRL2_LOW_NOISE_EN;
    }

    status = lps22hh_write_reg(LPS22HH_REG_CTRL_REG2, ctrl2);
    if (status != LPS22HH_OK) return status;

    /* Load reference pressure from TF-M */
    load_reference_pressure_from_tfm();

    printf("[LPS22HH] ✓ Initialization complete\n");
    printf("[LPS22HH]   ODR: ");
    switch (cfg->odr) {
        case LPS22HH_ODR_ONE_SHOT: printf("One-shot\n"); break;
        case LPS22HH_ODR_1HZ:  printf("1 Hz\n"); break;
        case LPS22HH_ODR_10HZ: printf("10 Hz\n"); break;
        case LPS22HH_ODR_25HZ: printf("25 Hz\n"); break;
        case LPS22HH_ODR_50HZ: printf("50 Hz\n"); break;
        case LPS22HH_ODR_75HZ: printf("75 Hz\n"); break;
        case LPS22HH_ODR_100HZ: printf("100 Hz\n"); break;
        case LPS22HH_ODR_200HZ: printf("200 Hz\n"); break;
    }
    printf("[LPS22HH]   Low-noise mode: %s\n", cfg->low_noise_mode ? "ON" : "OFF");
    printf("[LPS22HH]   BDU: %s\n", cfg->bdu_enable ? "ON" : "OFF");
    printf("[LPS22HH]   Reference pressure: %.2f hPa\n", g_reference_pressure_hPa);

    return LPS22HH_OK;
}

/**
 * @brief Read raw pressure and temperature
 */
LPS22HH_Status_t LPS22HH_ReadRaw(LPS22HH_RawData_t *data)
{
    LPS22HH_Status_t status;
    uint8_t buffer[5];

    if (data == NULL) {
        return LPS22HH_ERROR_INVALID_PARAM;
    }

    /* Read pressure (24-bit) and temperature (16-bit) */
    status = lps22hh_read_reg(LPS22HH_REG_PRESS_OUT_XL, buffer, 5);
    if (status != LPS22HH_OK) {
        return status;
    }

    /* Combine bytes (little-endian) */
    data->pressure_raw = (int32_t)((buffer[2] << 16) | (buffer[1] << 8) | buffer[0]);

    /* Sign extend 24-bit to 32-bit */
    if (data->pressure_raw & 0x800000) {
        data->pressure_raw |= 0xFF000000;
    }

    data->temperature_raw = (int16_t)((buffer[4] << 8) | buffer[3]);

    return LPS22HH_OK;
}

/**
 * @brief Read pressure and temperature
 */
LPS22HH_Status_t LPS22HH_ReadData(LPS22HH_Data_t *data)
{
    LPS22HH_Status_t status;
    LPS22HH_RawData_t raw;

    if (data == NULL) {
        return LPS22HH_ERROR_INVALID_PARAM;
    }

    /* Read raw data */
    status = LPS22HH_ReadRaw(&raw);
    if (status != LPS22HH_OK) {
        return status;
    }

    /* Convert to hPa */
    data->pressure_hPa = (float)raw.pressure_raw / LPS22HH_PRESSURE_SENSITIVITY;

    /* Convert to °C */
    data->temperature_c = (float)raw.temperature_raw / LPS22HH_TEMPERATURE_SENSITIVITY;

    /* Calculate altitude */
    data->altitude_m = LPS22HH_PressureToAltitude(data->pressure_hPa, g_reference_pressure_hPa);

    return LPS22HH_OK;
}

/**
 * @brief Read pressure only
 */
LPS22HH_Status_t LPS22HH_ReadPressure(float *pressure_hPa)
{
    LPS22HH_Data_t data;
    LPS22HH_Status_t status;

    status = LPS22HH_ReadData(&data);
    if (status != LPS22HH_OK) {
        return status;
    }

    *pressure_hPa = data.pressure_hPa;
    return LPS22HH_OK;
}

/**
 * @brief Read temperature only
 */
LPS22HH_Status_t LPS22HH_ReadTemperature(float *temp_c)
{
    LPS22HH_Data_t data;
    LPS22HH_Status_t status;

    status = LPS22HH_ReadData(&data);
    if (status != LPS22HH_OK) {
        return status;
    }

    *temp_c = data.temperature_c;
    return LPS22HH_OK;
}

/**
 * @brief Read altitude
 */
LPS22HH_Status_t LPS22HH_ReadAltitude(float *altitude_m)
{
    LPS22HH_Data_t data;
    LPS22HH_Status_t status;

    status = LPS22HH_ReadData(&data);
    if (status != LPS22HH_OK) {
        return status;
    }

    *altitude_m = data.altitude_m;
    return LPS22HH_OK;
}

/**
 * @brief Set reference pressure for altitude calculation
 */
void LPS22HH_SetReferencePressure(float reference_hPa)
{
    g_reference_pressure_hPa = reference_hPa;
    save_reference_pressure_to_tfm();
    printf("[LPS22HH] Reference pressure set to %.2f hPa\n", reference_hPa);
}

/**
 * @brief Calculate altitude from pressure
 */
float LPS22HH_PressureToAltitude(float pressure_hPa, float reference_hPa)
{
    /* Barometric formula:
     * altitude = 44330 * (1 - (P/P0)^0.1903)
     * where P = current pressure, P0 = reference pressure
     */
    float ratio = pressure_hPa / reference_hPa;
    float altitude = ALTITUDE_CONSTANT * (1.0f - powf(ratio, PRESSURE_EXPONENT));
    return altitude;
}

/**
 * @brief Check if new data is available
 */
LPS22HH_Status_t LPS22HH_IsDataReady(bool *pressure_ready, bool *temp_ready)
{
    LPS22HH_Status_t status;
    uint8_t status_reg;

    if (pressure_ready == NULL || temp_ready == NULL) {
        return LPS22HH_ERROR_INVALID_PARAM;
    }

    status = lps22hh_read_reg(LPS22HH_REG_STATUS, &status_reg, 1);
    if (status != LPS22HH_OK) {
        return status;
    }

    *pressure_ready = (status_reg & LPS22HH_STATUS_P_DA) != 0;
    *temp_ready = (status_reg & LPS22HH_STATUS_T_DA) != 0;

    return LPS22HH_OK;
}

/**
 * @brief Configure FIFO
 */
LPS22HH_Status_t LPS22HH_ConfigureFIFO(LPS22HH_FIFOMode_t mode, uint8_t watermark)
{
    LPS22HH_Status_t status;
    uint8_t fifo_ctrl;

    if (watermark > 127) {
        return LPS22HH_ERROR_INVALID_PARAM;
    }

    /* Set FIFO mode */
    fifo_ctrl = (mode << LPS22HH_FIFO_MODE_SHIFT);
    status = lps22hh_write_reg(LPS22HH_REG_FIFO_CTRL, fifo_ctrl);
    if (status != LPS22HH_OK) return status;

    /* Set watermark threshold */
    status = lps22hh_write_reg(LPS22HH_REG_FIFO_WTM, watermark);
    if (status != LPS22HH_OK) return status;

    printf("[LPS22HH] FIFO configured: mode=%d, watermark=%d\n", mode, watermark);

    return LPS22HH_OK;
}

/**
 * @brief Read FIFO level
 */
LPS22HH_Status_t LPS22HH_GetFIFOLevel(uint8_t *level)
{
    LPS22HH_Status_t status;
    uint8_t fifo_status;

    if (level == NULL) {
        return LPS22HH_ERROR_INVALID_PARAM;
    }

    status = lps22hh_read_reg(LPS22HH_REG_FIFO_STATUS1, &fifo_status, 1);
    if (status != LPS22HH_OK) {
        return status;
    }

    *level = fifo_status & 0x7F;  /* Bits 6:0 = FIFO level */

    return LPS22HH_OK;
}

/**
 * @brief Read all FIFO samples
 */
LPS22HH_Status_t LPS22HH_ReadFIFO(LPS22HH_Data_t *data, uint8_t max_samples,
                                   uint8_t *samples_read)
{
    LPS22HH_Status_t status;
    uint8_t fifo_level;

    if (data == NULL || samples_read == NULL) {
        return LPS22HH_ERROR_INVALID_PARAM;
    }

    /* Get FIFO level */
    status = LPS22HH_GetFIFOLevel(&fifo_level);
    if (status != LPS22HH_OK) {
        return status;
    }

    /* Read samples */
    uint8_t count = (fifo_level < max_samples) ? fifo_level : max_samples;
    for (uint8_t i = 0; i < count; i++) {
        uint8_t buffer[3];

        /* Read pressure from FIFO */
        status = lps22hh_read_reg(LPS22HH_REG_FIFO_DATA_OUT_PRESS_XL, buffer, 3);
        if (status != LPS22HH_OK) {
            *samples_read = i;
            return status;
        }

        /* Convert to hPa */
        int32_t pressure_raw = (int32_t)((buffer[2] << 16) | (buffer[1] << 8) | buffer[0]);
        if (pressure_raw & 0x800000) {
            pressure_raw |= 0xFF000000;
        }

        data[i].pressure_hPa = (float)pressure_raw / LPS22HH_PRESSURE_SENSITIVITY;
        data[i].altitude_m = LPS22HH_PressureToAltitude(data[i].pressure_hPa, g_reference_pressure_hPa);
        data[i].temperature_c = 0.0f;  /* Temperature not available in FIFO */
    }

    *samples_read = count;
    return LPS22HH_OK;
}

/**
 * @brief Enable/disable low-power mode
 */
LPS22HH_Status_t LPS22HH_SetLowPowerMode(bool enable)
{
    uint8_t ctrl1;
    LPS22HH_Status_t status;

    /* Read current config */
    status = lps22hh_read_reg(LPS22HH_REG_CTRL_REG1, &ctrl1, 1);
    if (status != LPS22HH_OK) {
        return status;
    }

    if (enable) {
        /* Low-power mode: 1 Hz */
        ctrl1 = (ctrl1 & ~LPS22HH_CTRL1_ODR_MASK) | (LPS22HH_ODR_1HZ << LPS22HH_CTRL1_ODR_SHIFT);
        printf("[LPS22HH] Low-power mode enabled (1 Hz)\n");
    } else {
        /* Normal mode: 10 Hz */
        ctrl1 = (ctrl1 & ~LPS22HH_CTRL1_ODR_MASK) | (LPS22HH_ODR_10HZ << LPS22HH_CTRL1_ODR_SHIFT);
        printf("[LPS22HH] Normal mode enabled (10 Hz)\n");
    }

    return lps22hh_write_reg(LPS22HH_REG_CTRL_REG1, ctrl1);
}

/**
 * @brief Trigger single measurement
 */
LPS22HH_Status_t LPS22HH_TriggerMeasurement(void)
{
    return lps22hh_write_reg(LPS22HH_REG_CTRL_REG2, LPS22HH_CTRL2_ONE_SHOT);
}

/**
 * @brief Perform software reset
 */
LPS22HH_Status_t LPS22HH_SoftwareReset(void)
{
    LPS22HH_Status_t status;

    status = lps22hh_write_reg(LPS22HH_REG_CTRL_REG2, LPS22HH_CTRL2_SWRESET);
    if (status != LPS22HH_OK) {
        return status;
    }

    nrf_delay_ms(10);
    printf("[LPS22HH] Software reset complete\n");

    return LPS22HH_OK;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Helper Functions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Calculate floor level from altitude
 */
int LPS22HH_AltitudeToFloor(float altitude_m, float floor_height_m)
{
    if (floor_height_m <= 0) {
        floor_height_m = TYPICAL_FLOOR_HEIGHT;
    }

    /* Round to nearest floor */
    int floor = (int)((altitude_m + floor_height_m / 2.0f) / floor_height_m);

    return floor;
}

/**
 * @brief Detect stair climbing from altitude change
 */
int LPS22HH_DetectStairClimbing(float altitude_start, float altitude_end,
                                 uint32_t duration_sec)
{
    float altitude_change = altitude_end - altitude_start;

    /* Threshold: minimum 0.5m change to be considered stairs */
    if (fabsf(altitude_change) < 0.5f) {
        return 0;
    }

    /* Calculate number of stairs (typical stair height: 18 cm) */
    int stairs = (int)(altitude_change / TYPICAL_STAIR_HEIGHT);

    /* Sanity check: typical stair climbing speed is 1 stair/second */
    int max_stairs = duration_sec;  /* Maximum possible stairs */
    if (abs(stairs) > max_stairs) {
        stairs = (stairs > 0) ? max_stairs : -max_stairs;
    }

    return stairs;
}

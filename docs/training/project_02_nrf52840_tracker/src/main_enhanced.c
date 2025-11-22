/**
 ******************************************************************************
 * @file    main_enhanced.c
 * @brief   Enhanced NRF52840 ML Activity Tracker - Main Application
 * @details Complete IoT fitness tracker with GPS, multi-sensor fusion, and TF-M security
 *
 * Hardware:
 * - NRF52840-DK development board
 * - X-Nucleo-IQS4A1 multi-sensor board (I2C)
 * - SimCom A7672SA GPS/4G module (UART)
 *
 * Features:
 * - 7 activity types (walking, running, cycling, driving, stairs, hiking, idle)
 * - GPS location tracking + 4G cloud upload
 * - Fall detection with GPS alert
 * - Compass heading and navigation
 * - Altitude tracking (floor and stair detection)
 * - Indoor/outdoor detection
 * - Calorie calculation
 * - TF-M secure storage
 * - BLE mobile app connectivity
 *
 * Author: TF-M Training Package
 * Date: 2025-11-22
 *
 ******************************************************************************
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* NRF SDK */
#include "nrf_drv_twi.h"
#include "nrf_drv_uart.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "boards.h"
#include "app_error.h"
#include "app_timer.h"
#include "ble_stack_init.h"

/* Sensor drivers */
#include "lsm6dso_driver.h"
#include "lis2mdl_driver.h"
#include "lps22hh_driver.h"
#include "simcom_a7672sa_driver.h"

/* Sensor fusion */
#include "sensor_fusion.h"

/* TF-M (if using secure partition) */
#ifdef USE_TFM
#include "psa/crypto.h"
#include "psa/internal_trusted_storage.h"
#include "psa/protected_storage.h"
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 * Configuration
 * ═══════════════════════════════════════════════════════════════════════════
 */

/* Device identity */
#define DEVICE_ID           "TRACKER-001"
#define DEVICE_NAME         "Enhanced ML Tracker"

/* Update intervals */
#define SENSOR_UPDATE_MS    100    /* 10 Hz sensor fusion */
#define GPS_UPDATE_MS       5000   /* 5 seconds GPS update */
#define CLOUD_UPLOAD_MS     60000  /* 1 minute cloud upload */
#define DISPLAY_UPDATE_MS   1000   /* 1 second display update */

/* Cloud API */
#define CLOUD_API_URL       "https://api.fitness.com/activity"

/* Emergency contact */
#define EMERGENCY_PHONE     "+1234567890"

/* User profile (for calorie calculation) */
#define USER_WEIGHT_KG      70.0f

/* ═══════════════════════════════════════════════════════════════════════════
 * Hardware Configuration
 * ═══════════════════════════════════════════════════════════════════════════
 */

/* I2C pins (for X-Nucleo-IQS4A1) */
#define TWI_SCL_PIN         27
#define TWI_SDA_PIN         26

/* UART pins (for SimCom A7672SA) */
#define UART_TX_PIN         6
#define UART_RX_PIN         8

/* LEDs */
#define LED_ACTIVITY        BSP_LED_0  /* Green: Activity indicator */
#define LED_GPS             BSP_LED_1  /* Blue: GPS fix indicator */
#define LED_FALL            BSP_LED_2  /* Red: Fall detected */
#define LED_CLOUD           BSP_LED_3  /* Yellow: Cloud upload */

/* ═══════════════════════════════════════════════════════════════════════════
 * Global Variables
 * ═══════════════════════════════════════════════════════════════════════════
 */

/* I2C/TWI instance */
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(0);

/* UART instance */
static const nrf_drv_uart_t m_uart = NRF_DRV_UART_INSTANCE(0);

/* Timers */
APP_TIMER_DEF(m_sensor_timer);
APP_TIMER_DEF(m_gps_timer);
APP_TIMER_DEF(m_cloud_timer);
APP_TIMER_DEF(m_display_timer);

/* State */
static FusedSensorData_t g_fused_data;
static bool g_fall_alert_sent = false;

/* ═══════════════════════════════════════════════════════════════════════════
 * Function Prototypes
 * ═══════════════════════════════════════════════════════════════════════════
 */

static void hardware_init(void);
static void sensors_init(void);
static void timers_init(void);
static void ble_init(void);
static void tfm_init(void);

static void sensor_update_handler(void *p_context);
static void gps_update_handler(void *p_context);
static void cloud_upload_handler(void *p_context);
static void display_update_handler(void *p_context);

static void handle_fall_detection(const FusedSensorData_t *data);
static void handle_cloud_upload(const FusedSensorData_t *data);
static void display_status(const FusedSensorData_t *data);
static void update_leds(const FusedSensorData_t *data);

/* ═══════════════════════════════════════════════════════════════════════════
 * Main Application
 * ═══════════════════════════════════════════════════════════════════════════
 */

int main(void)
{
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  Enhanced ML Activity Tracker with GPS\n");
    printf("  Device: %s\n", DEVICE_NAME);
    printf("  ID: %s\n", DEVICE_ID);
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("\n");

    /* Initialize hardware */
    hardware_init();

    /* Initialize sensors */
    sensors_init();

    /* Initialize timers */
    timers_init();

    /* Initialize BLE */
    ble_init();

    /* Initialize TF-M (if available) */
    tfm_init();

    printf("[MAIN] ✓ All systems initialized\n");
    printf("[MAIN] Starting activity tracking...\n\n");

    /* Start timers */
    app_timer_start(m_sensor_timer, APP_TIMER_TICKS(SENSOR_UPDATE_MS), NULL);
    app_timer_start(m_gps_timer, APP_TIMER_TICKS(GPS_UPDATE_MS), NULL);
    app_timer_start(m_cloud_timer, APP_TIMER_TICKS(CLOUD_UPLOAD_MS), NULL);
    app_timer_start(m_display_timer, APP_TIMER_TICKS(DISPLAY_UPDATE_MS), NULL);

    /* Main loop */
    while (true) {
        /* Process logs */
        NRF_LOG_PROCESS();

        /* Enter low-power mode */
        __WFE();
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Initialization Functions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Initialize hardware peripherals
 */
static void hardware_init(void)
{
    ret_code_t err_code;

    printf("[INIT] Initializing hardware...\n");

    /* Initialize logging */
    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    /* Initialize LEDs */
    bsp_board_init(BSP_INIT_LEDS);

    /* Initialize app timer */
    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    /* Initialize I2C (TWI) */
    const nrf_drv_twi_config_t twi_config = {
        .scl                = TWI_SCL_PIN,
        .sda                = TWI_SDA_PIN,
        .frequency          = NRF_DRV_TWI_FREQ_400K,
        .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
        .clear_bus_init     = false
    };

    err_code = nrf_drv_twi_init(&m_twi, &twi_config, NULL, NULL);
    APP_ERROR_CHECK(err_code);
    nrf_drv_twi_enable(&m_twi);

    /* Initialize UART */
    const nrf_drv_uart_config_t uart_config = {
        .pseltxd            = UART_TX_PIN,
        .pselrxd            = UART_RX_PIN,
        .pselcts            = NRF_UART_PSEL_DISCONNECTED,
        .pselrts            = NRF_UART_PSEL_DISCONNECTED,
        .p_context          = NULL,
        .hwfc               = NRF_UART_HWFC_DISABLED,
        .parity             = NRF_UART_PARITY_EXCLUDED,
        .baudrate           = NRF_UART_BAUDRATE_115200,
        .interrupt_priority = APP_IRQ_PRIORITY_LOW
    };

    err_code = nrf_drv_uart_init(&m_uart, &uart_config, NULL);
    APP_ERROR_CHECK(err_code);

    printf("[INIT] ✓ Hardware initialized\n");
}

/**
 * @brief Initialize all sensors
 */
static void sensors_init(void)
{
    printf("[INIT] Initializing sensors...\n");

    /* 1. LSM6DSO (IMU) */
    LSM6DSO_Config_t imu_config = {
        .accel_odr = LSM6DSO_ODR_52HZ,
        .accel_scale = LSM6DSO_ACCEL_SCALE_4G,
        .gyro_odr = LSM6DSO_ODR_52HZ,
        .gyro_scale = LSM6DSO_GYRO_SCALE_500DPS,
        .enable_step_counter = true,
        .enable_motion_detect = true
    };
    LSM6DSO_Status_t imu_status = LSM6DSO_Init(&m_twi, &imu_config);
    if (imu_status != LSM6DSO_OK) {
        printf("[INIT] ✗ LSM6DSO initialization failed!\n");
    }

    /* 2. LIS2MDL (Magnetometer) */
    LIS2MDL_Config_t mag_config = {
        .odr = LIS2MDL_ODR_50HZ,
        .mode = LIS2MDL_MODE_CONTINUOUS,
        .temp_comp_enable = true,
        .low_pass_filter = true
    };
    LIS2MDL_Status_t mag_status = LIS2MDL_Init(&m_twi, &mag_config);
    if (mag_status != LIS2MDL_OK) {
        printf("[INIT] ✗ LIS2MDL initialization failed!\n");
    } else {
        /* Perform calibration if needed */
        printf("[INIT] Magnetometer ready. Perform calibration? (rotate device in figure-8)\n");
        /* LIS2MDL_Calibrate(200); */
    }

    /* 3. LPS22HH (Barometer) */
    LPS22HH_Config_t baro_config = {
        .odr = LPS22HH_ODR_10HZ,
        .lpf = LPS22HH_LPF_ODR_DIV_9,
        .low_noise_mode = true,
        .bdu_enable = true
    };
    LPS22HH_Status_t baro_status = LPS22HH_Init(&m_twi, &baro_config);
    if (baro_status != LPS22HH_OK) {
        printf("[INIT] ✗ LPS22HH initialization failed!\n");
    } else {
        /* Set reference pressure at ground level */
        LPS22HH_Data_t ground_level;
        LPS22HH_ReadData(&ground_level);
        LPS22HH_SetReferencePressure(ground_level.pressure_hPa);
    }

    /* 4. SimCom A7672SA (GPS + 4G) */
    A7672_Config_t gps_config = {
        .baud_rate = 115200,
        .power_mode = A7672_POWER_NORMAL,
        .gps_mode = A7672_GPS_STANDALONE
    };
    A7672_Status_t gps_status = A7672_Init(&m_uart, &gps_config);
    if (gps_status != A7672_OK) {
        printf("[INIT] ✗ SimCom A7672SA initialization failed!\n");
    } else {
        /* Enable GPS */
        A7672_GPSPowerOn();
    }

    /* 5. Sensor Fusion Engine */
    SensorFusionConfig_t fusion_config = {
        .enable_gps_validation = true,
        .enable_fall_detection = true,
        .enable_stair_detection = true,
        .enable_route_tracking = true,
        .update_rate_ms = SENSOR_UPDATE_MS
    };
    SensorFusion_Init(&fusion_config);

    printf("[INIT] ✓ All sensors initialized\n");
}

/**
 * @brief Initialize timers
 */
static void timers_init(void)
{
    ret_code_t err_code;

    err_code = app_timer_create(&m_sensor_timer, APP_TIMER_MODE_REPEATED,
                                 sensor_update_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_gps_timer, APP_TIMER_MODE_REPEATED,
                                 gps_update_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_cloud_timer, APP_TIMER_MODE_REPEATED,
                                 cloud_upload_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_display_timer, APP_TIMER_MODE_REPEATED,
                                 display_update_handler);
    APP_ERROR_CHECK(err_code);

    printf("[INIT] ✓ Timers initialized\n");
}

/**
 * @brief Initialize BLE
 */
static void ble_init(void)
{
    /* Placeholder - implement BLE services for mobile app */
    printf("[INIT] BLE initialization (placeholder)\n");
}

/**
 * @brief Initialize TF-M
 */
static void tfm_init(void)
{
#ifdef USE_TFM
    /* Initialize PSA Crypto */
    psa_status_t status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        printf("[INIT] ✗ TF-M crypto initialization failed (0x%08lX)\n", status);
    } else {
        printf("[INIT] ✓ TF-M initialized\n");
    }
#else
    printf("[INIT] TF-M not available (running without secure partition)\n");
#endif
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Timer Handlers
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Sensor update handler (100ms - 10 Hz)
 */
static void sensor_update_handler(void *p_context)
{
    /* Update sensor fusion */
    int result = SensorFusion_Update(&g_fused_data);
    if (result != 0) {
        NRF_LOG_ERROR("Sensor fusion update failed");
        return;
    }

    /* Handle fall detection */
    handle_fall_detection(&g_fused_data);

    /* Update LEDs */
    update_leds(&g_fused_data);
}

/**
 * @brief GPS update handler (5 seconds)
 */
static void gps_update_handler(void *p_context)
{
    /* GPS data is already included in sensor fusion update */
    /* This handler can be used for GPS-specific tasks */

    if (g_fused_data.gps_valid) {
        NRF_LOG_INFO("GPS: %.6f, %.6f (%d sats)",
                     g_fused_data.latitude,
                     g_fused_data.longitude,
                     g_fused_data.satellites);
    }
}

/**
 * @brief Cloud upload handler (1 minute)
 */
static void cloud_upload_handler(void *p_context)
{
    handle_cloud_upload(&g_fused_data);
}

/**
 * @brief Display update handler (1 second)
 */
static void display_update_handler(void *p_context)
{
    display_status(&g_fused_data);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Application Logic
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Handle fall detection
 */
static void handle_fall_detection(const FusedSensorData_t *data)
{
    if (data->fall_status == FALL_CONFIRMED && !g_fall_alert_sent) {
        printf("\n");
        printf("🚨🚨🚨 FALL DETECTED 🚨🚨🚨\n");
        printf("Time: %lu ms\n", data->fall_timestamp);

        if (data->gps_valid) {
            printf("Location: %.6f, %.6f\n", data->latitude, data->longitude);

            /* Send emergency alert via 4G */
            char alert_json[512];
            snprintf(alert_json, sizeof(alert_json),
                "{\"device_id\":\"%s\","
                "\"type\":\"fall_detected\","
                "\"timestamp\":%lu,"
                "\"location\":{\"lat\":%.6f,\"lon\":%.6f},"
                "\"emergency_contact\":\"%s\"}",
                DEVICE_ID,
                data->fall_timestamp,
                data->latitude,
                data->longitude,
                EMERGENCY_PHONE);

            char response[256];
            A7672_Status_t status = A7672_HTTPPost(
                "https://emergency.api/alert",
                alert_json,
                strlen(alert_json),
                response,
                sizeof(response)
            );

            if (status == A7672_OK) {
                printf("✓ Emergency alert sent successfully\n");
            } else {
                printf("✗ Failed to send emergency alert\n");
            }
        } else {
            printf("⚠️  GPS not available for location\n");
        }

        /* Turn on fall LED */
        bsp_board_led_on(LED_FALL);

        g_fall_alert_sent = true;
        printf("\n");
    }

    /* Reset flag when fall is cleared */
    if (data->fall_status == FALL_NO_FALL && g_fall_alert_sent) {
        bsp_board_led_off(LED_FALL);
        g_fall_alert_sent = false;
    }
}

/**
 * @brief Upload activity data to cloud
 */
static void handle_cloud_upload(const FusedSensorData_t *data)
{
    /* Generate JSON payload */
    char json[1024];
    snprintf(json, sizeof(json),
        "{"
        "\"device_id\":\"%s\","
        "\"timestamp\":%lu,"
        "\"activity\":\"%s\","
        "\"confidence\":%.2f,"
        "\"steps\":%lu,"
        "\"distance_m\":%.1f,"
        "\"speed_kmh\":%.1f,"
        "\"heading\":%.1f,"
        "\"altitude_m\":%.1f,"
        "\"location\":{\"lat\":%.6f,\"lon\":%.6f},"
        "\"gps_valid\":%s,"
        "\"temperature_c\":%.1f"
        "}",
        DEVICE_ID,
        data->timestamp_ms,
        SensorFusion_GetActivityName(data->activity_final),
        data->confidence,
        data->total_steps,
        data->distance_m,
        data->speed_kmh,
        data->heading_degrees,
        data->altitude_m,
        data->latitude,
        data->longitude,
        data->gps_valid ? "true" : "false",
        data->temperature_c
    );

    /* Upload via 4G */
    bsp_board_led_on(LED_CLOUD);

    char response[256];
    A7672_Status_t status = A7672_HTTPPost(
        CLOUD_API_URL,
        json,
        strlen(json),
        response,
        sizeof(response)
    );

    if (status == A7672_OK) {
        printf("[CLOUD] ✓ Data uploaded successfully\n");
    } else {
        printf("[CLOUD] ✗ Upload failed\n");
    }

    bsp_board_led_off(LED_CLOUD);

    /* Save to TF-M storage */
#ifdef USE_TFM_STORAGE
    SensorFusion_SaveToStorage(data);
#endif
}

/**
 * @brief Display current status
 */
static void display_status(const FusedSensorData_t *data)
{
    printf("─────────────────────────────────────────────────────────────\n");
    printf("Activity: %s (%.0f%% confident)\n",
           SensorFusion_GetActivityName(data->activity_final),
           data->confidence * 100);

    printf("Motion: %lu steps | %.1f km | %.1f km/h\n",
           data->total_steps,
           data->distance_m / 1000.0f,
           data->speed_kmh);

    printf("Heading: %.1f° (%s)\n",
           data->heading_degrees,
           data->direction_name);

    printf("Altitude: %.1f m (Floor %d, Stairs %d)\n",
           data->altitude_m,
           data->floors_climbed,
           data->stairs_climbed);

    if (data->gps_valid) {
        printf("GPS: %.6f, %.6f (%d sats)\n",
               data->latitude,
               data->longitude,
               data->satellites);
    } else {
        printf("GPS: No fix\n");
    }

    printf("Environment: %.1f°C, %.1f hPa\n",
           data->temperature_c,
           data->pressure_hPa);

    printf("Location: %s\n",
           (data->location_type == LOCATION_INDOOR) ? "Indoor" :
           (data->location_type == LOCATION_OUTDOOR) ? "Outdoor" : "Unknown");

    printf("Fall status: %s\n",
           SensorFusion_GetFallStatusName(data->fall_status));

    /* Daily summary */
    char summary[512];
    if (SensorFusion_GenerateDailySummary(summary, sizeof(summary)) == 0) {
        printf("\nDaily Summary:\n%s\n", summary);
    }

    printf("─────────────────────────────────────────────────────────────\n\n");
}

/**
 * @brief Update LED indicators
 */
static void update_leds(const FusedSensorData_t *data)
{
    /* Activity LED (blink during active activities) */
    if (data->activity_final != ACTIVITY_IDLE) {
        static bool led_state = false;
        led_state = !led_state;
        if (led_state) {
            bsp_board_led_on(LED_ACTIVITY);
        } else {
            bsp_board_led_off(LED_ACTIVITY);
        }
    } else {
        bsp_board_led_off(LED_ACTIVITY);
    }

    /* GPS LED (on when GPS fix acquired) */
    if (data->gps_valid) {
        bsp_board_led_on(LED_GPS);
    } else {
        bsp_board_led_off(LED_GPS);
    }

    /* Fall LED handled in handle_fall_detection() */
    /* Cloud LED handled in handle_cloud_upload() */
}

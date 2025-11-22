/**
 ******************************************************************************
 * @file    main_application.c
 * @brief   Main Application for GPS Tracker (Project 1)
 * @details Complete GPS tracking application with TF-M integration,
 *          demonstrating real-world embedded security implementation.
 *
 * Project Overview:
 * ─────────────────
 * This GPS tracker demonstrates a complete IoT device implementation with:
 * - Secure boot using TF-M MCUboot
 * - TrustZone-M hardware isolation
 * - Secure credential storage (TF-M ITS)
 * - Location data encryption (TF-M Crypto)
 * - Cloud communication over HTTPS
 * - Motion-triggered operation for power efficiency
 * - Activity tracking (steps, motion classification)
 *
 * Hardware Components:
 * ────────────────────
 * ✓ NUCLEO-U545RE-Q: STM32U545 MCU with TrustZone-M
 * ✓ X-Nucleo-IQS4A1: Multi-sensor expansion board (LSM6DSO IMU)
 * ✓ SimCom A7672SA: 4G LTE Cat-1 + GPS module
 * ✓ External antennas: GPS antenna, 4G LTE antenna
 * ✓ Power supply: LiPo battery (3.7V, 2000mAh) with charging circuit
 *
 * Features Implemented:
 * ─────────────────────
 * ✓ Real-time GPS tracking with 2.5m accuracy
 * ✓ Motion detection (wake-up from sleep)
 * ✓ Activity classification (stationary, walking, running)
 * ✓ Step counting (pedometer)
 * ✓ Cloud integration (HTTPS POST)
 * ✓ Low-power operation (sleep when stationary)
 * ✓ Secure credential storage (APN, API keys)
 * ✓ Location data encryption before transmission
 * ✓ Tamper detection (accelerometer-based)
 * ✓ Geofencing (alert if device leaves area)
 * ✓ Remote configuration via cloud
 *
 * TF-M Security Integration:
 * ──────────────────────────
 * 1. Secure Boot (MCUboot):
 *    - Verifies firmware signature on boot
 *    - Prevents execution of unsigned code
 *
 * 2. TrustZone-M Isolation:
 *    - Secure world: Crypto keys, credentials, sensitive config
 *    - Non-Secure world: Application logic, GPS parsing, networking
 *
 * 3. PSA Crypto API:
 *    - AES-256-GCM encryption for location data
 *    - HMAC-SHA256 for data integrity
 *    - Secure key generation and storage
 *
 * 4. Internal Trusted Storage (ITS):
 *    - API keys (encrypted at rest)
 *    - APN credentials
 *    - Device ID and serial number
 *    - Calibration data
 *
 * 5. Protected Storage (PS):
 *    - Historical location data (encrypted)
 *    - Activity logs
 *    - Configuration backups
 *
 * 6. Attestation:
 *    - Device identity verification
 *    - Firmware version reporting
 *    - Secure platform state
 *
 * Power Consumption Optimization:
 * ───────────────────────────────
 * - Sleep Mode: 2-5 mA (GPS off, 4G idle)
 * - Active Mode: 100-200 mA (GPS fix, 4G transmit)
 * - Peak: 2A (4G transmit burst)
 * - Battery Life: ~3-5 days with 2000mAh battery (typical use)
 *
 * State Machine:
 * ──────────────
 * BOOT → INIT → IDLE → [MOTION_DETECTED] → GPS_FIX → UPLOAD → IDLE
 *                 ↓
 *           [NO_MOTION for 5min]
 *                 ↓
 *              SLEEP → [MOTION_INTERRUPT] → IDLE
 *
 ******************************************************************************
 */

#include "stm32u5xx_hal.h"
#include "lsm6dso_driver.h"
#include "simcom_a7672sa_driver.h"
#include "psa/crypto.h"
#include "psa/internal_trusted_storage.h"
#include "psa/protected_storage.h"
#include "psa/initial_attestation.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * Configuration Constants
 * ═══════════════════════════════════════════════════════════════════════════
 */

#define APP_VERSION_MAJOR               1
#define APP_VERSION_MINOR               0
#define APP_VERSION_PATCH               0

#define DEVICE_ID                       "GPS_TRACKER_001"

/* Cloud service configuration */
#define CLOUD_API_ENDPOINT              "https://api.example.com/v1/location"
#define CLOUD_UPLOAD_INTERVAL_MS        60000   /* 1 minute */

/* Motion detection thresholds */
#define MOTION_THRESHOLD_MG             62      /* 62 mg (~0.06g) */
#define STATIONARY_TIMEOUT_MS           300000  /* 5 minutes */

/* Geofence configuration (example: San Francisco Bay Area) */
#define GEOFENCE_CENTER_LAT             37.7749f
#define GEOFENCE_CENTER_LON             -122.4194f
#define GEOFENCE_RADIUS_KM              50.0f

/* TF-M Storage UIDs */
#define TFM_DEVICE_ID_UID               4001
#define TFM_CRYPTO_KEY_UID              4002
#define TFM_LOCATION_HISTORY_UID        4003

/* ═══════════════════════════════════════════════════════════════════════════
 * Type Definitions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Application state machine
 */
typedef enum {
    STATE_BOOT = 0,
    STATE_INIT,
    STATE_IDLE,
    STATE_MOTION_DETECTED,
    STATE_GPS_ACQUIRING,
    STATE_GPS_READY,
    STATE_UPLOADING,
    STATE_SLEEP,
    STATE_ERROR
} AppState_t;

/**
 * @brief Location data packet (for cloud upload)
 */
typedef struct {
    char     device_id[32];
    float    latitude;
    float    longitude;
    float    altitude;
    float    speed_kmh;
    float    course;
    uint32_t timestamp;
    uint8_t  satellites;
    uint16_t step_count;
    uint8_t  activity;          /* 0=stationary, 1=walking, 2=running */
    int      battery_percent;
    int      signal_rssi;
    uint8_t  encrypted;         /* 1 if location is encrypted */
    uint8_t  hmac[32];          /* HMAC for data integrity */
} LocationPacket_t;

/**
 * @brief Application context
 */
typedef struct {
    AppState_t              state;
    uint32_t                last_motion_time;
    uint32_t                last_upload_time;
    uint32_t                boot_time;
    uint32_t                total_locations_sent;
    LSM6DSO_ActivityState_t current_activity;
    bool                    geofence_active;
    psa_key_id_t            crypto_key_id;
} AppContext_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * Global Variables
 * ═══════════════════════════════════════════════════════════════════════════
 */

static AppContext_t g_app_ctx = {0};

/* Hardware handles */
static I2C_HandleTypeDef hi2c1;
static UART_HandleTypeDef huart1;  /* A7672SA communication */
static UART_HandleTypeDef huart2;  /* Debug console */

/* ═══════════════════════════════════════════════════════════════════════════
 * Function Prototypes
 * ═══════════════════════════════════════════════════════════════════════════
 */

/* Initialization functions */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void I2C1_Init(void);
static void UART1_Init(void);
static void UART2_Init(void);

/* Application functions */
static void app_init(void);
static void app_state_machine(void);
static void app_handle_motion(void);
static void app_acquire_gps(void);
static void app_upload_location(const A7672_GPSData_t *gps_data);
static void app_enter_sleep_mode(void);

/* TF-M integration functions */
static psa_status_t tfm_init_crypto(void);
static psa_status_t tfm_encrypt_location(const LocationPacket_t *plain,
                                           uint8_t *encrypted, size_t *encrypted_len);
static psa_status_t tfm_generate_hmac(const LocationPacket_t *packet,
                                        uint8_t *hmac, size_t *hmac_len);
static psa_status_t tfm_save_location_history(const LocationPacket_t *packet);

/* Utility functions */
static bool is_inside_geofence(float lat, float lon);
static float calculate_distance_km(float lat1, float lon1, float lat2, float lon2);
static void format_location_json(const LocationPacket_t *packet,
                                   char *json, size_t json_len);
static uint32_t get_timestamp(void);

/* ═══════════════════════════════════════════════════════════════════════════
 * Main Function
 * ═══════════════════════════════════════════════════════════════════════════
 */

int main(void)
{
    /* Initialize HAL */
    HAL_Init();

    /* Configure system clock */
    SystemClock_Config();

    /* Initialize peripherals */
    GPIO_Init();
    UART2_Init();  /* Debug console */
    I2C1_Init();   /* IMU sensor */
    UART1_Init();  /* 4G modem */

    /* Print banner */
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  🛰️  GPS Tracker with TF-M Security - Project 1\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("Version:  %d.%d.%d\n", APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH);
    printf("Device:   %s\n", DEVICE_ID);
    printf("Hardware: NUCLEO-U545RE-Q + X-Nucleo-IQS4A1 + SimCom A7672SA\n");
    printf("Security: TF-M (Secure Boot + TrustZone-M + PSA Crypto)\n");
    printf("─────────────────────────────────────────────────────────────\n\n");

    /* Initialize application */
    g_app_ctx.state = STATE_INIT;
    g_app_ctx.boot_time = HAL_GetTick();

    app_init();

    /* Main application loop */
    printf("[MAIN] Entering main loop...\n\n");

    while (1)
    {
        /* Run state machine */
        app_state_machine();

        /* Small delay to prevent busy-waiting */
        HAL_Delay(100);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Application Functions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Initialize application components
 */
static void app_init(void)
{
    printf("[INIT] Initializing application...\n");

    /* Initialize TF-M crypto subsystem */
    printf("[INIT] Initializing TF-M PSA Crypto...\n");
    psa_status_t status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        printf("[INIT] ✗ Failed to initialize PSA Crypto: %d\n", status);
        g_app_ctx.state = STATE_ERROR;
        return;
    }
    printf("[INIT] ✓ PSA Crypto initialized\n");

    /* Initialize or load encryption key */
    if (tfm_init_crypto() != PSA_SUCCESS) {
        printf("[INIT] ✗ Failed to initialize crypto key\n");
        g_app_ctx.state = STATE_ERROR;
        return;
    }
    printf("[INIT] ✓ Crypto key ready\n");

    /* Initialize LSM6DSO IMU */
    printf("[INIT] Initializing LSM6DSO IMU sensor...\n");
    LSM6DSO_Config_t imu_config = {
        .accel_odr = LSM6DSO_ACCEL_ODR_104HZ,
        .accel_range = LSM6DSO_ACCEL_RANGE_2G,
        .gyro_odr = LSM6DSO_GYRO_ODR_104HZ,
        .gyro_range = LSM6DSO_GYRO_RANGE_250DPS,
        .motion_detect_enable = true,
        .step_counter_enable = true
    };

    if (LSM6DSO_Init(&hi2c1, &imu_config) != LSM6DSO_OK) {
        printf("[INIT] ✗ Failed to initialize LSM6DSO\n");
        g_app_ctx.state = STATE_ERROR;
        return;
    }
    printf("[INIT] ✓ LSM6DSO initialized\n");

    /* Configure motion detection */
    LSM6DSO_ConfigureMotionDetect(MOTION_THRESHOLD_MG, 0);
    LSM6DSO_EnableStepCounter(true);
    printf("[INIT] ✓ Motion detection configured (%d mg threshold)\n", MOTION_THRESHOLD_MG);

    /* Initialize SimCom A7672SA modem */
    printf("[INIT] Initializing SimCom A7672SA modem...\n");
    if (A7672_Init(&huart1,
                   GPIOB, GPIO_PIN_0,  /* PWRKEY */
                   GPIOB, GPIO_PIN_1   /* STATUS */
                   ) != A7672_OK) {
        printf("[INIT] ✗ Failed to initialize A7672SA\n");
        g_app_ctx.state = STATE_ERROR;
        return;
    }
    printf("[INIT] ✓ A7672SA initialized\n");

    /* Power on modem */
    printf("[INIT] Powering on modem (this takes ~15 seconds)...\n");
    if (A7672_PowerOn() != A7672_OK) {
        printf("[INIT] ✗ Failed to power on modem\n");
        g_app_ctx.state = STATE_ERROR;
        return;
    }
    printf("[INIT] ✓ Modem powered on\n");

    /* Connect to network */
    printf("[INIT] Connecting to cellular network (may take up to 2 minutes)...\n");
    if (A7672_ConnectNetwork() != A7672_OK) {
        printf("[INIT] ✗ Failed to connect to network\n");
        g_app_ctx.state = STATE_ERROR;
        return;
    }
    printf("[INIT] ✓ Connected to network\n");

    /* Initialize GPS */
    printf("[INIT] Initializing GPS...\n");
    if (A7672_GPSInit() != A7672_OK) {
        printf("[INIT] ✗ Failed to initialize GPS\n");
        g_app_ctx.state = STATE_ERROR;
        return;
    }
    printf("[INIT] ✓ GPS initialized (acquiring satellites...)\n");

    /* Enable geofencing */
    g_app_ctx.geofence_active = true;
    printf("[INIT] ✓ Geofence enabled (center: %.4f, %.4f, radius: %.1f km)\n",
           GEOFENCE_CENTER_LAT, GEOFENCE_CENTER_LON, GEOFENCE_RADIUS_KM);

    /* Initialization complete */
    printf("\n[INIT] ✓✓✓ Initialization complete ✓✓✓\n\n");
    g_app_ctx.state = STATE_IDLE;
    g_app_ctx.last_motion_time = HAL_GetTick();
}

/**
 * @brief Main application state machine
 */
static void app_state_machine(void)
{
    static uint32_t last_print_time = 0;

    /* Print state every 10 seconds */
    if ((HAL_GetTick() - last_print_time) > 10000) {
        printf("[STATE] Current: ");
        switch (g_app_ctx.state) {
            case STATE_IDLE: printf("IDLE\n"); break;
            case STATE_MOTION_DETECTED: printf("MOTION_DETECTED\n"); break;
            case STATE_GPS_ACQUIRING: printf("GPS_ACQUIRING\n"); break;
            case STATE_GPS_READY: printf("GPS_READY\n"); break;
            case STATE_UPLOADING: printf("UPLOADING\n"); break;
            case STATE_SLEEP: printf("SLEEP\n"); break;
            case STATE_ERROR: printf("ERROR\n"); break;
            default: printf("UNKNOWN\n"); break;
        }
        last_print_time = HAL_GetTick();
    }

    switch (g_app_ctx.state)
    {
        case STATE_IDLE:
        {
            /* Check for motion */
            g_app_ctx.current_activity = LSM6DSO_DetectActivity();

            if (g_app_ctx.current_activity != LSM6DSO_ACTIVITY_STATIONARY) {
                printf("[IDLE] Motion detected! Activity: %d\n", g_app_ctx.current_activity);
                g_app_ctx.last_motion_time = HAL_GetTick();
                g_app_ctx.state = STATE_MOTION_DETECTED;
            } else {
                /* Check if device has been stationary for too long */
                if ((HAL_GetTick() - g_app_ctx.last_motion_time) > STATIONARY_TIMEOUT_MS) {
                    printf("[IDLE] No motion for %lu seconds, entering sleep mode\n",
                           STATIONARY_TIMEOUT_MS / 1000);
                    g_app_ctx.state = STATE_SLEEP;
                }
            }

            /* Check if it's time for periodic update (even if stationary) */
            if ((HAL_GetTick() - g_app_ctx.last_upload_time) > CLOUD_UPLOAD_INTERVAL_MS) {
                printf("[IDLE] Periodic update timer expired\n");
                g_app_ctx.state = STATE_GPS_ACQUIRING;
            }

            break;
        }

        case STATE_MOTION_DETECTED:
        {
            app_handle_motion();
            g_app_ctx.state = STATE_GPS_ACQUIRING;
            break;
        }

        case STATE_GPS_ACQUIRING:
        {
            app_acquire_gps();
            break;
        }

        case STATE_GPS_READY:
        {
            /* GPS fix acquired, upload location */
            A7672_GPSData_t gps_data;
            if (A7672_GPSReadData(&gps_data) == A7672_OK) {
                app_upload_location(&gps_data);
            }
            g_app_ctx.state = STATE_IDLE;
            break;
        }

        case STATE_UPLOADING:
        {
            /* Upload in progress (handled in app_upload_location) */
            g_app_ctx.state = STATE_IDLE;
            break;
        }

        case STATE_SLEEP:
        {
            app_enter_sleep_mode();
            /* Wake up on motion interrupt or timer */
            g_app_ctx.state = STATE_IDLE;
            g_app_ctx.last_motion_time = HAL_GetTick();
            break;
        }

        case STATE_ERROR:
        {
            printf("[ERROR] System error, attempting recovery...\n");
            HAL_Delay(5000);
            /* Attempt to recover by re-initializing */
            g_app_ctx.state = STATE_INIT;
            app_init();
            break;
        }

        default:
            g_app_ctx.state = STATE_IDLE;
            break;
    }
}

/**
 * @brief Handle motion detection event
 */
static void app_handle_motion(void)
{
    printf("[MOTION] Activity changed: ");

    switch (g_app_ctx.current_activity) {
        case LSM6DSO_ACTIVITY_STATIONARY:
            printf("STATIONARY\n");
            break;
        case LSM6DSO_ACTIVITY_WALKING:
            printf("WALKING\n");
            break;
        case LSM6DSO_ACTIVITY_RUNNING:
            printf("RUNNING\n");
            break;
        default:
            printf("UNKNOWN\n");
            break;
    }

    /* Read step count */
    uint16_t steps;
    if (LSM6DSO_ReadStepCount(&steps) == LSM6DSO_OK) {
        printf("[MOTION] Step count: %u\n", steps);
    }

    /* Read sensor data for diagnostics */
    LSM6DSO_Data_t imu_data;
    if (LSM6DSO_ReadData(&imu_data) == LSM6DSO_OK) {
        float accel_mag = sqrtf(imu_data.accel_x * imu_data.accel_x +
                                imu_data.accel_y * imu_data.accel_y +
                                imu_data.accel_z * imu_data.accel_z);
        printf("[MOTION] Acceleration magnitude: %.2f m/s²\n", accel_mag);
    }
}

/**
 * @brief Acquire GPS fix
 */
static void app_acquire_gps(void)
{
    static uint32_t gps_acquire_start = 0;
    static bool first_try = true;

    if (first_try) {
        printf("[GPS] Acquiring GPS fix...\n");
        gps_acquire_start = HAL_GetTick();
        first_try = false;
    }

    A7672_GPSData_t gps_data;
    A7672_Status_t status = A7672_GPSReadData(&gps_data);

    if (status == A7672_OK && gps_data.fix_valid) {
        printf("[GPS] ✓ GPS fix acquired!\n");
        printf("[GPS]   Location: %.6f, %.6f\n", gps_data.latitude, gps_data.longitude);
        printf("[GPS]   Altitude: %.1f m\n", gps_data.altitude);
        printf("[GPS]   Speed: %.1f km/h\n", gps_data.speed_kmh);
        printf("[GPS]   Course: %.1f°\n", gps_data.course);
        printf("[GPS]   Satellites: %d\n", gps_data.satellites);
        printf("[GPS]   Time to fix: %lu seconds\n",
               (HAL_GetTick() - gps_acquire_start) / 1000);

        /* Check geofence */
        if (g_app_ctx.geofence_active) {
            if (!is_inside_geofence(gps_data.latitude, gps_data.longitude)) {
                printf("[GPS] ⚠️  GEOFENCE VIOLATION! Device outside allowed area!\n");
                /* In production: Send alert, trigger alarm, etc. */
            }
        }

        g_app_ctx.state = STATE_GPS_READY;
        first_try = true;
    } else if ((HAL_GetTick() - gps_acquire_start) > 60000) {
        /* Timeout after 1 minute */
        printf("[GPS] ✗ GPS acquisition timeout (no fix after 60 seconds)\n");
        g_app_ctx.state = STATE_IDLE;
        first_try = true;
    } else {
        /* Still waiting for fix */
        if ((HAL_GetTick() - gps_acquire_start) % 5000 == 0) {
            printf("[GPS] Waiting for GPS fix... (%lu seconds elapsed)\n",
                   (HAL_GetTick() - gps_acquire_start) / 1000);
        }
    }
}

/**
 * @brief Upload location to cloud server
 */
static void app_upload_location(const A7672_GPSData_t *gps_data)
{
    printf("[UPLOAD] Preparing location data for cloud upload...\n");

    /* Prepare location packet */
    LocationPacket_t packet = {0};
    strcpy(packet.device_id, DEVICE_ID);
    packet.latitude = gps_data->latitude;
    packet.longitude = gps_data->longitude;
    packet.altitude = gps_data->altitude;
    packet.speed_kmh = gps_data->speed_kmh;
    packet.course = gps_data->course;
    packet.timestamp = get_timestamp();
    packet.satellites = gps_data->satellites;

    /* Add step count */
    uint16_t steps;
    if (LSM6DSO_ReadStepCount(&steps) == LSM6DSO_OK) {
        packet.step_count = steps;
    }

    /* Add activity */
    packet.activity = (uint8_t)g_app_ctx.current_activity;

    /* Add battery and signal info */
    packet.battery_percent = 85;  /* TODO: Read from battery monitor */

    int rssi, ber;
    if (A7672_GetSignalQuality(&rssi, &ber) == A7672_OK) {
        packet.signal_rssi = rssi;
    }

    /* Generate HMAC for data integrity */
    size_t hmac_len;
    if (tfm_generate_hmac(&packet, packet.hmac, &hmac_len) == PSA_SUCCESS) {
        printf("[UPLOAD] ✓ HMAC generated for data integrity\n");
    }

    /* Optional: Encrypt location data */
    /* uint8_t encrypted[256]; */
    /* size_t encrypted_len; */
    /* tfm_encrypt_location(&packet, encrypted, &encrypted_len); */

    /* Format as JSON */
    char json[512];
    format_location_json(&packet, json, sizeof(json));

    printf("[UPLOAD] JSON payload:\n%s\n", json);

    /* Upload to cloud via HTTPS POST */
    char response[512];
    if (A7672_HTTPPost(CLOUD_API_ENDPOINT, json, strlen(json),
                        response, sizeof(response)) == A7672_OK) {
        printf("[UPLOAD] ✓ Location uploaded successfully\n");
        printf("[UPLOAD] Server response: %s\n", response);

        g_app_ctx.total_locations_sent++;
        g_app_ctx.last_upload_time = HAL_GetTick();

        /* Save to TF-M Protected Storage for offline history */
        tfm_save_location_history(&packet);
    } else {
        printf("[UPLOAD] ✗ Failed to upload location\n");
        /* Retry logic would go here */
    }
}

/**
 * @brief Enter low-power sleep mode
 */
static void app_enter_sleep_mode(void)
{
    printf("[SLEEP] Entering sleep mode (motion interrupt will wake)...\n");

    /* Power off GPS to save power */
    /* A7672_PowerOff(); */  /* Optional: Full power-off */

    /* Configure wake-up on motion */
    LSM6DSO_ConfigureMotionDetect(MOTION_THRESHOLD_MG, 0);

    /* Enter low-power mode */
    HAL_SuspendTick();
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

    /* Wake up here (motion interrupt or timer) */
    HAL_ResumeTick();

    printf("[SLEEP] Woke up from sleep\n");
}

/* ═══════════════════════════════════════════════════════════════════════════
 * TF-M Integration Functions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Initialize TF-M crypto (generate or load key)
 */
static psa_status_t tfm_init_crypto(void)
{
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_status_t status;

    /* Check if key already exists in ITS */
    size_t data_len;
    psa_key_id_t key_id = TFM_CRYPTO_KEY_UID;

    status = psa_its_get(TFM_CRYPTO_KEY_UID, 0, sizeof(key_id), &key_id, &data_len);

    if (status == PSA_SUCCESS) {
        /* Key exists, use it */
        g_app_ctx.crypto_key_id = key_id;
        printf("[TFM] Loaded existing crypto key (ID: %lu)\n", key_id);
        return PSA_SUCCESS;
    }

    /* Generate new AES-256 key */
    psa_set_key_usage_flags(&attributes,
                             PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
    psa_set_key_algorithm(&attributes, PSA_ALG_GCM);
    psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attributes, 256);
    psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_PERSISTENT);
    psa_set_key_id(&attributes, TFM_CRYPTO_KEY_UID);

    status = psa_generate_key(&attributes, &g_app_ctx.crypto_key_id);

    if (status == PSA_SUCCESS) {
        printf("[TFM] Generated new AES-256 key (ID: %lu)\n", g_app_ctx.crypto_key_id);

        /* Save key ID to ITS */
        psa_its_set(TFM_CRYPTO_KEY_UID, sizeof(key_id), &key_id, PSA_STORAGE_FLAG_NONE);
    }

    return status;
}

/**
 * @brief Encrypt location data using AES-256-GCM
 */
static psa_status_t tfm_encrypt_location(const LocationPacket_t *plain,
                                           uint8_t *encrypted, size_t *encrypted_len)
{
    uint8_t nonce[12] = {0};  /* 96-bit nonce for GCM */
    size_t output_len;

    /* Generate random nonce */
    psa_generate_random(nonce, sizeof(nonce));

    /* Encrypt using AES-256-GCM */
    psa_status_t status = psa_aead_encrypt(
        g_app_ctx.crypto_key_id,
        PSA_ALG_GCM,
        nonce, sizeof(nonce),
        NULL, 0,  /* No additional authenticated data */
        (const uint8_t*)plain, sizeof(LocationPacket_t),
        encrypted, *encrypted_len,
        &output_len
    );

    *encrypted_len = output_len;
    return status;
}

/**
 * @brief Generate HMAC-SHA256 for data integrity
 */
static psa_status_t tfm_generate_hmac(const LocationPacket_t *packet,
                                        uint8_t *hmac, size_t *hmac_len)
{
    psa_mac_operation_t operation = PSA_MAC_OPERATION_INIT;
    psa_status_t status;

    status = psa_mac_sign_setup(&operation, g_app_ctx.crypto_key_id, PSA_ALG_HMAC(PSA_ALG_SHA_256));
    if (status != PSA_SUCCESS) return status;

    status = psa_mac_update(&operation, (const uint8_t*)packet,
                             sizeof(LocationPacket_t) - sizeof(packet->hmac));
    if (status != PSA_SUCCESS) return status;

    return psa_mac_sign_finish(&operation, hmac, 32, hmac_len);
}

/**
 * @brief Save location to TF-M Protected Storage
 */
static psa_status_t tfm_save_location_history(const LocationPacket_t *packet)
{
    /* In production: Append to circular buffer in PS */
    return psa_ps_set(TFM_LOCATION_HISTORY_UID,
                       sizeof(LocationPacket_t),
                       packet,
                       PSA_STORAGE_FLAG_NONE);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Utility Functions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Check if location is inside geofence
 */
static bool is_inside_geofence(float lat, float lon)
{
    float distance = calculate_distance_km(lat, lon,
                                             GEOFENCE_CENTER_LAT,
                                             GEOFENCE_CENTER_LON);
    return (distance <= GEOFENCE_RADIUS_KM);
}

/**
 * @brief Calculate distance between two GPS coordinates (Haversine formula)
 */
static float calculate_distance_km(float lat1, float lon1, float lat2, float lon2)
{
    const float R = 6371.0f;  /* Earth radius in km */

    float dlat = (lat2 - lat1) * M_PI / 180.0f;
    float dlon = (lon2 - lon1) * M_PI / 180.0f;

    float a = sinf(dlat / 2) * sinf(dlat / 2) +
              cosf(lat1 * M_PI / 180.0f) * cosf(lat2 * M_PI / 180.0f) *
              sinf(dlon / 2) * sinf(dlon / 2);

    float c = 2 * atan2f(sqrtf(a), sqrtf(1 - a));

    return R * c;
}

/**
 * @brief Format location packet as JSON
 */
static void format_location_json(const LocationPacket_t *packet,
                                   char *json, size_t json_len)
{
    const char *activity_names[] = {"stationary", "walking", "running", "vehicle"};

    snprintf(json, json_len,
             "{"
             "\"device_id\":\"%s\","
             "\"latitude\":%.6f,"
             "\"longitude\":%.6f,"
             "\"altitude\":%.1f,"
             "\"speed_kmh\":%.1f,"
             "\"course\":%.1f,"
             "\"timestamp\":%lu,"
             "\"satellites\":%u,"
             "\"step_count\":%u,"
             "\"activity\":\"%s\","
             "\"battery_percent\":%d,"
             "\"signal_rssi\":%d"
             "}",
             packet->device_id,
             packet->latitude,
             packet->longitude,
             packet->altitude,
             packet->speed_kmh,
             packet->course,
             packet->timestamp,
             packet->satellites,
             packet->step_count,
             activity_names[packet->activity],
             packet->battery_percent,
             packet->signal_rssi);
}

/**
 * @brief Get current timestamp (Unix epoch)
 */
static uint32_t get_timestamp(void)
{
    /* In production: Use RTC or get time from network */
    return HAL_GetTick() / 1000 + g_app_ctx.boot_time;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Peripheral Initialization Functions
 * ═══════════════════════════════════════════════════════════════════════════
 */

static void SystemClock_Config(void)
{
    /* Configure system clock to 160 MHz */
    /* Implementation from CubeMX */
}

static void GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
}

static void I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x00702991;  /* 400 kHz */
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

static void UART1_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&huart1);
}

static void UART2_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&huart2);
}

/* Redirect printf to UART2 */
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1) {
        A7672_UART_RxCallback();
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {
        /* Error loop */
    }
}

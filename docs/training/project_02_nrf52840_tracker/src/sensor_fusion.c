/**
 ******************************************************************************
 * @file    sensor_fusion.c
 * @brief   Multi-Sensor Fusion Engine Implementation
 * @details Combines LSM6DSO + LIS2MDL + LPS22HH + GPS for intelligent activity tracking
 *
 * Architecture:
 * ┌─────────────────────────────────────────────────────────────┐
 * │ Sensors → Fusion → ML Inference → GPS Validation → Output  │
 * └─────────────────────────────────────────────────────────────┘
 *
 * Features:
 * - Multi-sensor data fusion (4 sensors)
 * - GPS-validated ML activity classification
 * - Fall detection (3-stage algorithm)
 * - Indoor/outdoor detection
 * - Stair climbing detection
 * - Route tracking and recommendation
 * - Calorie calculation
 * - TF-M secure storage
 *
 ******************************************************************************
 */

#include "sensor_fusion.h"
#include "lsm6dso_driver.h"
#include "lis2mdl_driver.h"
#include "lps22hh_driver.h"
#include "simcom_a7672sa_driver.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

#ifdef USE_TFM_STORAGE
#include "psa/internal_trusted_storage.h"
#include "psa/protected_storage.h"
#include "psa/crypto.h"
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 * Constants
 * ═══════════════════════════════════════════════════════════════════════════
 */

/* Activity thresholds */
#define IDLE_THRESHOLD_MS2          0.5f   /* <0.5 m/s² = idle */
#define WALKING_SPEED_MIN_KMH       1.0f
#define WALKING_SPEED_MAX_KMH       7.0f
#define RUNNING_SPEED_MIN_KMH       7.0f
#define RUNNING_SPEED_MAX_KMH       20.0f
#define CYCLING_SPEED_MIN_KMH       10.0f
#define CYCLING_SPEED_MAX_KMH       30.0f
#define DRIVING_SPEED_MIN_KMH       30.0f

/* Fall detection */
#define FALL_FREE_FALL_THRESHOLD    0.5f   /* <0.5g = free fall */
#define FALL_IMPACT_THRESHOLD       4.0f   /* >4g = impact */
#define FALL_IMMOBILITY_TIME_MS     5000   /* 5 seconds no movement */
#define FALL_IMMOBILITY_THRESHOLD   0.3f   /* <0.3 m/s² = immobile */

/* Stair detection */
#define STAIR_ALTITUDE_MIN_M        0.5f   /* Min 0.5m change */
#define STAIR_HEIGHT_M              0.18f  /* Typical stair: 18cm */

/* Indoor/outdoor detection */
#define INDOOR_GPS_SAT_THRESHOLD    4      /* <4 sats = likely indoor */
#define INDOOR_MAGNETIC_ANOMALY     100.0f /* >100mG deviation = indoor */

/* ML feature window */
#define ML_WINDOW_SIZE              100    /* 100 samples @ 50Hz = 2 seconds */

/* TF-M storage UIDs */
#define TFM_FUSED_DATA_UID          0x0000000A
#define TFM_DAILY_SUMMARY_UID       0x0000000B
#define TFM_ROUTE_HISTORY_UID       0x0000000C

/* MET values for calorie calculation */
#define MET_IDLE                    1.0f
#define MET_WALKING                 3.5f
#define MET_RUNNING                 9.0f
#define MET_CYCLING                 7.5f
#define MET_STAIRS                  8.0f
#define MET_HIKING                  6.0f

/* ═══════════════════════════════════════════════════════════════════════════
 * Private Variables
 * ═══════════════════════════════════════════════════════════════════════════
 */

static SensorFusionConfig_t g_config;
static bool g_initialized = false;

/* Sensor data buffers */
static struct {
    LSM6DSO_AccelData_t accel_history[ML_WINDOW_SIZE];
    LSM6DSO_GyroData_t  gyro_history[ML_WINDOW_SIZE];
    float               altitude_history[50];  /* 5 seconds @ 10Hz */
    uint16_t            imu_idx;
    uint16_t            altitude_idx;
} g_buffers;

/* Statistics */
static struct {
    uint32_t total_steps;
    float    total_distance_m;
    float    total_calories;
    uint32_t activity_duration_ms[8];  /* Duration of each activity */
    uint32_t session_start_time;
} g_stats;

/* Fall detection state */
static struct {
    FallStatus_t status;
    uint32_t     fall_timestamp;
    uint32_t     immobility_start_time;
    bool         free_fall_detected;
    bool         impact_detected;
} g_fall_state;

/* Reference values */
static float g_reference_altitude_m = 0.0f;
static float g_last_valid_speed_kmh = 0.0f;

/* TensorFlow Lite Micro model (placeholder for actual model) */
extern void* g_ml_model;  /* Defined in ml_model.c */

/* ═══════════════════════════════════════════════════════════════════════════
 * Private Function Prototypes
 * ═══════════════════════════════════════════════════════════════════════════
 */

static void update_imu_buffers(const LSM6DSO_AccelData_t *accel, const LSM6DSO_GyroData_t *gyro);
static void update_altitude_buffer(float altitude);
static float calculate_accel_magnitude(const LSM6DSO_AccelData_t *accel);
static float calculate_gyro_magnitude(const LSM6DSO_GyroData_t *gyro);
static void extract_statistical_features(MLFeatureVector_t *features);
static Activity_t run_ml_inference(const MLFeatureVector_t *features, float *confidence);
static void update_fall_detection(float accel_magnitude);
static void update_statistics(const FusedSensorData_t *fused, uint32_t dt_ms);

/* ═══════════════════════════════════════════════════════════════════════════
 * Public API Implementation
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Initialize sensor fusion engine
 */
int SensorFusion_Init(const SensorFusionConfig_t *config)
{
    printf("[SensorFusion] Initializing...\n");

    /* Use defaults if no config provided */
    if (config != NULL) {
        g_config = *config;
    } else {
        g_config.enable_gps_validation = true;
        g_config.enable_fall_detection = true;
        g_config.enable_stair_detection = true;
        g_config.enable_route_tracking = true;
        g_config.update_rate_ms = 100;
    }

    /* Reset buffers */
    memset(&g_buffers, 0, sizeof(g_buffers));
    memset(&g_stats, 0, sizeof(g_stats));
    memset(&g_fall_state, 0, sizeof(g_fall_state));

    g_stats.session_start_time = get_timestamp_ms();

    g_initialized = true;

    printf("[SensorFusion] ✓ Initialization complete\n");
    printf("[SensorFusion]   GPS validation: %s\n", g_config.enable_gps_validation ? "ON" : "OFF");
    printf("[SensorFusion]   Fall detection: %s\n", g_config.enable_fall_detection ? "ON" : "OFF");
    printf("[SensorFusion]   Stair detection: %s\n", g_config.enable_stair_detection ? "ON" : "OFF");
    printf("[SensorFusion]   Update rate: %d ms\n", g_config.update_rate_ms);

    return 0;
}

/**
 * @brief Update sensor fusion with new readings
 */
int SensorFusion_Update(FusedSensorData_t *fused)
{
    if (!g_initialized || fused == NULL) {
        return -1;
    }

    static uint32_t last_update_time = 0;
    uint32_t current_time = get_timestamp_ms();
    uint32_t dt_ms = current_time - last_update_time;

    /* Initialize timestamp */
    fused->timestamp_ms = current_time;

    /* ─────────────────────────────────────────────────────────────────────
     * 1. Read all sensors
     * ───────────────────────────────────────────────────────────────────── */

    /* IMU (accelerometer + gyroscope) */
    LSM6DSO_AccelData_t accel;
    LSM6DSO_GyroData_t gyro;
    LSM6DSO_ReadAccel(&accel);
    LSM6DSO_ReadGyro(&gyro);
    update_imu_buffers(&accel, &gyro);

    /* Magnetometer (compass) */
    LIS2MDL_MagData_t mag;
    LIS2MDL_ReadMag(&mag);
    fused->heading_degrees = mag.heading;
    fused->direction_name = LIS2MDL_GetDirectionName(mag.heading);

    /* Barometer (altitude) */
    LPS22HH_Data_t baro;
    LPS22HH_ReadData(&baro);
    fused->altitude_m = baro.altitude_m;
    fused->temperature_c = baro.temperature_c;
    fused->pressure_hPa = baro.pressure_hPa;
    update_altitude_buffer(baro.altitude_m);

    /* GPS (location + speed) */
    A7672_GPSData_t gps;
    A7672_Status_t gps_status = A7672_GPSReadData(&gps);
    fused->gps_valid = (gps_status == A7672_OK && gps.fix_quality > 0);
    if (fused->gps_valid) {
        fused->latitude = gps.latitude;
        fused->longitude = gps.longitude;
        fused->speed_kmh = gps.speed_kmh;
        fused->satellites = gps.satellites;
        g_last_valid_speed_kmh = gps.speed_kmh;
    } else {
        fused->speed_kmh = 0.0f;
        fused->satellites = 0;
    }

    /* ─────────────────────────────────────────────────────────────────────
     * 2. Calculate derived metrics
     * ───────────────────────────────────────────────────────────────────── */

    /* Altitude change (last 5 seconds) */
    if (g_buffers.altitude_idx > 0) {
        fused->altitude_change_5s = fused->altitude_m -
            g_buffers.altitude_history[(g_buffers.altitude_idx - 1) % 50];
    } else {
        fused->altitude_change_5s = 0.0f;
    }

    /* Floor and stair detection */
    if (g_config.enable_stair_detection) {
        fused->floors_climbed = LPS22HH_AltitudeToFloor(
            fused->altitude_m - g_reference_altitude_m, 3.5f);

        if (fabsf(fused->altitude_change_5s) > STAIR_ALTITUDE_MIN_M) {
            fused->stairs_climbed = LPS22HH_DetectStairClimbing(
                g_reference_altitude_m, fused->altitude_m, 5);
        } else {
            fused->stairs_climbed = 0;
        }
    }

    /* Step counter */
    LSM6DSO_GetStepCount(&fused->total_steps);
    if (dt_ms > 0) {
        fused->steps_per_min = (fused->total_steps - g_stats.total_steps) *
                                (60000.0f / dt_ms);
    }

    /* Distance calculation (from steps or GPS) */
    if (fused->gps_valid && dt_ms > 0) {
        fused->distance_m = g_stats.total_distance_m +
                            (fused->speed_kmh * 1000.0f / 3600.0f) * (dt_ms / 1000.0f);
    } else {
        /* Estimate from steps (average stride length: 0.7m) */
        fused->distance_m = fused->total_steps * 0.7f;
    }

    /* ─────────────────────────────────────────────────────────────────────
     * 3. ML activity classification
     * ───────────────────────────────────────────────────────────────────── */

    /* Extract ML features */
    MLFeatureVector_t features;
    SensorFusion_ExtractMLFeatures(&features);

    /* Run ML inference */
    fused->activity_ml = run_ml_inference(&features, &fused->confidence);

    /* GPS validation (override ML if GPS provides strong signal) */
    if (g_config.enable_gps_validation && fused->gps_valid) {
        SensorFusion_ValidateWithGPS(fused->activity_ml, &gps, &fused->activity_final);
    } else {
        fused->activity_final = fused->activity_ml;
    }

    /* ─────────────────────────────────────────────────────────────────────
     * 4. Fall detection
     * ───────────────────────────────────────────────────────────────────── */

    if (g_config.enable_fall_detection) {
        float accel_mag = calculate_accel_magnitude(&accel);
        update_fall_detection(accel_mag);
        fused->fall_status = g_fall_state.status;
        fused->fall_timestamp = g_fall_state.fall_timestamp;
    } else {
        fused->fall_status = FALL_NO_FALL;
    }

    /* ─────────────────────────────────────────────────────────────────────
     * 5. Indoor/outdoor detection
     * ───────────────────────────────────────────────────────────────────── */

    SensorFusion_DetectLocationContext(&fused->location_type);

    /* ─────────────────────────────────────────────────────────────────────
     * 6. Update statistics
     * ───────────────────────────────────────────────────────────────────── */

    update_statistics(fused, dt_ms);
    last_update_time = current_time;

    return 0;
}

/**
 * @brief Extract ML features from sensor history
 */
int SensorFusion_ExtractMLFeatures(MLFeatureVector_t *features)
{
    if (!g_initialized || features == NULL) {
        return -1;
    }

    /* Copy IMU data (100 samples) */
    for (int i = 0; i < ML_WINDOW_SIZE; i++) {
        features->accel_x[i] = g_buffers.accel_history[i].accel_x;
        features->accel_y[i] = g_buffers.accel_history[i].accel_y;
        features->accel_z[i] = g_buffers.accel_history[i].accel_z;
        features->gyro_x[i] = g_buffers.gyro_history[i].gyro_x;
        features->gyro_y[i] = g_buffers.gyro_history[i].gyro_y;
        features->gyro_z[i] = g_buffers.gyro_history[i].gyro_z;
    }

    /* Extract statistical features */
    extract_statistical_features(features);

    /* Add GPS features (if available) */
    features->gps_speed_kmh = g_last_valid_speed_kmh;

    if (g_buffers.altitude_idx > 0) {
        features->gps_altitude_m = g_buffers.altitude_history[
            (g_buffers.altitude_idx - 1) % 50];

        if (g_buffers.altitude_idx > 1) {
            features->gps_altitude_change =
                g_buffers.altitude_history[(g_buffers.altitude_idx - 1) % 50] -
                g_buffers.altitude_history[(g_buffers.altitude_idx - 2) % 50];
        }
    }

    /* Barometer feature (altitude variance for stair detection) */
    if (g_buffers.altitude_idx >= 10) {
        float mean = 0, variance = 0;
        for (int i = 0; i < 10; i++) {
            mean += g_buffers.altitude_history[(g_buffers.altitude_idx - 1 - i) % 50];
        }
        mean /= 10.0f;

        for (int i = 0; i < 10; i++) {
            float diff = g_buffers.altitude_history[(g_buffers.altitude_idx - 1 - i) % 50] - mean;
            variance += diff * diff;
        }
        variance /= 10.0f;
        features->altitude_variance = variance;
    } else {
        features->altitude_variance = 0.0f;
    }

    /* Magnetometer feature (heading change rate for turning detection) */
    /* Placeholder - would need heading history buffer */
    features->heading_change_rate = 0.0f;

    return 0;
}

/**
 * @brief Classify activity using ML model
 */
int SensorFusion_ClassifyActivity(const MLFeatureVector_t *features,
                                   Activity_t *activity, float *confidence)
{
    if (features == NULL || activity == NULL || confidence == NULL) {
        return -1;
    }

    /* This is a placeholder - actual implementation would use TFLite */
    *activity = run_ml_inference(features, confidence);

    return 0;
}

/**
 * @brief Validate ML prediction with GPS
 */
void SensorFusion_ValidateWithGPS(Activity_t ml_activity,
                                   const A7672_GPSData_t *gps_data,
                                   Activity_t *validated_activity)
{
    if (gps_data == NULL || validated_activity == NULL) {
        *validated_activity = ml_activity;
        return;
    }

    *validated_activity = ml_activity;

    /* Rule 1: High speed → Driving */
    if (gps_data->speed_kmh > DRIVING_SPEED_MIN_KMH) {
        *validated_activity = ACTIVITY_DRIVING;
        return;
    }

    /* Rule 2: Cycling speed range */
    if (gps_data->speed_kmh >= CYCLING_SPEED_MIN_KMH &&
        gps_data->speed_kmh <= CYCLING_SPEED_MAX_KMH &&
        ml_activity == ACTIVITY_CYCLING) {
        /* Confirm cycling */
        return;
    }

    /* Rule 3: Running speed range */
    if (gps_data->speed_kmh >= RUNNING_SPEED_MIN_KMH &&
        gps_data->speed_kmh <= RUNNING_SPEED_MAX_KMH &&
        ml_activity == ACTIVITY_RUNNING) {
        /* Confirm running */
        return;
    }

    /* Rule 4: Walking speed range */
    if (gps_data->speed_kmh >= WALKING_SPEED_MIN_KMH &&
        gps_data->speed_kmh <= WALKING_SPEED_MAX_KMH &&
        ml_activity == ACTIVITY_WALKING) {
        /* Confirm walking */
        return;
    }

    /* Rule 5: Speed too high for predicted activity → override */
    if (gps_data->speed_kmh > WALKING_SPEED_MAX_KMH &&
        ml_activity == ACTIVITY_WALKING) {
        if (gps_data->speed_kmh < RUNNING_SPEED_MAX_KMH) {
            *validated_activity = ACTIVITY_RUNNING;
        } else if (gps_data->speed_kmh < CYCLING_SPEED_MAX_KMH) {
            *validated_activity = ACTIVITY_CYCLING;
        } else {
            *validated_activity = ACTIVITY_DRIVING;
        }
    }
}

/**
 * @brief Detect falls
 */
int SensorFusion_DetectFall(FallStatus_t *fall_status)
{
    if (!g_initialized || fall_status == NULL) {
        return -1;
    }

    *fall_status = g_fall_state.status;
    return 0;
}

/**
 * @brief Detect indoor/outdoor location
 */
int SensorFusion_DetectLocationContext(LocationType_t *location_type)
{
    if (!g_initialized || location_type == NULL) {
        return -1;
    }

    /* Check GPS signal strength */
    A7672_GPSData_t gps;
    A7672_Status_t status = A7672_GPSReadData(&gps);
    bool gps_weak = (status != A7672_OK || gps.satellites < INDOOR_GPS_SAT_THRESHOLD);

    /* Check for magnetic field anomalies (indoor = near metal structures) */
    LIS2MDL_MagData_t mag;
    LIS2MDL_ReadMag(&mag);
    float expected_field = 500.0f;  /* Earth's field ~500 mGauss */
    float field_deviation = fabsf(mag.magnitude - expected_field);
    bool magnetic_anomaly = (field_deviation > INDOOR_MAGNETIC_ANOMALY);

    /* Decision logic */
    if (gps_weak && magnetic_anomaly) {
        *location_type = LOCATION_INDOOR;
    } else if (!gps_weak) {
        *location_type = LOCATION_OUTDOOR;
    } else {
        *location_type = LOCATION_UNKNOWN;
    }

    return 0;
}

/**
 * @brief Calculate calories burned
 */
float SensorFusion_CalculateCalories(Activity_t activity,
                                      float duration_min,
                                      float weight_kg)
{
    /* MET (Metabolic Equivalent of Task) formula:
     * Calories = MET × weight_kg × duration_hours
     */

    float met = MET_IDLE;

    switch (activity) {
        case ACTIVITY_IDLE:     met = MET_IDLE;    break;
        case ACTIVITY_WALKING:  met = MET_WALKING; break;
        case ACTIVITY_RUNNING:  met = MET_RUNNING; break;
        case ACTIVITY_CYCLING:  met = MET_CYCLING; break;
        case ACTIVITY_STAIRS:   met = MET_STAIRS;  break;
        case ACTIVITY_HIKING:   met = MET_HIKING;  break;
        case ACTIVITY_DRIVING:  met = MET_IDLE;    break;  /* Minimal effort */
        default:                met = MET_IDLE;    break;
    }

    float duration_hours = duration_min / 60.0f;
    float calories = met * weight_kg * duration_hours;

    return calories;
}

/**
 * @brief Recommend route based on history
 */
int SensorFusion_RecommendRoute(const A7672_GPSData_t *current_location,
                                float *recommended_heading)
{
    if (!g_initialized || current_location == NULL || recommended_heading == NULL) {
        return -1;
    }

    /* Placeholder - would analyze route history from TF-M PS */
    /* For now, return current heading */
    LIS2MDL_MagData_t mag;
    LIS2MDL_ReadMag(&mag);
    *recommended_heading = mag.heading;

    return 0;
}

/**
 * @brief Reset statistics
 */
void SensorFusion_ResetStats(void)
{
    memset(&g_stats, 0, sizeof(g_stats));
    g_stats.session_start_time = get_timestamp_ms();
    LSM6DSO_ResetStepCounter();
    printf("[SensorFusion] Statistics reset\n");
}

/**
 * @brief Get activity name
 */
const char* SensorFusion_GetActivityName(Activity_t activity)
{
    switch (activity) {
        case ACTIVITY_IDLE:     return "Idle";
        case ACTIVITY_WALKING:  return "Walking";
        case ACTIVITY_RUNNING:  return "Running";
        case ACTIVITY_CYCLING:  return "Cycling";
        case ACTIVITY_DRIVING:  return "Driving";
        case ACTIVITY_STAIRS:   return "Stairs";
        case ACTIVITY_HIKING:   return "Hiking";
        default:                return "Unknown";
    }
}

/**
 * @brief Get fall status name
 */
const char* SensorFusion_GetFallStatusName(FallStatus_t status)
{
    switch (status) {
        case FALL_NO_FALL:      return "No fall";
        case FALL_SUSPECTED:    return "Fall suspected";
        case FALL_CONFIRMED:    return "Fall detected!";
        case FALL_FALSE_ALARM:  return "False alarm";
        default:                return "Unknown";
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * TF-M Storage Functions
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifdef USE_TFM_STORAGE

/**
 * @brief Save fused data to TF-M PS
 */
int SensorFusion_SaveToStorage(const FusedSensorData_t *data)
{
    if (data == NULL) {
        return -1;
    }

    psa_status_t status = psa_ps_set(TFM_FUSED_DATA_UID, sizeof(FusedSensorData_t),
                                      data, PSA_STORAGE_FLAG_NONE);

    if (status == PSA_SUCCESS) {
        return 0;
    } else {
        printf("[SensorFusion] ⚠️  Failed to save data (0x%08lX)\n", status);
        return -1;
    }
}

/**
 * @brief Load data from TF-M PS
 */
int SensorFusion_LoadFromStorage(FusedSensorData_t *data, uint16_t max_entries,
                                  uint16_t *entries_loaded)
{
    if (data == NULL || entries_loaded == NULL) {
        return -1;
    }

    size_t data_len;
    psa_status_t status = psa_ps_get(TFM_FUSED_DATA_UID, 0, sizeof(FusedSensorData_t),
                                      data, &data_len);

    if (status == PSA_SUCCESS) {
        *entries_loaded = 1;
        return 0;
    } else {
        *entries_loaded = 0;
        return -1;
    }
}

/**
 * @brief Generate daily summary
 */
int SensorFusion_GenerateDailySummary(char *summary_json, uint16_t max_len)
{
    if (summary_json == NULL || max_len == 0) {
        return -1;
    }

    /* Get current date */
    uint32_t timestamp = get_timestamp_ms();

    /* Calculate totals */
    float total_calories = g_stats.total_calories;
    float total_distance_km = g_stats.total_distance_m / 1000.0f;
    uint32_t total_steps = g_stats.total_steps;

    /* Generate JSON */
    int len = snprintf(summary_json, max_len,
        "{\n"
        "  \"date\": \"%lu\",\n"
        "  \"steps\": %lu,\n"
        "  \"distance_km\": %.2f,\n"
        "  \"calories\": %.0f,\n"
        "  \"activities\": {\n"
        "    \"idle\": %lu,\n"
        "    \"walking\": %lu,\n"
        "    \"running\": %lu,\n"
        "    \"cycling\": %lu,\n"
        "    \"driving\": %lu,\n"
        "    \"stairs\": %lu,\n"
        "    \"hiking\": %lu\n"
        "  }\n"
        "}",
        timestamp,
        total_steps,
        total_distance_km,
        total_calories,
        g_stats.activity_duration_ms[ACTIVITY_IDLE] / 60000,
        g_stats.activity_duration_ms[ACTIVITY_WALKING] / 60000,
        g_stats.activity_duration_ms[ACTIVITY_RUNNING] / 60000,
        g_stats.activity_duration_ms[ACTIVITY_CYCLING] / 60000,
        g_stats.activity_duration_ms[ACTIVITY_DRIVING] / 60000,
        g_stats.activity_duration_ms[ACTIVITY_STAIRS] / 60000,
        g_stats.activity_duration_ms[ACTIVITY_HIKING] / 60000
    );

    if (len >= max_len) {
        return -1;  /* Buffer too small */
    }

    /* Save to TF-M PS */
    psa_ps_set(TFM_DAILY_SUMMARY_UID, len, summary_json, PSA_STORAGE_FLAG_NONE);

    return 0;
}

#else
/* No TF-M storage */
int SensorFusion_SaveToStorage(const FusedSensorData_t *data) { return -1; }
int SensorFusion_LoadFromStorage(FusedSensorData_t *data, uint16_t max_entries,
                                  uint16_t *entries_loaded) { return -1; }
int SensorFusion_GenerateDailySummary(char *summary_json, uint16_t max_len) { return -1; }
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 * Private Helper Functions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Update IMU circular buffers
 */
static void update_imu_buffers(const LSM6DSO_AccelData_t *accel,
                                const LSM6DSO_GyroData_t *gyro)
{
    g_buffers.accel_history[g_buffers.imu_idx] = *accel;
    g_buffers.gyro_history[g_buffers.imu_idx] = *gyro;
    g_buffers.imu_idx = (g_buffers.imu_idx + 1) % ML_WINDOW_SIZE;
}

/**
 * @brief Update altitude circular buffer
 */
static void update_altitude_buffer(float altitude)
{
    g_buffers.altitude_history[g_buffers.altitude_idx % 50] = altitude;
    g_buffers.altitude_idx++;
}

/**
 * @brief Calculate acceleration magnitude
 */
static float calculate_accel_magnitude(const LSM6DSO_AccelData_t *accel)
{
    return sqrtf(accel->accel_x * accel->accel_x +
                 accel->accel_y * accel->accel_y +
                 accel->accel_z * accel->accel_z);
}

/**
 * @brief Calculate gyroscope magnitude
 */
static float calculate_gyro_magnitude(const LSM6DSO_GyroData_t *gyro)
{
    return sqrtf(gyro->gyro_x * gyro->gyro_x +
                 gyro->gyro_y * gyro->gyro_y +
                 gyro->gyro_z * gyro->gyro_z);
}

/**
 * @brief Extract statistical features from IMU data
 */
static void extract_statistical_features(MLFeatureVector_t *features)
{
    float accel_sum = 0, gyro_sum = 0;

    /* Calculate means */
    for (int i = 0; i < ML_WINDOW_SIZE; i++) {
        accel_sum += calculate_accel_magnitude(&g_buffers.accel_history[i]);
        gyro_sum += calculate_gyro_magnitude(&g_buffers.gyro_history[i]);
    }

    features->accel_magnitude_mean = accel_sum / ML_WINDOW_SIZE;
    features->gyro_magnitude_mean = gyro_sum / ML_WINDOW_SIZE;

    /* Calculate variances */
    float accel_var_sum = 0, gyro_var_sum = 0;
    for (int i = 0; i < ML_WINDOW_SIZE; i++) {
        float accel_diff = calculate_accel_magnitude(&g_buffers.accel_history[i]) -
                           features->accel_magnitude_mean;
        float gyro_diff = calculate_gyro_magnitude(&g_buffers.gyro_history[i]) -
                          features->gyro_magnitude_mean;
        accel_var_sum += accel_diff * accel_diff;
        gyro_var_sum += gyro_diff * gyro_diff;
    }

    features->accel_magnitude_variance = accel_var_sum / ML_WINDOW_SIZE;
    features->gyro_magnitude_variance = gyro_var_sum / ML_WINDOW_SIZE;
}

/**
 * @brief Run ML inference (placeholder)
 */
static Activity_t run_ml_inference(const MLFeatureVector_t *features, float *confidence)
{
    /* This is a simplified rule-based classifier
     * In production, this would use TensorFlow Lite Micro
     */

    float accel_mag = features->accel_magnitude_mean;
    float gyro_mag = features->gyro_magnitude_mean;
    float speed = features->gps_speed_kmh;

    *confidence = 0.85f;  /* Default confidence */

    /* Rule-based classification */
    if (accel_mag < IDLE_THRESHOLD_MS2) {
        return ACTIVITY_IDLE;
    }

    if (features->altitude_variance > 0.5f) {
        return ACTIVITY_STAIRS;
    }

    if (speed > DRIVING_SPEED_MIN_KMH) {
        return ACTIVITY_DRIVING;
    }

    if (speed > CYCLING_SPEED_MIN_KMH && gyro_mag > 50.0f) {
        return ACTIVITY_CYCLING;
    }

    if (speed > RUNNING_SPEED_MIN_KMH && accel_mag > 15.0f) {
        return ACTIVITY_RUNNING;
    }

    if (speed > WALKING_SPEED_MIN_KMH && accel_mag > 5.0f) {
        return ACTIVITY_WALKING;
    }

    return ACTIVITY_IDLE;
}

/**
 * @brief Update fall detection state machine
 */
static void update_fall_detection(float accel_magnitude)
{
    uint32_t current_time = get_timestamp_ms();

    switch (g_fall_state.status) {
        case FALL_NO_FALL:
            /* Check for free fall */
            if (accel_magnitude < FALL_FREE_FALL_THRESHOLD) {
                g_fall_state.free_fall_detected = true;
            }

            /* Check for impact */
            if (g_fall_state.free_fall_detected &&
                accel_magnitude > FALL_IMPACT_THRESHOLD) {
                g_fall_state.status = FALL_SUSPECTED;
                g_fall_state.impact_detected = true;
                g_fall_state.immobility_start_time = current_time;
                g_fall_state.fall_timestamp = current_time;
                printf("[SensorFusion] ⚠️  Fall suspected!\n");
            }
            break;

        case FALL_SUSPECTED:
            /* Check for continued immobility */
            if (accel_magnitude < FALL_IMMOBILITY_THRESHOLD) {
                uint32_t immobile_duration = current_time - g_fall_state.immobility_start_time;

                if (immobile_duration >= FALL_IMMOBILITY_TIME_MS) {
                    g_fall_state.status = FALL_CONFIRMED;
                    printf("[SensorFusion] 🚨 FALL CONFIRMED!\n");
                }
            } else {
                /* Movement detected - false alarm */
                g_fall_state.status = FALL_FALSE_ALARM;
                printf("[SensorFusion] False alarm - movement detected\n");
            }
            break;

        case FALL_CONFIRMED:
            /* Stay in this state until manually reset or movement detected */
            if (accel_magnitude > FALL_IMMOBILITY_THRESHOLD * 2) {
                g_fall_state.status = FALL_NO_FALL;
                g_fall_state.free_fall_detected = false;
                g_fall_state.impact_detected = false;
                printf("[SensorFusion] Fall state cleared - user moving\n");
            }
            break;

        case FALL_FALSE_ALARM:
            /* Reset after 5 seconds */
            if (current_time - g_fall_state.fall_timestamp > 5000) {
                g_fall_state.status = FALL_NO_FALL;
                g_fall_state.free_fall_detected = false;
                g_fall_state.impact_detected = false;
            }
            break;
    }
}

/**
 * @brief Update session statistics
 */
static void update_statistics(const FusedSensorData_t *fused, uint32_t dt_ms)
{
    /* Update activity duration */
    if (fused->activity_final < 8) {
        g_stats.activity_duration_ms[fused->activity_final] += dt_ms;
    }

    /* Update distance */
    g_stats.total_distance_m = fused->distance_m;

    /* Update steps */
    g_stats.total_steps = fused->total_steps;

    /* Calculate calories for this interval */
    if (dt_ms > 0) {
        float duration_min = dt_ms / 60000.0f;
        float calories = SensorFusion_CalculateCalories(
            fused->activity_final, duration_min, 70.0f);  /* Assume 70kg */
        g_stats.total_calories += calories;
    }
}

/**
 * @brief Get current timestamp in milliseconds
 * @note This function should be implemented based on your platform's timer
 */
__attribute__((weak)) uint32_t get_timestamp_ms(void)
{
    /* Placeholder - implement with platform-specific timer */
    static uint32_t counter = 0;
    return counter++;
}

/**
 ******************************************************************************
 * @file    sensor_fusion.h
 * @brief   Multi-Sensor Fusion Engine Header
 * @details Combines data from all sensors for enhanced ML activity recognition
 *
 * Sensor Suite:
 * - LSM6DSO: 6-axis IMU (accelerometer + gyroscope)
 * - LIS2MDL: 3-axis magnetometer (compass heading)
 * - LPS22HH: Barometer (altitude tracking)
 * - SimCom A7672SA: GPS + 4G LTE
 *
 * Features:
 * - Multi-sensor data fusion
 * - Activity validation (GPS vs ML)
 * - Enhanced ML feature extraction
 * - Fall detection
 * - Route recommendation
 * - Indoor/outdoor detection
 *
 * TF-M Integration:
 * - Fused data encrypted before storage
 * - GPS coordinates stored in PSA PS
 * - Activity history secured
 *
 ******************************************************************************
 */

#ifndef SENSOR_FUSION_H
#define SENSOR_FUSION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "lsm6dso_driver.h"
#include "lis2mdl_driver.h"
#include "lps22hh_driver.h"
#include "simcom_a7672sa_driver.h"

/* ═══════════════════════════════════════════════════════════════════════════
 * Type Definitions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Activity types (ML classification + GPS validation)
 */
typedef enum {
    ACTIVITY_IDLE       = 0,  /* Standing still or sitting */
    ACTIVITY_WALKING    = 1,  /* Walking (1-7 km/h) */
    ACTIVITY_RUNNING    = 2,  /* Running (7-20 km/h) */
    ACTIVITY_CYCLING    = 3,  /* Cycling (10-30 km/h) */
    ACTIVITY_DRIVING    = 4,  /* Driving (>30 km/h) */
    ACTIVITY_STAIRS     = 5,  /* Climbing/descending stairs */
    ACTIVITY_HIKING     = 6,  /* Hiking (altitude change + walking) */
    ACTIVITY_UNKNOWN    = 7
} Activity_t;

/**
 * @brief Fall detection status
 */
typedef enum {
    FALL_NO_FALL        = 0,
    FALL_SUSPECTED      = 1,  /* High acceleration detected */
    FALL_CONFIRMED      = 2,  /* Impact + no movement for 5 seconds */
    FALL_FALSE_ALARM    = 3   /* Motion resumed */
} FallStatus_t;

/**
 * @brief Indoor/outdoor detection
 */
typedef enum {
    LOCATION_UNKNOWN    = 0,
    LOCATION_INDOOR     = 1,  /* GPS weak, magnetic anomaly */
    LOCATION_OUTDOOR    = 2   /* GPS strong, normal magnetic field */
} LocationType_t;

/**
 * @brief Fused sensor data
 */
typedef struct {
    /* Activity classification */
    Activity_t activity_ml;         /* ML model prediction */
    Activity_t activity_final;      /* Final (validated by GPS) */
    float      confidence;          /* ML confidence (0.0-1.0) */

    /* Motion metrics */
    float      steps_per_min;       /* Cadence */
    uint32_t   total_steps;         /* Step counter */
    float      speed_kmh;           /* Speed from GPS */
    float      distance_m;          /* Distance traveled */

    /* Orientation */
    float      heading_degrees;     /* Compass heading (0-360°) */
    const char *direction_name;     /* Cardinal direction (N, NE, E, ...) */

    /* Altitude tracking */
    float      altitude_m;          /* Current altitude */
    float      altitude_change_5s;  /* Change in last 5 seconds */
    int        floors_climbed;      /* Estimated floors */
    int        stairs_climbed;      /* Estimated stairs */

    /* GPS data */
    bool       gps_valid;           /* GPS fix acquired */
    float      latitude;            /* Degrees */
    float      longitude;           /* Degrees */
    uint8_t    satellites;          /* Number of satellites */

    /* Environmental */
    float      temperature_c;       /* Ambient temperature */
    float      pressure_hPa;        /* Atmospheric pressure */

    /* Fall detection */
    FallStatus_t fall_status;       /* Fall detection state */
    uint32_t     fall_timestamp;    /* Time of fall (if detected) */

    /* Location context */
    LocationType_t location_type;   /* Indoor/outdoor */

    /* Timestamp */
    uint32_t   timestamp_ms;        /* System timestamp */
} FusedSensorData_t;

/**
 * @brief ML feature vector (input to TensorFlow Lite model)
 */
typedef struct {
    /* IMU features (100 samples × 6 axes = 600 features) */
    float accel_x[100];
    float accel_y[100];
    float accel_z[100];
    float gyro_x[100];
    float gyro_y[100];
    float gyro_z[100];

    /* Statistical features */
    float accel_magnitude_mean;
    float accel_magnitude_variance;
    float gyro_magnitude_mean;
    float gyro_magnitude_variance;

    /* GPS features */
    float gps_speed_kmh;
    float gps_altitude_m;
    float gps_altitude_change;

    /* Barometer features */
    float altitude_variance;        /* Stairs detection */

    /* Magnetometer features */
    float heading_change_rate;      /* Turning detection */
} MLFeatureVector_t;

/**
 * @brief Sensor fusion configuration
 */
typedef struct {
    bool enable_gps_validation;     /* Validate ML with GPS */
    bool enable_fall_detection;     /* Monitor for falls */
    bool enable_stair_detection;    /* Detect stair climbing */
    bool enable_route_tracking;     /* Track GPS route */
    uint16_t update_rate_ms;        /* Fusion update rate */
} SensorFusionConfig_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * Public API Functions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Initialize sensor fusion engine
 * @param config Pointer to configuration (NULL for defaults)
 * @return 0 on success
 *
 * @example
 * SensorFusionConfig_t config = {
 *     .enable_gps_validation = true,
 *     .enable_fall_detection = true,
 *     .enable_stair_detection = true,
 *     .enable_route_tracking = true,
 *     .update_rate_ms = 100
 * };
 * SensorFusion_Init(&config);
 */
int SensorFusion_Init(const SensorFusionConfig_t *config);

/**
 * @brief Update sensor fusion with new sensor readings
 * @param fused Pointer to output fused data structure
 * @return 0 on success
 *
 * @note Call this function at regular intervals (e.g., every 100ms)
 *
 * @example
 * FusedSensorData_t fused;
 * SensorFusion_Update(&fused);
 * printf("Activity: %s\n", SensorFusion_GetActivityName(fused.activity_final));
 * printf("Speed: %.1f km/h\n", fused.speed_kmh);
 * printf("Heading: %.1f° (%s)\n", fused.heading_degrees, fused.direction_name);
 */
int SensorFusion_Update(FusedSensorData_t *fused);

/**
 * @brief Extract ML features from sensor data
 * @param features Pointer to output feature vector
 * @return 0 on success
 *
 * @note Call this after collecting 100 IMU samples (~2 seconds @ 50Hz)
 */
int SensorFusion_ExtractMLFeatures(MLFeatureVector_t *features);

/**
 * @brief Classify activity using ML model
 * @param features Pointer to feature vector
 * @param activity Output: Predicted activity
 * @param confidence Output: Confidence (0.0-1.0)
 * @return 0 on success
 *
 * @example
 * MLFeatureVector_t features;
 * SensorFusion_ExtractMLFeatures(&features);
 *
 * Activity_t activity;
 * float confidence;
 * SensorFusion_ClassifyActivity(&features, &activity, &confidence);
 *
 * if (confidence > 0.8) {
 *     printf("High confidence: %s (%.1f%%)\n",
 *            SensorFusion_GetActivityName(activity), confidence * 100);
 * }
 */
int SensorFusion_ClassifyActivity(const MLFeatureVector_t *features,
                                   Activity_t *activity, float *confidence);

/**
 * @brief Validate ML prediction with GPS data
 * @param ml_activity ML predicted activity
 * @param gps_data GPS data
 * @param validated_activity Output: Validated activity
 *
 * @example
 * // ML says "walking" but GPS shows 40 km/h → change to "driving"
 * Activity_t ml_pred = ACTIVITY_WALKING;
 * A7672_GPSData_t gps;
 * A7672_GPSReadData(&gps);
 *
 * Activity_t final_activity;
 * SensorFusion_ValidateWithGPS(ml_pred, &gps, &final_activity);
 */
void SensorFusion_ValidateWithGPS(Activity_t ml_activity,
                                   const A7672_GPSData_t *gps_data,
                                   Activity_t *validated_activity);

/**
 * @brief Detect falls using accelerometer
 * @param fall_status Output: Fall detection status
 * @return 0 on success
 *
 * @note Monitors for:
 * - Sudden high acceleration (free fall)
 * - Impact (>4g)
 * - Immobility after impact (>5 seconds)
 */
int SensorFusion_DetectFall(FallStatus_t *fall_status);

/**
 * @brief Detect indoor/outdoor location
 * @param location_type Output: Location type
 * @return 0 on success
 *
 * @note Uses:
 * - GPS signal strength
 * - Magnetic field anomalies
 * - Pressure stability
 */
int SensorFusion_DetectLocationContext(LocationType_t *location_type);

/**
 * @brief Calculate calories burned
 * @param activity Current activity
 * @param duration_min Duration in minutes
 * @param weight_kg User weight in kg
 * @return Calories burned
 *
 * @example
 * float calories = SensorFusion_CalculateCalories(
 *     ACTIVITY_RUNNING,
 *     30,   // 30 minutes
 *     70    // 70 kg
 * );
 * printf("Burned %.0f calories\n", calories);
 */
float SensorFusion_CalculateCalories(Activity_t activity,
                                      float duration_min,
                                      float weight_kg);

/**
 * @brief Get recommended route based on activity history
 * @param current_location Current GPS coordinates
 * @param recommended_heading Output: Suggested heading (degrees)
 * @return 0 on success
 *
 * @note Analyzes past routes to suggest popular paths
 */
int SensorFusion_RecommendRoute(const A7672_GPSData_t *current_location,
                                float *recommended_heading);

/**
 * @brief Reset step counter and statistics
 */
void SensorFusion_ResetStats(void);

/**
 * @brief Get activity name string
 * @param activity Activity type
 * @return Activity name (e.g., "Walking", "Running")
 */
const char* SensorFusion_GetActivityName(Activity_t activity);

/**
 * @brief Get fall status string
 * @param status Fall status
 * @return Status string (e.g., "No fall", "Fall detected!")
 */
const char* SensorFusion_GetFallStatusName(FallStatus_t status);

/* ═══════════════════════════════════════════════════════════════════════════
 * Data Logging Functions (TF-M Integration)
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Save fused data to TF-M Protected Storage
 * @param data Pointer to fused data
 * @return 0 on success
 *
 * @note Data is encrypted before storage
 */
int SensorFusion_SaveToStorage(const FusedSensorData_t *data);

/**
 * @brief Load historical data from TF-M Protected Storage
 * @param data Array to store loaded data
 * @param max_entries Maximum number of entries to load
 * @param entries_loaded Output: Actual number of entries loaded
 * @return 0 on success
 */
int SensorFusion_LoadFromStorage(FusedSensorData_t *data, uint16_t max_entries,
                                  uint16_t *entries_loaded);

/**
 * @brief Generate daily activity summary
 * @param summary_json Output: JSON string with summary
 * @param max_len Maximum length of JSON string
 * @return 0 on success
 *
 * @example
 * char json[512];
 * SensorFusion_GenerateDailySummary(json, sizeof(json));
 * printf("%s\n", json);
 * // Output:
 * // {
 * //   "date": "2025-11-22",
 * //   "steps": 10523,
 * //   "distance_km": 7.8,
 * //   "calories": 485,
 * //   "activities": {
 * //     "walking": 45,
 * //     "running": 15,
 * //     "cycling": 30
 * //   }
 * // }
 */
int SensorFusion_GenerateDailySummary(char *summary_json, uint16_t max_len);

/* ═══════════════════════════════════════════════════════════════════════════
 * Usage Examples
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * Example 1: Basic Activity Tracking
 * ───────────────────────────────────
 * ```c
 * SensorFusion_Init(NULL);  // Use defaults
 *
 * while (1) {
 *     FusedSensorData_t fused;
 *     SensorFusion_Update(&fused);
 *
 *     printf("Activity: %s (%.0f%% confident)\n",
 *            SensorFusion_GetActivityName(fused.activity_final),
 *            fused.confidence * 100);
 *     printf("Steps: %lu | Speed: %.1f km/h | Heading: %s\n",
 *            fused.total_steps, fused.speed_kmh, fused.direction_name);
 *
 *     nrf_delay_ms(100);
 * }
 * ```
 *
 * Example 2: Fall Detection
 * ─────────────────────────
 * ```c
 * SensorFusionConfig_t config = {
 *     .enable_fall_detection = true,
 *     .update_rate_ms = 50  // High rate for fall detection
 * };
 * SensorFusion_Init(&config);
 *
 * while (1) {
 *     FusedSensorData_t fused;
 *     SensorFusion_Update(&fused);
 *
 *     if (fused.fall_status == FALL_CONFIRMED) {
 *         printf("⚠️  FALL DETECTED!\n");
 *         // Send alert to emergency contact
 *         send_fall_alert(fused.latitude, fused.longitude);
 *     }
 *
 *     nrf_delay_ms(50);
 * }
 * ```
 *
 * Example 3: GPS-Enhanced ML
 * ──────────────────────────
 * ```c
 * // Extract ML features
 * MLFeatureVector_t features;
 * SensorFusion_ExtractMLFeatures(&features);
 *
 * // Classify activity
 * Activity_t ml_activity;
 * float confidence;
 * SensorFusion_ClassifyActivity(&features, &ml_activity, &confidence);
 * printf("ML prediction: %s (%.0f%%)\n",
 *        SensorFusion_GetActivityName(ml_activity), confidence * 100);
 *
 * // Validate with GPS
 * A7672_GPSData_t gps;
 * A7672_GPSReadData(&gps);
 *
 * Activity_t final_activity;
 * SensorFusion_ValidateWithGPS(ml_activity, &gps, &final_activity);
 * printf("Validated activity: %s\n",
 *        SensorFusion_GetActivityName(final_activity));
 * ```
 *
 * Example 4: Stair Climbing Detection
 * ────────────────────────────────────
 * ```c
 * while (1) {
 *     FusedSensorData_t fused;
 *     SensorFusion_Update(&fused);
 *
 *     if (fused.activity_final == ACTIVITY_STAIRS) {
 *         printf("Stairs detected!\n");
 *         printf("  Altitude change: %.1f m\n", fused.altitude_change_5s);
 *         printf("  Floors: %d\n", fused.floors_climbed);
 *         printf("  Stairs: %d\n", fused.stairs_climbed);
 *     }
 *
 *     nrf_delay_ms(1000);
 * }
 * ```
 *
 * Example 5: Daily Summary
 * ────────────────────────
 * ```c
 * // At end of day (midnight)
 * char summary[512];
 * SensorFusion_GenerateDailySummary(summary, sizeof(summary));
 *
 * // Upload to cloud
 * A7672_HTTPPost(
 *     "https://api.fitness.com/daily",
 *     summary,
 *     strlen(summary),
 *     response,
 *     sizeof(response)
 * );
 *
 * // Display to user via BLE
 * ble_send_notification(summary);
 * ```
 */

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_FUSION_H */

/**
 * @file gps_parser.h
 * @brief GPS NMEA Sentence Parser
 *
 * Parses standard NMEA 0183 sentences from GPS modules:
 * - GGA: Global Positioning System Fix Data
 * - RMC: Recommended Minimum Specific GPS Data
 * - GSA: GPS DOP and Active Satellites
 * - GSV: GPS Satellites in View
 * - VTG: Track Made Good and Ground Speed
 *
 * @author TF-M Training Project
 * @date 2024
 */

#ifndef GPS_PARSER_H
#define GPS_PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Configuration
 ******************************************************************************/

#define GPS_MAX_SENTENCE_LEN    82      /**< Maximum NMEA sentence length */
#define GPS_MAX_SATELLITES      12      /**< Maximum satellites to track */

/*******************************************************************************
 * Data Types
 ******************************************************************************/

/**
 * @brief GPS fix quality indicator
 */
typedef enum {
    GPS_FIX_INVALID = 0,                /**< Invalid fix */
    GPS_FIX_GPS = 1,                    /**< GPS fix */
    GPS_FIX_DGPS = 2,                   /**< Differential GPS fix */
    GPS_FIX_PPS = 3,                    /**< PPS fix */
    GPS_FIX_RTK = 4,                    /**< Real-Time Kinematic */
    GPS_FIX_RTK_FLOAT = 5,              /**< Float RTK */
    GPS_FIX_ESTIMATED = 6,              /**< Estimated (dead reckoning) */
    GPS_FIX_MANUAL = 7,                 /**< Manual input mode */
    GPS_FIX_SIMULATION = 8              /**< Simulation mode */
} gps_fix_quality_t;

/**
 * @brief GPS fix mode (from GSA sentence)
 */
typedef enum {
    GPS_MODE_NOT_AVAILABLE = 1,         /**< Fix not available */
    GPS_MODE_2D = 2,                    /**< 2D fix */
    GPS_MODE_3D = 3                     /**< 3D fix */
} gps_fix_mode_t;

/**
 * @brief GPS position status
 */
typedef enum {
    GPS_STATUS_INVALID = 'V',           /**< Invalid / Warning */
    GPS_STATUS_VALID = 'A'              /**< Valid / Active */
} gps_status_t;

/**
 * @brief Cardinal direction
 */
typedef enum {
    GPS_DIR_NORTH = 'N',
    GPS_DIR_SOUTH = 'S',
    GPS_DIR_EAST = 'E',
    GPS_DIR_WEST = 'W'
} gps_direction_t;

/**
 * @brief Satellite information
 */
typedef struct {
    uint8_t prn;                        /**< Satellite PRN number */
    uint8_t elevation;                  /**< Elevation in degrees (0-90) */
    uint16_t azimuth;                   /**< Azimuth in degrees (0-359) */
    uint8_t snr;                        /**< Signal-to-Noise Ratio (dB) */
    bool used_in_fix;                   /**< Used in position fix */
} gps_satellite_t;

/**
 * @brief GGA sentence data (Global Positioning System Fix Data)
 *
 * Example: $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
 */
typedef struct {
    uint32_t utc_time;                  /**< UTC time (hhmmss) */
    double latitude;                    /**< Latitude in decimal degrees */
    char lat_direction;                 /**< 'N' or 'S' */
    double longitude;                   /**< Longitude in decimal degrees */
    char lon_direction;                 /**< 'E' or 'W' */
    gps_fix_quality_t fix_quality;      /**< Fix quality indicator */
    uint8_t satellites_used;            /**< Number of satellites */
    float hdop;                         /**< Horizontal Dilution of Precision */
    float altitude;                     /**< Altitude above MSL (meters) */
    float geoid_separation;             /**< Geoid separation (meters) */
    uint16_t dgps_age;                  /**< Age of DGPS data (seconds) */
    uint16_t dgps_station_id;           /**< DGPS station ID */
    bool valid;                         /**< Data validity flag */
} gps_gga_data_t;

/**
 * @brief RMC sentence data (Recommended Minimum Specific GPS Data)
 *
 * Example: $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
 */
typedef struct {
    uint32_t utc_time;                  /**< UTC time (hhmmss) */
    gps_status_t status;                /**< Status: 'A' = valid, 'V' = invalid */
    double latitude;                    /**< Latitude in decimal degrees */
    char lat_direction;                 /**< 'N' or 'S' */
    double longitude;                   /**< Longitude in decimal degrees */
    char lon_direction;                 /**< 'E' or 'W' */
    float speed_knots;                  /**< Speed over ground (knots) */
    float course;                       /**< Course over ground (degrees) */
    uint32_t date;                      /**< UTC date (ddmmyy) */
    float magnetic_variation;           /**< Magnetic variation (degrees) */
    char mag_var_direction;             /**< 'E' or 'W' */
    char mode_indicator;                /**< A=Autonomous, D=Differential, E=Estimated */
    bool valid;                         /**< Data validity flag */
} gps_rmc_data_t;

/**
 * @brief GSA sentence data (GPS DOP and Active Satellites)
 *
 * Example: $GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39
 */
typedef struct {
    char mode_selection;                /**< 'M' = Manual, 'A' = Automatic */
    gps_fix_mode_t fix_mode;            /**< 1=No fix, 2=2D, 3=3D */
    uint8_t satellite_prn[12];          /**< PRNs of satellites used in fix */
    uint8_t satellites_count;           /**< Number of satellites used */
    float pdop;                         /**< Position Dilution of Precision */
    float hdop;                         /**< Horizontal DOP */
    float vdop;                         /**< Vertical DOP */
    bool valid;                         /**< Data validity flag */
} gps_gsa_data_t;

/**
 * @brief GSV sentence data (GPS Satellites in View)
 *
 * Example: $GPGSV,3,1,11,03,03,111,00,04,15,270,00,06,01,010,00,13,06,292,00*74
 */
typedef struct {
    uint8_t total_messages;             /**< Total number of GSV messages */
    uint8_t message_number;             /**< This message number */
    uint8_t satellites_in_view;         /**< Total satellites in view */
    gps_satellite_t satellites[4];      /**< Up to 4 satellites per message */
    uint8_t satellite_count;            /**< Satellites in this message */
    bool valid;                         /**< Data validity flag */
} gps_gsv_data_t;

/**
 * @brief Complete GPS data (combined from multiple sentences)
 */
typedef struct {
    /* Position data (from GGA/RMC) */
    double latitude;                    /**< Latitude in decimal degrees */
    double longitude;                   /**< Longitude in decimal degrees */
    float altitude;                     /**< Altitude in meters */
    float speed_kmh;                    /**< Speed in km/h */
    float course;                       /**< Course in degrees */

    /* Quality metrics */
    gps_fix_quality_t fix_quality;      /**< Fix quality */
    gps_fix_mode_t fix_mode;            /**< Fix mode (2D/3D) */
    uint8_t satellites_used;            /**< Satellites used in fix */
    uint8_t satellites_in_view;         /**< Total satellites in view */
    float hdop;                         /**< Horizontal DOP */
    float vdop;                         /**< Vertical DOP */
    float pdop;                         /**< Position DOP */

    /* Time information */
    struct tm utc_time;                 /**< UTC time structure */
    uint64_t timestamp_ms;              /**< Unix timestamp in milliseconds */

    /* Status flags */
    bool position_valid;                /**< Position is valid */
    bool time_valid;                    /**< Time is valid */
    bool fix_3d;                        /**< 3D fix available */

    /* Satellite details */
    gps_satellite_t satellites[GPS_MAX_SATELLITES];
    uint8_t satellite_count;

    /* Last update time */
    uint64_t last_update_ms;            /**< Time of last update */
} gps_data_t;

/**
 * @brief NMEA sentence type
 */
typedef enum {
    NMEA_UNKNOWN = 0,
    NMEA_GGA,                           /**< Global Positioning System Fix Data */
    NMEA_RMC,                           /**< Recommended Minimum Specific GPS Data */
    NMEA_GSA,                           /**< GPS DOP and Active Satellites */
    NMEA_GSV,                           /**< GPS Satellites in View */
    NMEA_VTG,                           /**< Track Made Good and Ground Speed */
    NMEA_GLL                            /**< Geographic Position - Latitude/Longitude */
} nmea_sentence_type_t;

/*******************************************************************************
 * Functions
 ******************************************************************************/

/**
 * @brief Initialize GPS parser
 *
 * @param gps_data Pointer to GPS data structure to initialize
 */
void gps_parser_init(gps_data_t *gps_data);

/**
 * @brief Parse NMEA sentence
 *
 * Parses a complete NMEA sentence and updates the GPS data structure.
 *
 * @param sentence NMEA sentence string (with or without checksum)
 * @param gps_data Pointer to GPS data structure
 * @return Sentence type parsed, or NMEA_UNKNOWN if invalid
 *
 * @note The sentence string will be modified (null terminators added)
 */
nmea_sentence_type_t gps_parse_sentence(char *sentence, gps_data_t *gps_data);

/**
 * @brief Parse GGA sentence
 *
 * @param sentence GGA sentence (after $GPGGA,)
 * @param gga_data Pointer to GGA data structure
 * @return true if parsed successfully, false otherwise
 */
bool gps_parse_gga(const char *sentence, gps_gga_data_t *gga_data);

/**
 * @brief Parse RMC sentence
 *
 * @param sentence RMC sentence (after $GPRMC,)
 * @param rmc_data Pointer to RMC data structure
 * @return true if parsed successfully, false otherwise
 */
bool gps_parse_rmc(const char *sentence, gps_rmc_data_t *rmc_data);

/**
 * @brief Parse GSA sentence
 *
 * @param sentence GSA sentence (after $GPGSA,)
 * @param gsa_data Pointer to GSA data structure
 * @return true if parsed successfully, false otherwise
 */
bool gps_parse_gsa(const char *sentence, gps_gsa_data_t *gsa_data);

/**
 * @brief Parse GSV sentence
 *
 * @param sentence GSV sentence (after $GPGSV,)
 * @param gsv_data Pointer to GSV data structure
 * @return true if parsed successfully, false otherwise
 */
bool gps_parse_gsv(const char *sentence, gps_gsv_data_t *gsv_data);

/**
 * @brief Verify NMEA checksum
 *
 * @param sentence Complete NMEA sentence with checksum
 * @return true if checksum is valid, false otherwise
 */
bool gps_verify_checksum(const char *sentence);

/**
 * @brief Calculate NMEA checksum
 *
 * @param sentence NMEA sentence (without $ and checksum)
 * @return Calculated checksum
 */
uint8_t gps_calculate_checksum(const char *sentence);

/**
 * @brief Convert NMEA coordinate to decimal degrees
 *
 * Converts NMEA format (ddmm.mmmm or dddmm.mmmm) to decimal degrees.
 *
 * @param nmea_coord NMEA coordinate value
 * @param direction Cardinal direction ('N', 'S', 'E', 'W')
 * @return Decimal degrees (negative for South/West)
 */
double gps_nmea_to_decimal(double nmea_coord, char direction);

/**
 * @brief Convert knots to km/h
 *
 * @param knots Speed in knots
 * @return Speed in km/h
 */
float gps_knots_to_kmh(float knots);

/**
 * @brief Check if GPS data is fresh
 *
 * Determines if GPS data is recent enough to be trusted.
 *
 * @param gps_data GPS data structure
 * @param max_age_ms Maximum age in milliseconds
 * @param current_time_ms Current time in milliseconds
 * @return true if data is fresh, false otherwise
 */
bool gps_is_data_fresh(const gps_data_t *gps_data,
                       uint32_t max_age_ms,
                       uint64_t current_time_ms);

/**
 * @brief Get fix quality string
 *
 * @param quality Fix quality value
 * @return Human-readable string
 */
const char *gps_get_fix_quality_str(gps_fix_quality_t quality);

/**
 * @brief Get fix mode string
 *
 * @param mode Fix mode value
 * @return Human-readable string
 */
const char *gps_get_fix_mode_str(gps_fix_mode_t mode);

/**
 * @brief Calculate distance between two GPS points
 *
 * Uses Haversine formula to calculate great-circle distance.
 *
 * @param lat1 Latitude of point 1 (degrees)
 * @param lon1 Longitude of point 1 (degrees)
 * @param lat2 Latitude of point 2 (degrees)
 * @param lon2 Longitude of point 2 (degrees)
 * @return Distance in meters
 */
double gps_calculate_distance(double lat1, double lon1,
                              double lat2, double lon2);

/**
 * @brief Calculate bearing between two GPS points
 *
 * @param lat1 Latitude of point 1 (degrees)
 * @param lon1 Longitude of point 1 (degrees)
 * @param lat2 Latitude of point 2 (degrees)
 * @param lon2 Longitude of point 2 (degrees)
 * @return Bearing in degrees (0-360)
 */
double gps_calculate_bearing(double lat1, double lon1,
                             double lat2, double lon2);

/**
 * @brief Format GPS coordinates for display
 *
 * Formats coordinates as "DD°MM'SS.S"N/S/E/W"
 *
 * @param latitude Latitude in decimal degrees
 * @param longitude Longitude in decimal degrees
 * @param buffer Output buffer
 * @param buffer_len Buffer size
 */
void gps_format_coordinates(double latitude, double longitude,
                            char *buffer, size_t buffer_len);

#ifdef __cplusplus
}
#endif

#endif /* GPS_PARSER_H */

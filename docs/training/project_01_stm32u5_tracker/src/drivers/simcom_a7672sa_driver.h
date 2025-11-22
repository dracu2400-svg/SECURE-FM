/**
 ******************************************************************************
 * @file    simcom_a7672sa_driver.h
 * @brief   SimCom A7672SA 4G LTE + GPS Module Driver Header
 * @details Public API for SimCom A7672SA cellular and GPS functionality
 *
 * Quick Start Example:
 * ────────────────────
 * ```c
 * // 1. Initialize UART (115200 baud)
 * UART_HandleTypeDef huart1;
 * // ... UART init code ...
 *
 * // 2. Initialize module
 * A7672_Init(&huart1,
 *            GPIOB, GPIO_PIN_0,  // PWRKEY
 *            GPIOB, GPIO_PIN_1); // STATUS
 *
 * // 3. Power on
 * A7672_PowerOn();
 *
 * // 4. Connect to network
 * A7672_ConnectNetwork();
 *
 * // 5. Initialize GPS
 * A7672_GPSInit();
 *
 * // 6. Read GPS data
 * A7672_GPSData_t gps;
 * while (1) {
 *     if (A7672_GPSReadData(&gps) == A7672_OK) {
 *         printf("Location: %.6f, %.6f\n", gps.latitude, gps.longitude);
 *
 *         // Send to cloud via HTTP
 *         char json[256];
 *         snprintf(json, sizeof(json),
 *                  "{\"lat\":%.6f,\"lon\":%.6f,\"speed\":%.1f}",
 *                  gps.latitude, gps.longitude, gps.speed_kmh);
 *
 *         char response[512];
 *         A7672_HTTPPost("https://api.example.com/location",
 *                        json, strlen(json), response, sizeof(response));
 *     }
 *     HAL_Delay(60000);  // Update every minute
 * }
 * ```
 *
 ******************************************************************************
 */

#ifndef SIMCOM_A7672SA_DRIVER_H
#define SIMCOM_A7672SA_DRIVER_H

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
    A7672_OK                = 0,   /* Success */
    A7672_ERROR_COMM        = -1,  /* UART communication error */
    A7672_ERROR_TIMEOUT     = -2,  /* Operation timeout */
    A7672_ERROR_INVALID_PARAM = -3, /* Invalid parameter */
    A7672_ERROR_NOT_READY   = -4,  /* Module not ready */
    A7672_ERROR_NETWORK     = -5,  /* Network error */
    A7672_ERROR_GPS         = -6,  /* GPS error */
    A7672_ERROR_NO_FIX      = -7,  /* No GPS fix */
    A7672_ERROR_HTTP        = -8,  /* HTTP error */
    A7672_ERROR_SIM         = -9,  /* SIM card error */
    A7672_ERROR_AT_CMD      = -10, /* AT command error */
    A7672_ERROR_PARSE       = -11, /* Response parse error */
    A7672_ERROR_STORAGE     = -12  /* TF-M storage error */
} A7672_Status_t;

/**
 * @brief Module state
 */
typedef enum {
    A7672_STATE_POWER_OFF   = 0,   /* Module powered off */
    A7672_STATE_IDLE        = 1,   /* Module on, not configured */
    A7672_STATE_READY       = 2,   /* Module ready, no network */
    A7672_STATE_CONNECTED   = 3,   /* Connected to network */
    A7672_STATE_ERROR       = 4    /* Error state */
} A7672_ModuleState_t;

/**
 * @brief Network status information
 */
typedef struct {
    bool    registered;             /* Network registration status */
    int     rssi;                   /* Signal strength (0-31, 99=unknown) */
    int     ber;                    /* Bit error rate */
    char    operator_name[32];      /* Network operator name */
    uint8_t network_type;           /* 0=None, 1=2G, 2=3G, 3=4G */
} A7672_NetworkStatus_t;

/**
 * @brief GPS data structure
 */
typedef struct {
    float   latitude;               /* Latitude in decimal degrees */
    float   longitude;              /* Longitude in decimal degrees */
    float   altitude;               /* Altitude in meters (MSL) */
    float   speed_kmh;              /* Speed in km/h */
    float   course;                 /* Course over ground in degrees */
    uint8_t satellites;             /* Number of satellites in use */
    float   hdop;                   /* Horizontal dilution of precision */
    bool    fix_valid;              /* GPS fix valid flag */
    uint8_t fix_type;               /* 0=No fix, 1=GPS fix, 2=DGPS fix */
} A7672_GPSData_t;

/**
 * @brief TCP socket handle
 */
typedef struct {
    uint8_t socket_id;              /* Socket ID (0-11) */
    bool    is_connected;           /* Connection status */
    char    remote_addr[64];        /* Remote server address */
    uint16_t remote_port;           /* Remote server port */
} A7672_Socket_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * Public API Functions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Initialize A7672SA module
 * @param huart Pointer to initialized UART handle (115200 baud)
 * @param pwrkey_port GPIO port for PWRKEY pin
 * @param pwrkey_pin GPIO pin number for PWRKEY
 * @param status_port GPIO port for STATUS pin
 * @param status_pin GPIO pin number for STATUS
 * @return A7672_OK on success
 *
 * @example
 * A7672_Init(&huart1, GPIOB, GPIO_PIN_0, GPIOB, GPIO_PIN_1);
 */
A7672_Status_t A7672_Init(UART_HandleTypeDef *huart,
                          GPIO_TypeDef *pwrkey_port, uint16_t pwrkey_pin,
                          GPIO_TypeDef *status_port, uint16_t status_pin);

/**
 * @brief Power on the module
 * @return A7672_OK on success
 *
 * @note This function sends a power-on pulse and waits for boot (~15s)
 */
A7672_Status_t A7672_PowerOn(void);

/**
 * @brief Power off the module
 * @return A7672_OK on success
 */
A7672_Status_t A7672_PowerOff(void);

/**
 * @brief Connect to cellular network
 * @return A7672_OK on success
 *
 * @note This function:
 *       1. Checks SIM card status
 *       2. Configures APN (from TF-M ITS)
 *       3. Waits for network registration (up to 2 minutes)
 *       4. Activates PDP context
 *
 * @example
 * if (A7672_ConnectNetwork() == A7672_OK) {
 *     printf("Connected to network\n");
 * }
 */
A7672_Status_t A7672_ConnectNetwork(void);

/**
 * @brief Initialize GPS
 * @return A7672_OK on success
 *
 * @note GPS will start searching for satellites. First fix may take
 *       30-60 seconds (cold start) or 5-10 seconds (warm start)
 */
A7672_Status_t A7672_GPSInit(void);

/**
 * @brief Read GPS data
 * @param gps_data Pointer to output structure
 * @return A7672_OK on success, A7672_ERROR_NO_FIX if no GPS fix yet
 *
 * @example
 * A7672_GPSData_t gps;
 * if (A7672_GPSReadData(&gps) == A7672_OK) {
 *     printf("Location: %.6f, %.6f\n", gps.latitude, gps.longitude);
 *     printf("Altitude: %.1f m, Speed: %.1f km/h\n", gps.altitude, gps.speed_kmh);
 * } else {
 *     printf("Waiting for GPS fix...\n");
 * }
 */
A7672_Status_t A7672_GPSReadData(A7672_GPSData_t *gps_data);

/**
 * @brief Send HTTP GET request
 * @param url Full URL (e.g., "http://api.example.com/data")
 * @param response Buffer for response body
 * @param response_len Length of response buffer
 * @return A7672_OK on success
 *
 * @example
 * char response[1024];
 * if (A7672_HTTPGet("http://api.example.com/status", response, sizeof(response)) == A7672_OK) {
 *     printf("Response: %s\n", response);
 * }
 */
A7672_Status_t A7672_HTTPGet(const char *url, char *response, uint16_t response_len);

/**
 * @brief Send HTTP POST request
 * @param url Full URL
 * @param data Data to send (e.g., JSON payload)
 * @param data_len Length of data
 * @param response Buffer for response body
 * @param response_len Length of response buffer
 * @return A7672_OK on success
 *
 * @example
 * char json[] = "{\"lat\":37.7749,\"lon\":-122.4194,\"device\":\"tracker01\"}";
 * char response[512];
 * A7672_HTTPPost("https://api.example.com/location",
 *                json, strlen(json), response, sizeof(response));
 */
A7672_Status_t A7672_HTTPPost(const char *url, const char *data, uint16_t data_len,
                               char *response, uint16_t response_len);

/**
 * @brief Get signal quality
 * @param rssi Output: RSSI (0-31: -113 to -51 dBm, 99=unknown)
 * @param ber Output: Bit Error Rate (0-7, 99=unknown)
 * @return A7672_OK on success
 *
 * @note RSSI interpretation:
 *       0      = -113 dBm or less (very poor)
 *       1      = -111 dBm
 *       2-9    = -109 to -95 dBm (poor)
 *       10-14  = -93 to -85 dBm (fair)
 *       15-19  = -83 to -75 dBm (good)
 *       20-30  = -73 to -53 dBm (excellent)
 *       31     = -51 dBm or greater (excellent)
 *       99     = unknown
 *
 * @example
 * int rssi, ber;
 * A7672_GetSignalQuality(&rssi, &ber);
 * printf("Signal: RSSI=%d (%d dBm), BER=%d\n",
 *        rssi, -113 + (rssi * 2), ber);
 */
A7672_Status_t A7672_GetSignalQuality(int *rssi, int *ber);

/**
 * @brief UART RX callback (must be called from HAL_UART_RxCpltCallback)
 *
 * @example
 * void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
 * {
 *     if (huart == &huart1) {
 *         A7672_UART_RxCallback();
 *     }
 * }
 */
void A7672_UART_RxCallback(void);

/* ═══════════════════════════════════════════════════════════════════════════
 * Advanced Functions (Optional)
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Open TCP socket
 * @param socket Pointer to socket structure
 * @param server Server address (IP or domain)
 * @param port Server port
 * @return A7672_OK on success
 */
A7672_Status_t A7672_TCPOpen(A7672_Socket_t *socket, const char *server, uint16_t port);

/**
 * @brief Send data over TCP socket
 * @param socket Pointer to socket structure
 * @param data Data to send
 * @param len Length of data
 * @return A7672_OK on success
 */
A7672_Status_t A7672_TCPSend(A7672_Socket_t *socket, const uint8_t *data, uint16_t len);

/**
 * @brief Receive data from TCP socket
 * @param socket Pointer to socket structure
 * @param buffer Buffer for received data
 * @param len Maximum length to receive
 * @param received Output: Number of bytes received
 * @return A7672_OK on success
 */
A7672_Status_t A7672_TCPReceive(A7672_Socket_t *socket, uint8_t *buffer,
                                 uint16_t len, uint16_t *received);

/**
 * @brief Close TCP socket
 * @param socket Pointer to socket structure
 * @return A7672_OK on success
 */
A7672_Status_t A7672_TCPClose(A7672_Socket_t *socket);

/**
 * @brief Send SMS message
 * @param phone_number Recipient phone number (international format)
 * @param message Message text (max 160 characters)
 * @return A7672_OK on success
 */
A7672_Status_t A7672_SMSSend(const char *phone_number, const char *message);

/**
 * @brief Read SMS message
 * @param index SMS index (1-based)
 * @param sender Output: Sender phone number
 * @param message Output: Message text
 * @param max_len Maximum message length
 * @return A7672_OK on success
 */
A7672_Status_t A7672_SMSRead(uint16_t index, char *sender, char *message, uint16_t max_len);

/* ═══════════════════════════════════════════════════════════════════════════
 * TF-M Integration Functions
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Save APN configuration to TF-M ITS
 * @param apn Access Point Name
 * @param username APN username (optional, can be NULL)
 * @param password APN password (optional, can be NULL)
 * @return A7672_OK on success
 *
 * @example
 * A7672_SaveAPNConfig("internet", "", "");  // Simple APN
 * A7672_SaveAPNConfig("wap.cingular", "WAP@CINGULARGPRS.COM", "CINGULAR1");
 */
A7672_Status_t A7672_SaveAPNConfig(const char *apn, const char *username, const char *password);

/**
 * @brief Save API key to TF-M ITS
 * @param api_key Cloud service API key
 * @return A7672_OK on success
 *
 * @note API key is encrypted at rest in TF-M secure storage
 */
A7672_Status_t A7672_SaveAPIKey(const char *api_key);

/* ═══════════════════════════════════════════════════════════════════════════
 * Helper Macros
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * @brief Convert RSSI value to dBm
 */
#define A7672_RSSI_TO_DBM(rssi)  (((rssi) == 99) ? -999 : (-113 + ((rssi) * 2)))

/**
 * @brief Check if RSSI indicates good signal (>= -85 dBm)
 */
#define A7672_IS_SIGNAL_GOOD(rssi)  ((rssi) >= 14)

/**
 * @brief Check if GPS fix is valid
 */
#define A7672_GPS_IS_FIX_VALID(gps)  ((gps)->fix_valid && (gps)->satellites >= 4)

/* ═══════════════════════════════════════════════════════════════════════════
 * Usage Examples
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * Example 1: Basic GPS Tracker
 * ─────────────────────────────
 * ```c
 * // Initialize and connect
 * A7672_Init(&huart1, GPIOB, GPIO_PIN_0, GPIOB, GPIO_PIN_1);
 * A7672_PowerOn();
 * A7672_ConnectNetwork();
 * A7672_GPSInit();
 *
 * // Main loop
 * while (1) {
 *     A7672_GPSData_t gps;
 *     if (A7672_GPSReadData(&gps) == A7672_OK) {
 *         printf("📍 Location: %.6f, %.6f\n", gps.latitude, gps.longitude);
 *         printf("   Altitude: %.1f m, Speed: %.1f km/h\n",
 *                gps.altitude, gps.speed_kmh);
 *         printf("   Satellites: %d, Fix type: %d\n",
 *                gps.satellites, gps.fix_type);
 *     }
 *     HAL_Delay(5000);  // Update every 5 seconds
 * }
 * ```
 *
 * Example 2: Cloud Integration with TF-M
 * ───────────────────────────────────────
 * ```c
 * // One-time configuration (stored in TF-M ITS)
 * A7672_SaveAPNConfig("internet", "", "");
 * A7672_SaveAPIKey("your_api_key_here");
 *
 * // Send location to cloud
 * A7672_GPSData_t gps;
 * if (A7672_GPSReadData(&gps) == A7672_OK) {
 *     // Prepare JSON payload
 *     char json[256];
 *     snprintf(json, sizeof(json),
 *              "{\"device\":\"tracker01\","
 *              "\"lat\":%.6f,\"lon\":%.6f,"
 *              "\"alt\":%.1f,\"speed\":%.1f,"
 *              "\"timestamp\":%lu}",
 *              gps.latitude, gps.longitude,
 *              gps.altitude, gps.speed_kmh,
 *              (unsigned long)time(NULL));
 *
 *     // Send to cloud via HTTPS POST
 *     char response[512];
 *     if (A7672_HTTPPost("https://api.mycloud.com/location",
 *                        json, strlen(json),
 *                        response, sizeof(response)) == A7672_OK) {
 *         printf("✓ Location uploaded to cloud\n");
 *         printf("Response: %s\n", response);
 *     }
 * }
 * ```
 *
 * Example 3: Motion-Triggered GPS Update
 * ───────────────────────────────────────
 * ```c
 * // Integrate with LSM6DSO IMU for power-efficient tracking
 * LSM6DSO_ActivityState_t activity = LSM6DSO_DetectActivity();
 *
 * if (activity != LSM6DSO_ACTIVITY_STATIONARY) {
 *     // Device is moving - update GPS
 *     A7672_GPSData_t gps;
 *     if (A7672_GPSReadData(&gps) == A7672_OK) {
 *         // Send to cloud
 *         send_location_to_cloud(&gps);
 *     }
 * } else {
 *     // Device stationary - enter low power mode
 *     A7672_PowerOff();  // Save power
 * }
 * ```
 *
 * Example 4: Signal Quality Monitoring
 * ─────────────────────────────────────
 * ```c
 * int rssi, ber;
 * A7672_GetSignalQuality(&rssi, &ber);
 *
 * int dbm = A7672_RSSI_TO_DBM(rssi);
 * printf("Signal Strength: %d dBm", dbm);
 *
 * if (A7672_IS_SIGNAL_GOOD(rssi)) {
 *     printf(" (GOOD)\n");
 * } else if (rssi >= 10) {
 *     printf(" (FAIR)\n");
 * } else {
 *     printf(" (POOR)\n");
 * }
 * ```
 */

#ifdef __cplusplus
}
#endif

#endif /* SIMCOM_A7672SA_DRIVER_H */

/**
 * @file simcom_a7672sa.h
 * @brief SimCom A7672SA 4G LTE Cat-1 + GPS Module Driver
 *
 * This driver provides complete support for the SimCom A7672SA module:
 * - 4G LTE Cat-1 cellular connectivity
 * - GPS/GNSS positioning
 * - AT command interface
 * - TCP/UDP sockets
 * - TLS 1.2/1.3 support
 * - HTTP/HTTPS client
 * - MQTT client
 *
 * Hardware Interface:
 * - UART for AT commands (115200 baud default)
 * - Power control GPIO
 * - Reset GPIO
 * - Status GPIO (for DTR/RI)
 *
 * @author TF-M Training Project
 * @date 2024
 */

#ifndef SIMCOM_A7672SA_H
#define SIMCOM_A7672SA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Configuration Constants
 ******************************************************************************/

#define SIMCOM_A7672_MAX_RESPONSE_LEN   2048    /**< Maximum AT response length */
#define SIMCOM_A7672_MAX_SOCKETS        6       /**< Maximum concurrent sockets */
#define SIMCOM_A7672_DEFAULT_TIMEOUT_MS 5000    /**< Default command timeout */
#define SIMCOM_A7672_POWERUP_DELAY_MS   5000    /**< Power-up stabilization delay */

/*******************************************************************************
 * Data Types
 ******************************************************************************/

/**
 * @brief Module initialization status
 */
typedef enum {
    SIMCOM_STATUS_UNINITIALIZED = 0,
    SIMCOM_STATUS_INITIALIZING,
    SIMCOM_STATUS_READY,
    SIMCOM_STATUS_ERROR,
    SIMCOM_STATUS_SLEEP
} simcom_status_t;

/**
 * @brief Network registration status
 */
typedef enum {
    SIMCOM_NET_NOT_REGISTERED = 0,      /**< Not registered, not searching */
    SIMCOM_NET_REGISTERED_HOME,         /**< Registered on home network */
    SIMCOM_NET_SEARCHING,               /**< Searching for network */
    SIMCOM_NET_REGISTRATION_DENIED,     /**< Registration denied */
    SIMCOM_NET_UNKNOWN,                 /**< Unknown status */
    SIMCOM_NET_REGISTERED_ROAMING       /**< Registered, roaming */
} simcom_net_status_t;

/**
 * @brief GPS fix status
 */
typedef enum {
    SIMCOM_GPS_NO_FIX = 0,              /**< No GPS fix */
    SIMCOM_GPS_FIX_2D,                  /**< 2D fix (lat/lon only) */
    SIMCOM_GPS_FIX_3D                   /**< 3D fix (lat/lon/alt) */
} simcom_gps_fix_t;

/**
 * @brief Socket protocol type
 */
typedef enum {
    SIMCOM_PROTO_TCP = 0,
    SIMCOM_PROTO_UDP
} simcom_protocol_t;

/**
 * @brief Socket state
 */
typedef enum {
    SIMCOM_SOCKET_CLOSED = 0,
    SIMCOM_SOCKET_CONNECTING,
    SIMCOM_SOCKET_CONNECTED,
    SIMCOM_SOCKET_CLOSING
} simcom_socket_state_t;

/**
 * @brief TLS version
 */
typedef enum {
    SIMCOM_TLS_VERSION_1_0 = 0,
    SIMCOM_TLS_VERSION_1_1,
    SIMCOM_TLS_VERSION_1_2,
    SIMCOM_TLS_VERSION_1_3
} simcom_tls_version_t;

/**
 * @brief Error codes
 */
typedef enum {
    SIMCOM_OK = 0,                      /**< Success */
    SIMCOM_ERR_TIMEOUT = -1,            /**< Command timeout */
    SIMCOM_ERR_INVALID_PARAM = -2,      /**< Invalid parameter */
    SIMCOM_ERR_NOT_INITIALIZED = -3,    /**< Module not initialized */
    SIMCOM_ERR_AT_COMMAND = -4,         /**< AT command failed */
    SIMCOM_ERR_NO_MEMORY = -5,          /**< Out of memory */
    SIMCOM_ERR_NETWORK = -6,            /**< Network error */
    SIMCOM_ERR_GPS = -7,                /**< GPS error */
    SIMCOM_ERR_SOCKET = -8,             /**< Socket error */
    SIMCOM_ERR_TLS = -9,                /**< TLS error */
    SIMCOM_ERR_BUSY = -10               /**< Module busy */
} simcom_error_t;

/**
 * @brief GPS position data
 */
typedef struct {
    simcom_gps_fix_t fix_type;          /**< Fix type */
    double latitude;                    /**< Latitude in degrees */
    double longitude;                   /**< Longitude in degrees */
    float altitude;                     /**< Altitude in meters */
    float speed;                        /**< Speed in km/h */
    float course;                       /**< Course in degrees */
    uint8_t satellites;                 /**< Number of satellites */
    float hdop;                         /**< Horizontal dilution of precision */
    uint32_t timestamp;                 /**< UTC timestamp (seconds since epoch) */
    bool valid;                         /**< Position validity flag */
} simcom_gps_data_t;

/**
 * @brief Network information
 */
typedef struct {
    simcom_net_status_t status;         /**< Registration status */
    char operator_name[32];             /**< Network operator name */
    int16_t rssi;                       /**< Signal strength (dBm) */
    uint8_t ber;                        /**< Bit error rate */
    char imei[20];                      /**< IMEI number */
    char iccid[24];                     /**< SIM ICCID */
    char ip_address[16];                /**< Assigned IP address */
} simcom_network_info_t;

/**
 * @brief Socket configuration
 */
typedef struct {
    simcom_protocol_t protocol;         /**< TCP or UDP */
    char remote_host[128];              /**< Remote hostname/IP */
    uint16_t remote_port;               /**< Remote port */
    uint16_t local_port;                /**< Local port (0 = auto) */
    bool use_tls;                       /**< Enable TLS */
    simcom_tls_version_t tls_version;   /**< TLS version */
    uint32_t timeout_ms;                /**< Connection timeout */
} simcom_socket_config_t;

/**
 * @brief Socket handle
 */
typedef struct {
    uint8_t id;                         /**< Socket ID (0-5) */
    simcom_socket_state_t state;        /**< Current state */
    simcom_protocol_t protocol;         /**< Protocol type */
    bool use_tls;                       /**< TLS enabled */
    uint16_t remote_port;               /**< Remote port */
    char remote_host[128];              /**< Remote host */
} simcom_socket_t;

/**
 * @brief UART receive callback function type
 *
 * Called when data is received from the module.
 *
 * @param data Received data buffer
 * @param len Length of received data
 * @param user_data User context pointer
 */
typedef void (*simcom_rx_callback_t)(const uint8_t *data, size_t len, void *user_data);

/**
 * @brief Module configuration
 */
typedef struct {
    void *uart_handle;                  /**< UART peripheral handle */
    uint32_t uart_baudrate;             /**< UART baud rate */

    /* GPIO pins (platform-specific) */
    void *power_gpio;                   /**< Power control GPIO */
    uint32_t power_pin;                 /**< Power control pin */
    void *reset_gpio;                   /**< Reset GPIO */
    uint32_t reset_pin;                 /**< Reset pin */

    /* Callbacks */
    simcom_rx_callback_t rx_callback;   /**< RX callback */
    void *user_data;                    /**< User context for callback */

    /* Buffers */
    uint8_t *rx_buffer;                 /**< RX buffer (must be allocated) */
    size_t rx_buffer_size;              /**< RX buffer size */
    uint8_t *tx_buffer;                 /**< TX buffer (must be allocated) */
    size_t tx_buffer_size;              /**< TX buffer size */
} simcom_config_t;

/**
 * @brief Module handle (opaque type)
 */
typedef struct simcom_handle simcom_handle_t;

/*******************************************************************************
 * Initialization and Power Management
 ******************************************************************************/

/**
 * @brief Initialize the SimCom A7672SA module
 *
 * This function initializes the module hardware interface and powers on
 * the module. It waits for the module to respond to AT commands.
 *
 * @param config Module configuration
 * @param handle Pointer to receive module handle
 * @return SIMCOM_OK on success, error code otherwise
 *
 * @note This function blocks until the module is ready (up to 10 seconds)
 */
simcom_error_t simcom_init(const simcom_config_t *config, simcom_handle_t **handle);

/**
 * @brief Deinitialize the module
 *
 * Powers down the module and releases resources.
 *
 * @param handle Module handle
 * @return SIMCOM_OK on success, error code otherwise
 */
simcom_error_t simcom_deinit(simcom_handle_t *handle);

/**
 * @brief Power on the module
 *
 * @param handle Module handle
 * @return SIMCOM_OK on success, error code otherwise
 */
simcom_error_t simcom_power_on(simcom_handle_t *handle);

/**
 * @brief Power off the module
 *
 * @param handle Module handle
 * @return SIMCOM_OK on success, error code otherwise
 */
simcom_error_t simcom_power_off(simcom_handle_t *handle);

/**
 * @brief Reset the module
 *
 * @param handle Module handle
 * @return SIMCOM_OK on success, error code otherwise
 */
simcom_error_t simcom_reset(simcom_handle_t *handle);

/**
 * @brief Get module status
 *
 * @param handle Module handle
 * @return Current status
 */
simcom_status_t simcom_get_status(simcom_handle_t *handle);

/*******************************************************************************
 * AT Command Interface
 ******************************************************************************/

/**
 * @brief Send AT command and wait for response
 *
 * @param handle Module handle
 * @param command AT command string (without AT prefix or \r\n)
 * @param response Buffer to receive response
 * @param response_len Maximum response length
 * @param timeout_ms Command timeout in milliseconds
 * @return SIMCOM_OK on success, error code otherwise
 *
 * @note Response includes all lines until "OK" or "ERROR"
 */
simcom_error_t simcom_send_at_command(simcom_handle_t *handle,
                                      const char *command,
                                      char *response,
                                      size_t response_len,
                                      uint32_t timeout_ms);

/**
 * @brief Send raw AT command (for advanced usage)
 *
 * @param handle Module handle
 * @param command Complete AT command with terminators
 * @param len Command length
 * @return SIMCOM_OK on success, error code otherwise
 */
simcom_error_t simcom_send_raw(simcom_handle_t *handle,
                               const uint8_t *command,
                               size_t len);

/**
 * @brief Read response from module
 *
 * @param handle Module handle
 * @param response Buffer to receive response
 * @param response_len Maximum response length
 * @param timeout_ms Read timeout
 * @return Number of bytes read, or negative error code
 */
int simcom_read_response(simcom_handle_t *handle,
                         char *response,
                         size_t response_len,
                         uint32_t timeout_ms);

/*******************************************************************************
 * Network Functions
 ******************************************************************************/

/**
 * @brief Set APN for data connection
 *
 * @param handle Module handle
 * @param apn Access Point Name
 * @param username APN username (NULL if not required)
 * @param password APN password (NULL if not required)
 * @return SIMCOM_OK on success, error code otherwise
 */
simcom_error_t simcom_set_apn(simcom_handle_t *handle,
                              const char *apn,
                              const char *username,
                              const char *password);

/**
 * @brief Activate data connection
 *
 * @param handle Module handle
 * @return SIMCOM_OK on success, error code otherwise
 */
simcom_error_t simcom_activate_data(simcom_handle_t *handle);

/**
 * @brief Deactivate data connection
 *
 * @param handle Module handle
 * @return SIMCOM_OK on success, error code otherwise
 */
simcom_error_t simcom_deactivate_data(simcom_handle_t *handle);

/**
 * @brief Get network information
 *
 * @param handle Module handle
 * @param info Pointer to network info structure
 * @return SIMCOM_OK on success, error code otherwise
 */
simcom_error_t simcom_get_network_info(simcom_handle_t *handle,
                                       simcom_network_info_t *info);

/**
 * @brief Wait for network registration
 *
 * Blocks until the module registers on the network or timeout occurs.
 *
 * @param handle Module handle
 * @param timeout_ms Maximum wait time in milliseconds
 * @return SIMCOM_OK on success, error code otherwise
 */
simcom_error_t simcom_wait_for_network(simcom_handle_t *handle,
                                       uint32_t timeout_ms);

/*******************************************************************************
 * GPS Functions
 ******************************************************************************/

/**
 * @brief Start GPS subsystem
 *
 * @param handle Module handle
 * @return SIMCOM_OK on success, error code otherwise
 */
simcom_error_t simcom_gps_start(simcom_handle_t *handle);

/**
 * @brief Stop GPS subsystem
 *
 * @param handle Module handle
 * @return SIMCOM_OK on success, error code otherwise
 */
simcom_error_t simcom_gps_stop(simcom_handle_t *handle);

/**
 * @brief Get current GPS position
 *
 * @param handle Module handle
 * @param gps_data Pointer to GPS data structure
 * @return SIMCOM_OK on success, error code otherwise
 */
simcom_error_t simcom_gps_get_position(simcom_handle_t *handle,
                                       simcom_gps_data_t *gps_data);

/**
 * @brief Check if GPS has valid fix
 *
 * @param handle Module handle
 * @return true if GPS has valid fix, false otherwise
 */
bool simcom_gps_has_fix(simcom_handle_t *handle);

/*******************************************************************************
 * Socket Functions
 ******************************************************************************/

/**
 * @brief Open a socket
 *
 * @param handle Module handle
 * @param config Socket configuration
 * @param socket Pointer to receive socket handle
 * @return SIMCOM_OK on success, error code otherwise
 */
simcom_error_t simcom_socket_open(simcom_handle_t *handle,
                                  const simcom_socket_config_t *config,
                                  simcom_socket_t *socket);

/**
 * @brief Close a socket
 *
 * @param handle Module handle
 * @param socket Socket handle
 * @return SIMCOM_OK on success, error code otherwise
 */
simcom_error_t simcom_socket_close(simcom_handle_t *handle,
                                   simcom_socket_t *socket);

/**
 * @brief Send data through socket
 *
 * @param handle Module handle
 * @param socket Socket handle
 * @param data Data to send
 * @param len Data length
 * @param sent Pointer to receive number of bytes sent
 * @return SIMCOM_OK on success, error code otherwise
 */
simcom_error_t simcom_socket_send(simcom_handle_t *handle,
                                  simcom_socket_t *socket,
                                  const uint8_t *data,
                                  size_t len,
                                  size_t *sent);

/**
 * @brief Receive data from socket
 *
 * @param handle Module handle
 * @param socket Socket handle
 * @param buffer Buffer to receive data
 * @param buffer_len Buffer size
 * @param received Pointer to receive number of bytes received
 * @param timeout_ms Receive timeout
 * @return SIMCOM_OK on success, error code otherwise
 */
simcom_error_t simcom_socket_receive(simcom_handle_t *handle,
                                     simcom_socket_t *socket,
                                     uint8_t *buffer,
                                     size_t buffer_len,
                                     size_t *received,
                                     uint32_t timeout_ms);

/*******************************************************************************
 * HTTP/HTTPS Functions
 ******************************************************************************/

/**
 * @brief HTTP request method
 */
typedef enum {
    SIMCOM_HTTP_GET = 0,
    SIMCOM_HTTP_POST,
    SIMCOM_HTTP_PUT,
    SIMCOM_HTTP_DELETE,
    SIMCOM_HTTP_HEAD
} simcom_http_method_t;

/**
 * @brief Perform HTTP/HTTPS request
 *
 * @param handle Module handle
 * @param method HTTP method
 * @param url Complete URL (http:// or https://)
 * @param headers Optional headers (NULL-terminated array)
 * @param body Request body (for POST/PUT)
 * @param body_len Body length
 * @param response Buffer to receive response
 * @param response_len Response buffer size
 * @param http_code Pointer to receive HTTP status code
 * @return SIMCOM_OK on success, error code otherwise
 */
simcom_error_t simcom_http_request(simcom_handle_t *handle,
                                   simcom_http_method_t method,
                                   const char *url,
                                   const char **headers,
                                   const uint8_t *body,
                                   size_t body_len,
                                   uint8_t *response,
                                   size_t response_len,
                                   int *http_code);

/*******************************************************************************
 * Utility Functions
 ******************************************************************************/

/**
 * @brief Get module firmware version
 *
 * @param handle Module handle
 * @param version Buffer to receive version string
 * @param len Buffer size
 * @return SIMCOM_OK on success, error code otherwise
 */
simcom_error_t simcom_get_version(simcom_handle_t *handle,
                                  char *version,
                                  size_t len);

/**
 * @brief Get module IMEI
 *
 * @param handle Module handle
 * @param imei Buffer to receive IMEI (minimum 16 bytes)
 * @return SIMCOM_OK on success, error code otherwise
 */
simcom_error_t simcom_get_imei(simcom_handle_t *handle, char *imei);

/**
 * @brief Get SIM ICCID
 *
 * @param handle Module handle
 * @param iccid Buffer to receive ICCID (minimum 21 bytes)
 * @return SIMCOM_OK on success, error code otherwise
 */
simcom_error_t simcom_get_iccid(simcom_handle_t *handle, char *iccid);

/**
 * @brief Get signal strength
 *
 * @param handle Module handle
 * @param rssi Pointer to receive RSSI in dBm
 * @param ber Pointer to receive Bit Error Rate
 * @return SIMCOM_OK on success, error code otherwise
 */
simcom_error_t simcom_get_signal_strength(simcom_handle_t *handle,
                                          int16_t *rssi,
                                          uint8_t *ber);

/**
 * @brief Set module to low power mode
 *
 * @param handle Module handle
 * @param enable true to enable sleep mode, false to disable
 * @return SIMCOM_OK on success, error code otherwise
 */
simcom_error_t simcom_set_sleep_mode(simcom_handle_t *handle, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* SIMCOM_A7672SA_H */

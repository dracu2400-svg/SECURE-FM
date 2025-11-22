# TF-M Cloud Server

Complete cloud backend for testing and managing TF-M-enabled IoT devices.

## Features

- ✅ **Device Registration**: Secure device onboarding with public key registration
- ✅ **Attestation Verification**: PSA Initial Attestation token verification
- ✅ **OTA Updates**: Firmware version management and distribution
- ✅ **Telemetry**: Device data collection and storage
- ✅ **Certificate Management**: Trusted key management for attestation

## Quick Start

### 1. Installation

```bash
# Install dependencies
pip install -r requirements.txt

# Create certificate directory and add trusted keys
mkdir -p certificates
# Copy device public keys (.pem files) to certificates/
```

### 2. Run Server

```bash
python tfm_cloud_server.py
```

Server will start on `http://localhost:5000`

### 3. Test with Example Client

```bash
# In another terminal
cd examples
python device_client_example.py
```

## API Documentation

### Device Registration

**POST** `/api/v1/device/register`

Register a new device with the cloud server.

```json
{
    "device_id": "unique-device-id",
    "device_name": "My Tracker",
    "manufacturer": "Acme Corp",
    "model": "Tracker-v1",
    "hw_version": "1.0",
    "public_key": "-----BEGIN PUBLIC KEY-----\n...\n-----END PUBLIC KEY-----"
}
```

Response:
```json
{
    "status": "registered",
    "device_id": "unique-device-id",
    "message": "Device registered successfully"
}
```

### Attestation

**GET** `/api/v1/device/<device_id>/challenge`

Get attestation challenge nonce.

Response:
```json
{
    "challenge": "base64-encoded-32-byte-nonce",
    "device_id": "unique-device-id",
    "expires_at": "2024-01-15T10:30:00"
}
```

**POST** `/api/v1/device/attest`

Verify device attestation token.

```json
{
    "device_id": "unique-device-id",
    "token": "base64-encoded-attestation-token",
    "challenge": "base64-encoded-challenge"
}
```

Response:
```json
{
    "verified": true,
    "claims": {
        "instance_id": "0123456789abcdef...",
        "hw_version": "1.0.0",
        "sw_components": [...]
    },
    "verified_with": "device_key.pem"
}
```

### Firmware Management

**POST** `/api/v1/firmware/upload`

Upload new firmware version (multipart/form-data).

Form fields:
- `file`: Firmware binary
- `version`: Version string (e.g., "1.2.3")
- `security_counter`: Security counter value
- `description`: Optional description

**GET** `/api/v1/firmware/latest`

Get latest firmware information.

**GET** `/api/v1/firmware/download/<firmware_id>`

Download firmware binary.

### Telemetry

**POST** `/api/v1/device/<device_id>/telemetry`

Submit telemetry data.

```json
{
    "type": "gps",
    "data": {
        "latitude": 37.7749,
        "longitude": -122.4194,
        "altitude": 100.5,
        "timestamp": "2024-01-15T10:30:00Z"
    }
}
```

**GET** `/api/v1/device/<device_id>/telemetry?limit=100&type=gps`

Retrieve telemetry data.

### Device Management

**GET** `/api/v1/devices`

List all registered devices.

## Device Integration

### C/C++ Example (TF-M Device)

```c
#include "psa/initial_attestation.h"
#include "psa/crypto.h"
#include <http_client.h>

/* Server configuration */
#define CLOUD_SERVER_URL "https://your-server.com/api/v1"
#define DEVICE_ID "my-tracker-001"

/**
 * Perform device attestation with cloud server
 */
int perform_attestation(void)
{
    psa_status_t status;
    uint8_t challenge[32];
    uint8_t token[2048];
    size_t token_len;

    /* 1. Get challenge from server */
    char url[256];
    snprintf(url, sizeof(url), "%s/device/%s/challenge",
             CLOUD_SERVER_URL, DEVICE_ID);

    http_response_t response;
    if (http_get(url, &response) != 0) {
        printf("Failed to get challenge\n");
        return -1;
    }

    /* Parse JSON and extract challenge */
    parse_challenge_from_json(response.body, challenge, sizeof(challenge));

    /* 2. Generate attestation token */
    status = psa_initial_attest_get_token(
        challenge, sizeof(challenge),
        token, sizeof(token),
        &token_len
    );

    if (status != PSA_SUCCESS) {
        printf("Failed to get attestation token: %d\n", status);
        return -2;
    }

    printf("Generated attestation token: %zu bytes\n", token_len);

    /* 3. Send token to server for verification */
    char json_body[4096];
    snprintf(json_body, sizeof(json_body),
             "{"
             "\"device_id\":\"%s\","
             "\"token\":\"%s\","
             "\"challenge\":\"%s\""
             "}",
             DEVICE_ID,
             base64_encode(token, token_len),
             base64_encode(challenge, sizeof(challenge)));

    snprintf(url, sizeof(url), "%s/device/attest", CLOUD_SERVER_URL);

    if (http_post(url, json_body, &response) != 0) {
        printf("Failed to submit attestation\n");
        return -3;
    }

    /* 4. Check verification result */
    bool verified = parse_verified_from_json(response.body);

    if (verified) {
        printf("✓ Attestation verified!\n");
        return 0;
    } else {
        printf("✗ Attestation failed!\n");
        return -4;
    }
}

/**
 * Submit GPS telemetry
 */
int submit_gps_telemetry(double lat, double lon, double alt)
{
    char url[256];
    char json_body[512];

    snprintf(url, sizeof(url), "%s/device/%s/telemetry",
             CLOUD_SERVER_URL, DEVICE_ID);

    snprintf(json_body, sizeof(json_body),
             "{"
             "\"type\":\"gps\","
             "\"data\":{"
             "\"latitude\":%.6f,"
             "\"longitude\":%.6f,"
             "\"altitude\":%.2f,"
             "\"timestamp\":\"%s\""
             "}"
             "}",
             lat, lon, alt, get_iso8601_timestamp());

    http_response_t response;
    if (http_post(url, json_body, &response) != 0) {
        printf("Failed to submit telemetry\n");
        return -1;
    }

    printf("✓ Telemetry submitted\n");
    return 0;
}

/**
 * Check for firmware updates
 */
int check_firmware_update(void)
{
    char url[256];
    snprintf(url, sizeof(url), "%s/firmware/latest", CLOUD_SERVER_URL);

    http_response_t response;
    if (http_get(url, &response) != 0) {
        printf("Failed to check for updates\n");
        return -1;
    }

    /* Parse JSON response */
    char version[32];
    uint32_t security_counter;
    char sha256[65];

    parse_firmware_info_from_json(response.body, version, &security_counter, sha256);

    printf("Latest firmware: %s (counter: %u)\n", version, security_counter);

    /* Compare with current version */
    if (is_newer_version(version, CURRENT_VERSION) &&
        security_counter >= get_current_security_counter()) {

        printf("New firmware available, downloading...\n");

        /* Download firmware */
        snprintf(url, sizeof(url), "%s/firmware/download/%s",
                 CLOUD_SERVER_URL, version);

        /* Download and install via FWU service */
        return download_and_install_firmware(url);
    }

    printf("Firmware is up to date\n");
    return 0;
}
```

### Python Example

See `examples/device_client_example.py` for complete Python client example.

## Security Considerations

### Production Deployment

⚠️ **This server is for testing purposes. For production:**

1. **Use HTTPS**: Enable TLS/SSL
   ```python
   # Use gunicorn with SSL
   gunicorn --certfile=cert.pem --keyfile=key.pem tfm_cloud_server:app
   ```

2. **Authentication**: Add API key or OAuth2 authentication
   ```python
   from flask_httpauth import HTTPTokenAuth
   auth = HTTPTokenAuth(scheme='Bearer')
   ```

3. **Rate Limiting**: Prevent abuse
   ```python
   from flask_limiter import Limiter
   limiter = Limiter(app, key_func=get_remote_address)
   ```

4. **Secure Database**: Use PostgreSQL instead of SQLite
   ```bash
   pip install psycopg2-binary
   ```

5. **Key Management**: Use HSM or key management service
   - AWS KMS
   - Azure Key Vault
   - HashiCorp Vault

6. **Logging & Monitoring**: Add comprehensive logging
   ```python
   import logging
   logging.basicConfig(level=logging.INFO)
   ```

### Attestation Security

The attestation verifier checks:
- ✅ Challenge nonce matches (replay protection)
- ✅ Signature verification with trusted keys
- ✅ COSE structure validation
- ✅ Claim parsing and validation

Trusted keys must be added to `certificates/` directory before starting server.

## Database Schema

### Devices Table
```sql
CREATE TABLE devices (
    device_id TEXT PRIMARY KEY,
    device_name TEXT,
    manufacturer TEXT,
    model TEXT,
    hw_version TEXT,
    sw_version TEXT,
    registered_at TIMESTAMP,
    last_seen TIMESTAMP,
    public_key TEXT,
    certificate TEXT,
    attestation_verified BOOLEAN,
    status TEXT
);
```

### Attestation Tokens Table
```sql
CREATE TABLE attestation_tokens (
    id INTEGER PRIMARY KEY,
    device_id TEXT,
    token_data TEXT,
    verified BOOLEAN,
    timestamp TIMESTAMP
);
```

### Firmware Table
```sql
CREATE TABLE firmware (
    id INTEGER PRIMARY KEY,
    version TEXT UNIQUE,
    filename TEXT,
    size INTEGER,
    sha256 TEXT,
    security_counter INTEGER,
    uploaded_at TIMESTAMP,
    description TEXT
);
```

## Troubleshooting

### Attestation Verification Fails

1. **Check trusted keys**: Ensure device public key is in `certificates/`
   ```bash
   ls -la certificates/
   ```

2. **Verify key format**: Keys must be PEM-encoded
   ```bash
   openssl pkey -in certificates/device.pem -text -noout
   ```

3. **Check challenge**: Challenge must match exactly
   ```python
   # Enable debug logging
   import logging
   logging.basicConfig(level=logging.DEBUG)
   ```

4. **Verify COSE structure**: Token must be valid CBOR COSE_Sign1
   ```bash
   # Decode token to inspect
   python -c "import cbor2, base64, sys; print(cbor2.loads(base64.b64decode(sys.argv[1])))" <token>
   ```

### Database Locked Error

SQLite has locking limitations. For production, use PostgreSQL:

```python
# Update CONFIG
CONFIG = {
    'database_url': 'postgresql://user:pass@localhost/tfm_cloud'
}
```

## Examples

See `examples/` directory for:
- `device_client_example.py` - Python client simulation
- `register_device.sh` - Shell script for device registration
- `upload_firmware.sh` - Shell script for firmware upload

## License

BSD-3-Clause (same as TF-M)

## Support

For questions or issues:
- TF-M Mailing List: https://lists.trustedfirmware.org/mailman/listinfo/tf-m
- TF-M GitHub: https://github.com/TrustedFirmwareM/trusted-firmware-m

# OTA Firmware Update Server

**Purpose:** Secure Over-The-Air firmware update server for Lab 29

**Platform:** Python 3.8+ (runs on PC or Raspberry Pi)

**Protocol:** HTTPS with mutual TLS authentication

---

## Overview

This server provides secure firmware updates for TF-M devices demonstrating:
- Firmware signing and verification
- Encrypted firmware download
- Rollback protection
- Delta updates (optional)
- Device attestation before update

---

## Quick Start

### Install Dependencies

```bash
cd tools/server_mcu/ota_server/
pip3 install -r requirements.txt
```

### Generate Certificates

```bash
./scripts/generate_certs.sh
```

### Start Server

```bash
python3 ota_server.py \
    --firmware firmware_v2.0.0.bin \
    --cert certs/server.crt \
    --key certs/server.key \
    --port 8443
```

**Expected Output:**
```
═══════════════════════════════════════════════════════════
  🔐 OTA Firmware Update Server v1.0
═══════════════════════════════════════════════════════════
Firmware: firmware_v2.0.0.bin (245 KB)
Version: 2.0.0
Signature: VALID
Protocol: HTTPS (mutual TLS)
Port: 8443
─────────────────────────────────────────────────────────

[SERVER] Loading certificates...
[SERVER] ✓ Server certificate loaded
[SERVER] ✓ Client CA certificate loaded
[SERVER] Starting HTTPS server...
[SERVER] ✓ Server ready at https://0.0.0.0:8443

[SERVER] Waiting for device connections...
```

---

## Device Update Flow

### Step 1: Device Connects

Device (NUCLEO board) initiates HTTPS connection:

```c
// On device (Lab 29)
https_connect("https://192.168.1.100:8443", client_cert, client_key);
```

Server sees:
```
[CONNECT] Device connected: 192.168.1.50:43821
[TLS] Mutual TLS handshake...
[TLS] ✓ Client certificate verified
[TLS] Device ID: STM32U545-001
```

### Step 2: Device Requests Update

```
GET /firmware/check HTTP/1.1
X-Device-ID: STM32U545-001
X-Current-Version: 1.0.0
X-Hardware-ID: NUCLEO-U545RE-Q
```

Server responds:
```json
{
  "update_available": true,
  "new_version": "2.0.0",
  "file_size": 251904,
  "signature": "3F8D2C7E...",
  "download_url": "/firmware/download"
}
```

### Step 3: Device Attests

Device sends attestation token:
```
POST /attest HTTP/1.1
Content-Type: application/cbor

[CBOR-encoded attestation token]
```

Server verifies:
```
[ATTEST] Verifying attestation token...
[ATTEST] Device ID: STM32U545-001
[ATTEST] Firmware version: 1.0.0
[ATTEST] Boot state: SECURE
[ATTEST] ✓ Token signature valid
[ATTEST] ✓ Device authenticated
[ATTEST] Allowing firmware download
```

### Step 4: Firmware Download

```
GET /firmware/download HTTP/1.1
X-Device-ID: STM32U545-001
Range: bytes=0-4095  # Download in chunks
```

Server response:
```
HTTP/1.1 206 Partial Content
Content-Range: bytes 0-4095/251904
Content-Type: application/octet-stream

[4096 bytes of encrypted firmware]
```

Device downloads in 4KB chunks showing progress:
```
[DOWNLOAD] Progress: 4096 / 251904 (1%)
[DOWNLOAD] Progress: 8192 / 251904 (3%)
...
[DOWNLOAD] Progress: 251904 / 251904 (100%)
[DOWNLOAD] ✓ Download complete
```

### Step 5: Verification

Device verifies firmware signature:
```
[VERIFY] Checking firmware signature...
[VERIFY] Algorithm: ECDSA-P256-SHA256
[VERIFY] Public key: [from device cert]
[VERIFY] ✓ Signature valid
[VERIFY] Firmware is authentic
```

### Step 6: Installation

```
[INSTALL] Writing to slot 1...
[INSTALL] Progress: 50%
[INSTALL] Progress: 100%
[INSTALL] ✓ Firmware written
[INSTALL] Setting boot flag...
[INSTALL] ✓ Ready to reboot
```

### Step 7: Reboot & Confirm

```
[BOOT] MCUboot starting...
[BOOT] Candidate firmware in slot 1
[BOOT] Verifying signature...
[BOOT] ✓ Signature OK
[BOOT] Swapping images...
[BOOT] Starting firmware v2.0.0...

[APP] ✓ Firmware updated successfully!
[APP] Version: 2.0.0
[APP] Confirming update to server...
```

Server receives confirmation:
```
POST /firmware/confirm HTTP/1.1
X-Device-ID: STM32U545-001
X-New-Version: 2.0.0

[UPDATE] Device STM32U545-001 confirmed update to v2.0.0
[UPDATE] ✓ Update successful
```

---

## Server Configuration

### config.yaml

```yaml
server:
  host: 0.0.0.0
  port: 8443
  max_devices: 100
  chunk_size: 4096

security:
  tls_version: "TLS1.3"
  require_client_cert: true
  verify_attestation: true

firmware:
  storage_path: "./firmware/"
  signing_key: "./keys/signing_key.pem"
  max_file_size: 2097152  # 2 MB

logging:
  level: INFO
  file: "./logs/ota_server.log"
  max_size: 10485760  # 10 MB
```

---

## API Endpoints

### GET /firmware/check

**Purpose:** Check if update available

**Request:**
```
Headers:
  X-Device-ID: <device-id>
  X-Current-Version: <version>
  X-Hardware-ID: <hardware-id>
```

**Response:**
```json
{
  "update_available": true|false,
  "new_version": "2.0.0",
  "file_size": 251904,
  "sha256": "7F3D8C2E...",
  "signature": "3F8D2C7E...",
  "release_notes": "Bug fixes and security updates"
}
```

### POST /attest

**Purpose:** Verify device identity

**Request:**
```
Content-Type: application/cbor
Body: [CBOR attestation token]
```

**Response:**
```json
{
  "status": "verified",
  "device_id": "STM32U545-001",
  "download_token": "abc123..."
}
```

### GET /firmware/download

**Purpose:** Download firmware

**Request:**
```
Headers:
  X-Device-ID: <device-id>
  X-Download-Token: <token-from-attest>
  Range: bytes=<start>-<end>
```

**Response:**
```
HTTP/1.1 206 Partial Content
Content-Range: bytes <start>-<end>/<total>
Content-Type: application/octet-stream

[Firmware chunk]
```

### POST /firmware/confirm

**Purpose:** Confirm successful update

**Request:**
```json
{
  "device_id": "STM32U545-001",
  "new_version": "2.0.0",
  "status": "success"
}
```

**Response:**
```json
{
  "status": "confirmed"
}
```

---

## Firmware Signing

### Generate Signing Key

```bash
# ECDSA P-256 (recommended)
openssl ecparam -name prime256v1 -genkey -out signing_key.pem

# Extract public key
openssl ec -in signing_key.pem -pubout -out signing_key_pub.pem
```

### Sign Firmware

```bash
python3 scripts/sign_firmware.py \
    --firmware firmware_v2.0.0.bin \
    --key signing_key.pem \
    --output firmware_v2.0.0.signed.bin
```

**Output:**
```
[SIGN] Reading firmware: firmware_v2.0.0.bin (245 KB)
[SIGN] Computing SHA-256 hash...
[SIGN] Hash: 7F3D8C2E5B1A9F4D8C3E6A7F2D5C9B1E...
[SIGN] Signing with ECDSA-P256...
[SIGN] Signature: 3F8D2C7E1A5F9B4D8C3E7A1F6D9C2E5B...
[SIGN] ✓ Firmware signed
[SIGN] Output: firmware_v2.0.0.signed.bin (245 KB + 64 bytes signature)
```

---

## Security Features

### 1. Mutual TLS Authentication
- Server verifies client certificate
- Client verifies server certificate
- Prevents man-in-the-middle attacks

### 2. Device Attestation
- Verifies device identity before allowing download
- Ensures only authorized devices get updates
- Uses PSA Initial Attestation

### 3. Firmware Signature Verification
- Device verifies firmware signature
- Uses ECDSA P-256
- Prevents installation of malicious firmware

### 4. Rollback Protection
- MCUboot checks security counter
- Prevents downgrade to old vulnerable versions
- Monotonic counter in secure storage

### 5. Encrypted Transport
- All communication over TLS 1.3
- Perfect forward secrecy
- Strong cipher suites only

### 6. Rate Limiting
- Prevents DoS attacks
- Max 10 requests per minute per device
- Configurable thresholds

---

## Monitoring & Logging

### Log Format

```
2025-11-22 14:32:15 [INFO] Device connected: STM32U545-001 from 192.168.1.50
2025-11-22 14:32:16 [INFO] Attestation verified for STM32U545-001
2025-11-22 14:32:17 [INFO] Firmware download started: v2.0.0
2025-11-22 14:32:45 [INFO] Download complete: STM32U545-001 (245 KB in 28s)
2025-11-22 14:33:10 [INFO] Update confirmed: STM32U545-001 → v2.0.0
```

### Metrics Dashboard

```bash
# Start metrics server
python3 scripts/metrics.py --port 9090
```

Access at http://localhost:9090:
- Total updates: 42
- Successful: 40 (95%)
- Failed: 2 (5%)
- Average download time: 25s
- Devices by version: v1.0.0 (10), v2.0.0 (32)

---

## Troubleshooting

### Issue: Certificate verification fails

**Error:**
```
SSL: CERTIFICATE_VERIFY_FAILED
```

**Solution:**
1. Check CA certificate is correct
2. Verify certificate not expired
3. Ensure hostname matches certificate CN

```bash
# Check certificate
openssl x509 -in server.crt -text -noout
```

### Issue: Device can't connect

**Check:**
1. Network connectivity (`ping server-ip`)
2. Port not blocked by firewall
3. Server is running

```bash
# Test connectivity
curl -k https://server-ip:8443/health
```

### Issue: Signature verification fails

**Causes:**
- Wrong signing key
- Firmware corrupted during download
- Signature algorithm mismatch

**Debug:**
```bash
# Verify signature manually
python3 scripts/verify_signature.py \
    --firmware firmware.bin \
    --signature firmware.sig \
    --pubkey signing_key_pub.pem
```

---

## Production Deployment

### Checklist

- [ ] Use production TLS certificates (not self-signed)
- [ ] Enable rate limiting
- [ ] Set up CDN for firmware distribution
- [ ] Implement rollout strategy (canary, staged)
- [ ] Set up monitoring and alerts
- [ ] Configure automatic backups
- [ ] Document rollback procedure
- [ ] Test disaster recovery
- [ ] Enable audit logging
- [ ] Set up redundant servers

### Scaling

For >1000 devices:
- Use load balancer (nginx, HAProxy)
- CDN for firmware files (CloudFront, Cloudflare)
- Database for device tracking (PostgreSQL)
- Message queue for notifications (RabbitMQ)

---

## Integration with Lab 29

See: `section_05_optimization/labs/solutions/lab_29/README.md`

**Device code connects to this server for OTA updates**

---

**Version:** 1.0
**Python:** 3.8+
**Dependencies:** flask, cryptography, cbor2, pycose
**License:** MIT

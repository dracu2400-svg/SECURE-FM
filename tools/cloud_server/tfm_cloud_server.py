#!/usr/bin/env python3
"""
TF-M Cloud Server for Testing and Verification
Supports: Attestation, Device Management, OTA Updates, Telemetry

This server provides a complete cloud backend for testing TF-M devices:
- Device attestation verification
- Secure device registration
- OTA firmware update distribution
- Telemetry data collection
- Certificate management
"""

import os
import sys
import json
import hashlib
import hmac
import base64
import binascii
from datetime import datetime, timedelta
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# Flask for HTTP server
from flask import Flask, request, jsonify, send_file
from flask_cors import CORS

# Cryptography
from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec, rsa
from cryptography.hazmat.backends import default_backend
from cryptography.x509.oid import NameOID, ExtensionOID
import cbor2

# Database
import sqlite3

# MQTT (optional)
try:
    import paho.mqtt.client as mqtt
    MQTT_AVAILABLE = True
except ImportError:
    MQTT_AVAILABLE = False
    print("Warning: paho-mqtt not available. MQTT features disabled.")

app = Flask(__name__)
CORS(app)  # Enable CORS for testing

# Configuration
CONFIG = {
    'server_port': 5000,
    'database': 'tfm_cloud.db',
    'firmware_dir': 'firmware',
    'cert_dir': 'certificates',
    'attestation_verify': True,
    'mqtt_enabled': MQTT_AVAILABLE,
    'mqtt_broker': 'localhost',
    'mqtt_port': 1883,
}

# ============================================================================
# Database Setup
# ============================================================================

def init_database():
    """Initialize SQLite database"""
    conn = sqlite3.connect(CONFIG['database'])
    c = conn.cursor()

    # Devices table
    c.execute('''
        CREATE TABLE IF NOT EXISTS devices (
            device_id TEXT PRIMARY KEY,
            device_name TEXT,
            manufacturer TEXT,
            model TEXT,
            hw_version TEXT,
            sw_version TEXT,
            registered_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            last_seen TIMESTAMP,
            public_key TEXT,
            certificate TEXT,
            attestation_verified BOOLEAN DEFAULT 0,
            status TEXT DEFAULT 'pending'
        )
    ''')

    # Attestation tokens table
    c.execute('''
        CREATE TABLE IF NOT EXISTS attestation_tokens (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            device_id TEXT,
            token_data TEXT,
            verified BOOLEAN,
            timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (device_id) REFERENCES devices(device_id)
        )
    ''')

    # Firmware versions table
    c.execute('''
        CREATE TABLE IF NOT EXISTS firmware (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            version TEXT UNIQUE,
            filename TEXT,
            size INTEGER,
            sha256 TEXT,
            security_counter INTEGER,
            uploaded_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            description TEXT
        )
    ''')

    # OTA updates table
    c.execute('''
        CREATE TABLE IF NOT EXISTS ota_updates (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            device_id TEXT,
            firmware_id INTEGER,
            status TEXT DEFAULT 'pending',
            initiated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            completed_at TIMESTAMP,
            FOREIGN KEY (device_id) REFERENCES devices(device_id),
            FOREIGN KEY (firmware_id) REFERENCES firmware(id)
        )
    ''')

    # Telemetry table
    c.execute('''
        CREATE TABLE IF NOT EXISTS telemetry (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            device_id TEXT,
            timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            data_type TEXT,
            data TEXT,
            FOREIGN KEY (device_id) REFERENCES devices(device_id)
        )
    ''')

    conn.commit()
    conn.close()
    print("✓ Database initialized")

# ============================================================================
# Attestation Verification
# ============================================================================

class AttestationVerifier:
    """
    Verifies PSA Initial Attestation Tokens
    Implements PSA Attestation API verification
    """

    # PSA Attestation claims (EAT - Entity Attestation Token)
    PSA_CLAIM_NONCE = -75000              # Challenge nonce
    PSA_CLAIM_INSTANCE_ID = -75001        # UEID (Unique Entity ID)
    PSA_CLAIM_BOOT_SEED = -75002          # Boot seed
    PSA_CLAIM_HW_VERSION = -75003         # Hardware version
    PSA_CLAIM_IMPLEMENTATION_ID = -75004  # Implementation ID
    PSA_CLAIM_SW_COMPONENTS = -75005      # Software components
    PSA_CLAIM_PROFILE_DEFINITION = -75006 # Profile
    PSA_CLAIM_SECURITY_LIFECYCLE = -75007 # Lifecycle state

    def __init__(self):
        """Initialize verifier"""
        self.trusted_keys = self._load_trusted_keys()

    def _load_trusted_keys(self) -> List:
        """Load trusted public keys for verification"""
        keys = []
        cert_dir = Path(CONFIG['cert_dir'])

        if not cert_dir.exists():
            cert_dir.mkdir(parents=True)
            print("⚠ No trusted keys found. Add device public keys to certificates/")
            return keys

        for key_file in cert_dir.glob('*.pem'):
            try:
                with open(key_file, 'rb') as f:
                    key_data = f.read()
                    # Try to load as public key
                    try:
                        key = serialization.load_pem_public_key(key_data, backend=default_backend())
                        keys.append({
                            'file': key_file.name,
                            'key': key
                        })
                        print(f"✓ Loaded trusted key: {key_file.name}")
                    except:
                        # Try as certificate
                        cert = x509.load_pem_x509_certificate(key_data, backend=default_backend())
                        keys.append({
                            'file': key_file.name,
                            'key': cert.public_key()
                        })
                        print(f"✓ Loaded trusted certificate: {key_file.name}")
            except Exception as e:
                print(f"✗ Failed to load {key_file}: {e}")

        return keys

    def verify_token(self, token_bytes: bytes, challenge: bytes) -> Dict:
        """
        Verify PSA attestation token

        Args:
            token_bytes: CBOR-encoded COSE_Sign1 token
            challenge: Challenge nonce sent to device

        Returns:
            Dictionary with verification results and claims
        """
        result = {
            'verified': False,
            'claims': {},
            'errors': []
        }

        try:
            # Decode CBOR COSE_Sign1 structure
            # COSE_Sign1 = [protected, unprotected, payload, signature]
            cose_msg = cbor2.loads(token_bytes)

            if not isinstance(cose_msg, list) or len(cose_msg) != 4:
                result['errors'].append("Invalid COSE_Sign1 structure")
                return result

            protected = cose_msg[0]
            unprotected = cose_msg[1]
            payload = cose_msg[2]
            signature = cose_msg[3]

            # Decode protected headers
            if protected:
                protected_headers = cbor2.loads(protected)
            else:
                protected_headers = {}

            # Get algorithm
            alg = protected_headers.get(1)  # Algorithm parameter
            if alg is None:
                result['errors'].append("No algorithm specified")
                return result

            print(f"Token algorithm: {alg}")

            # Decode payload (EAT claims)
            claims = cbor2.loads(payload)
            result['claims'] = self._decode_claims(claims)

            # Verify challenge nonce
            token_nonce = claims.get(self.PSA_CLAIM_NONCE)
            if token_nonce != challenge:
                result['errors'].append("Challenge nonce mismatch")
                print(f"Expected nonce: {challenge.hex()}")
                print(f"Token nonce: {token_nonce.hex() if token_nonce else 'None'}")
                return result

            # Create signature structure for verification
            # Sig_structure = [
            #     context = "Signature1",
            #     protected,
            #     external_aad = b'',
            #     payload
            # ]
            sig_structure = [
                "Signature1",
                protected,
                b'',  # external_aad
                payload
            ]
            sig_structure_bytes = cbor2.dumps(sig_structure)

            # Try each trusted key
            verified = False
            for trusted_key_info in self.trusted_keys:
                try:
                    key = trusted_key_info['key']

                    # Verify signature based on algorithm
                    if alg == -7:  # ES256 (ECDSA with SHA-256)
                        key.verify(signature, sig_structure_bytes,
                                   ec.ECDSA(hashes.SHA256()))
                        verified = True
                        result['verified_with'] = trusted_key_info['file']
                        break
                    elif alg == -35:  # ES384 (ECDSA with SHA-384)
                        key.verify(signature, sig_structure_bytes,
                                   ec.ECDSA(hashes.SHA384()))
                        verified = True
                        result['verified_with'] = trusted_key_info['file']
                        break
                    elif alg == -257:  # RS256 (RSA with SHA-256)
                        from cryptography.hazmat.primitives.asymmetric import padding
                        key.verify(signature, sig_structure_bytes,
                                   padding.PKCS1v15(), hashes.SHA256())
                        verified = True
                        result['verified_with'] = trusted_key_info['file']
                        break

                except Exception as e:
                    continue  # Try next key

            if not verified:
                result['errors'].append("Signature verification failed with all trusted keys")
                return result

            result['verified'] = True
            print("✓ Attestation token verified successfully")

        except Exception as e:
            result['errors'].append(f"Verification error: {str(e)}")
            import traceback
            traceback.print_exc()

        return result

    def _decode_claims(self, claims: Dict) -> Dict:
        """Decode PSA attestation claims to human-readable format"""
        decoded = {}

        # Instance ID (UEID)
        if self.PSA_CLAIM_INSTANCE_ID in claims:
            instance_id = claims[self.PSA_CLAIM_INSTANCE_ID]
            decoded['instance_id'] = instance_id.hex() if isinstance(instance_id, bytes) else str(instance_id)

        # Boot seed
        if self.PSA_CLAIM_BOOT_SEED in claims:
            boot_seed = claims[self.PSA_CLAIM_BOOT_SEED]
            decoded['boot_seed'] = boot_seed.hex() if isinstance(boot_seed, bytes) else str(boot_seed)

        # Hardware version
        if self.PSA_CLAIM_HW_VERSION in claims:
            decoded['hw_version'] = claims[self.PSA_CLAIM_HW_VERSION]

        # Implementation ID
        if self.PSA_CLAIM_IMPLEMENTATION_ID in claims:
            impl_id = claims[self.PSA_CLAIM_IMPLEMENTATION_ID]
            decoded['implementation_id'] = impl_id.hex() if isinstance(impl_id, bytes) else str(impl_id)

        # Software components
        if self.PSA_CLAIM_SW_COMPONENTS in claims:
            sw_components = claims[self.PSA_CLAIM_SW_COMPONENTS]
            decoded['sw_components'] = []

            for component in sw_components:
                comp_info = {
                    'type': component.get(1, 'Unknown'),
                    'measurement': component.get(2, b'').hex() if isinstance(component.get(2), bytes) else '',
                    'version': component.get(4, 'Unknown'),
                    'signer_id': component.get(5, b'').hex() if isinstance(component.get(5), bytes) else '',
                }
                decoded['sw_components'].append(comp_info)

        # Profile
        if self.PSA_CLAIM_PROFILE_DEFINITION in claims:
            decoded['profile'] = claims[self.PSA_CLAIM_PROFILE_DEFINITION]

        # Security lifecycle
        if self.PSA_CLAIM_SECURITY_LIFECYCLE in claims:
            lifecycle = claims[self.PSA_CLAIM_SECURITY_LIFECYCLE]
            lifecycle_names = {
                0x0000: 'Unknown',
                0x1000: 'Assembly and test',
                0x2000: 'PSA RoT provisioning',
                0x3000: 'Secured',
                0x4000: 'Non-PSA RoT debug',
                0x5000: 'Recoverable PSA RoT debug',
                0x6000: 'Decommissioned',
            }
            decoded['lifecycle'] = lifecycle_names.get(lifecycle, f'Unknown (0x{lifecycle:04x})')

        # Nonce
        if self.PSA_CLAIM_NONCE in claims:
            nonce = claims[self.PSA_CLAIM_NONCE]
            decoded['nonce'] = nonce.hex() if isinstance(nonce, bytes) else str(nonce)

        return decoded

# Global verifier instance
attestation_verifier = AttestationVerifier()

# ============================================================================
# HTTP API Endpoints
# ============================================================================

@app.route('/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    return jsonify({
        'status': 'healthy',
        'timestamp': datetime.now().isoformat(),
        'version': '1.0.0'
    })

@app.route('/api/v1/device/register', methods=['POST'])
def register_device():
    """
    Register new device

    Request body:
    {
        "device_id": "unique-device-id",
        "device_name": "My Tracker",
        "manufacturer": "Acme Corp",
        "model": "Tracker-v1",
        "hw_version": "1.0",
        "public_key": "PEM-encoded public key"
    }
    """
    data = request.json

    required_fields = ['device_id', 'public_key']
    for field in required_fields:
        if field not in data:
            return jsonify({'error': f'Missing required field: {field}'}), 400

    try:
        conn = sqlite3.connect(CONFIG['database'])
        c = conn.cursor()

        # Check if device already exists
        c.execute('SELECT device_id FROM devices WHERE device_id = ?', (data['device_id'],))
        if c.fetchone():
            return jsonify({'error': 'Device already registered'}), 409

        # Insert device
        c.execute('''
            INSERT INTO devices (device_id, device_name, manufacturer, model, hw_version, public_key)
            VALUES (?, ?, ?, ?, ?, ?)
        ''', (
            data['device_id'],
            data.get('device_name', 'Unknown'),
            data.get('manufacturer', 'Unknown'),
            data.get('model', 'Unknown'),
            data.get('hw_version', 'Unknown'),
            data['public_key']
        ))

        conn.commit()
        conn.close()

        print(f"✓ Device registered: {data['device_id']}")

        return jsonify({
            'status': 'registered',
            'device_id': data['device_id'],
            'message': 'Device registered successfully'
        }), 201

    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/v1/device/attest', methods=['POST'])
def verify_attestation():
    """
    Verify device attestation token

    Request body:
    {
        "device_id": "unique-device-id",
        "token": "base64-encoded attestation token",
        "challenge": "base64-encoded challenge"
    }
    """
    data = request.json

    required_fields = ['device_id', 'token', 'challenge']
    for field in required_fields:
        if field not in data:
            return jsonify({'error': f'Missing required field: {field}'}), 400

    try:
        # Decode token and challenge
        token_bytes = base64.b64decode(data['token'])
        challenge_bytes = base64.b64decode(data['challenge'])

        print(f"Verifying attestation for device: {data['device_id']}")
        print(f"Token size: {len(token_bytes)} bytes")
        print(f"Challenge: {challenge_bytes.hex()}")

        # Verify token
        result = attestation_verifier.verify_token(token_bytes, challenge_bytes)

        # Store attestation
        conn = sqlite3.connect(CONFIG['database'])
        c = conn.cursor()

        c.execute('''
            INSERT INTO attestation_tokens (device_id, token_data, verified)
            VALUES (?, ?, ?)
        ''', (data['device_id'], data['token'], result['verified']))

        # Update device status if verified
        if result['verified']:
            c.execute('''
                UPDATE devices
                SET attestation_verified = 1, status = 'verified', last_seen = CURRENT_TIMESTAMP
                WHERE device_id = ?
            ''', (data['device_id'],))

        conn.commit()
        conn.close()

        return jsonify({
            'verified': result['verified'],
            'claims': result['claims'],
            'errors': result.get('errors', []),
            'verified_with': result.get('verified_with')
        }), 200

    except Exception as e:
        import traceback
        traceback.print_exc()
        return jsonify({'error': str(e), 'verified': False}), 500

@app.route('/api/v1/device/<device_id>/challenge', methods=['GET'])
def get_challenge(device_id):
    """
    Generate challenge nonce for device attestation

    Returns 32-byte random challenge
    """
    try:
        challenge = os.urandom(32)  # 32-byte random nonce

        # Store challenge for verification (in production, use Redis or similar)
        # For now, just return it

        return jsonify({
            'challenge': base64.b64encode(challenge).decode('utf-8'),
            'device_id': device_id,
            'expires_at': (datetime.now() + timedelta(minutes=5)).isoformat()
        }), 200

    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/v1/firmware/upload', methods=['POST'])
def upload_firmware():
    """
    Upload new firmware version

    Multipart form data:
    - file: Firmware binary file
    - version: Firmware version string
    - security_counter: Security counter value
    - description: Optional description
    """
    if 'file' not in request.files:
        return jsonify({'error': 'No file provided'}), 400

    file = request.files['file']
    version = request.form.get('version')
    security_counter = request.form.get('security_counter', 0)
    description = request.form.get('description', '')

    if not version:
        return jsonify({'error': 'Version required'}), 400

    try:
        # Create firmware directory if not exists
        fw_dir = Path(CONFIG['firmware_dir'])
        fw_dir.mkdir(exist_ok=True)

        # Save file
        filename = f"firmware_{version}.bin"
        filepath = fw_dir / filename
        file.save(filepath)

        # Calculate SHA256
        sha256 = hashlib.sha256()
        with open(filepath, 'rb') as f:
            for chunk in iter(lambda: f.read(4096), b''):
                sha256.update(chunk)
        file_hash = sha256.hexdigest()

        # Get file size
        file_size = filepath.stat().st_size

        # Store in database
        conn = sqlite3.connect(CONFIG['database'])
        c = conn.cursor()

        c.execute('''
            INSERT INTO firmware (version, filename, size, sha256, security_counter, description)
            VALUES (?, ?, ?, ?, ?, ?)
        ''', (version, filename, file_size, file_hash, int(security_counter), description))

        firmware_id = c.lastrowid
        conn.commit()
        conn.close()

        print(f"✓ Firmware uploaded: {version} ({file_size} bytes)")

        return jsonify({
            'firmware_id': firmware_id,
            'version': version,
            'size': file_size,
            'sha256': file_hash,
            'filename': filename
        }), 201

    except Exception as e:
        import traceback
        traceback.print_exc()
        return jsonify({'error': str(e)}), 500

@app.route('/api/v1/firmware/latest', methods=['GET'])
def get_latest_firmware():
    """Get latest firmware version"""
    try:
        conn = sqlite3.connect(CONFIG['database'])
        c = conn.cursor()

        c.execute('''
            SELECT id, version, filename, size, sha256, security_counter, description
            FROM firmware
            ORDER BY id DESC
            LIMIT 1
        ''')

        row = c.fetchone()
        conn.close()

        if not row:
            return jsonify({'error': 'No firmware available'}), 404

        return jsonify({
            'firmware_id': row[0],
            'version': row[1],
            'filename': row[2],
            'size': row[3],
            'sha256': row[4],
            'security_counter': row[5],
            'description': row[6]
        }), 200

    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/v1/firmware/download/<int:firmware_id>', methods=['GET'])
def download_firmware(firmware_id):
    """Download firmware binary"""
    try:
        conn = sqlite3.connect(CONFIG['database'])
        c = conn.cursor()

        c.execute('SELECT filename FROM firmware WHERE id = ?', (firmware_id,))
        row = c.fetchone()
        conn.close()

        if not row:
            return jsonify({'error': 'Firmware not found'}), 404

        filepath = Path(CONFIG['firmware_dir']) / row[0]

        if not filepath.exists():
            return jsonify({'error': 'Firmware file not found'}), 404

        return send_file(filepath, as_attachment=True)

    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/v1/device/<device_id>/telemetry', methods=['POST'])
def submit_telemetry(device_id):
    """
    Submit telemetry data from device

    Request body:
    {
        "type": "gps" | "sensor" | "status",
        "data": { ... telemetry data ... }
    }
    """
    data = request.json

    try:
        conn = sqlite3.connect(CONFIG['database'])
        c = conn.cursor()

        # Update last seen
        c.execute('''
            UPDATE devices SET last_seen = CURRENT_TIMESTAMP WHERE device_id = ?
        ''', (device_id,))

        # Store telemetry
        c.execute('''
            INSERT INTO telemetry (device_id, data_type, data)
            VALUES (?, ?, ?)
        ''', (device_id, data.get('type', 'unknown'), json.dumps(data.get('data', {}))))

        conn.commit()
        conn.close()

        return jsonify({'status': 'received'}), 200

    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/v1/device/<device_id>/telemetry', methods=['GET'])
def get_telemetry(device_id):
    """Get telemetry data for device"""
    limit = request.args.get('limit', 100, type=int)
    data_type = request.args.get('type')

    try:
        conn = sqlite3.connect(CONFIG['database'])
        c = conn.cursor()

        if data_type:
            c.execute('''
                SELECT timestamp, data_type, data
                FROM telemetry
                WHERE device_id = ? AND data_type = ?
                ORDER BY timestamp DESC
                LIMIT ?
            ''', (device_id, data_type, limit))
        else:
            c.execute('''
                SELECT timestamp, data_type, data
                FROM telemetry
                WHERE device_id = ?
                ORDER BY timestamp DESC
                LIMIT ?
            ''', (device_id, limit))

        rows = c.fetchall()
        conn.close()

        telemetry = []
        for row in rows:
            telemetry.append({
                'timestamp': row[0],
                'type': row[1],
                'data': json.loads(row[2])
            })

        return jsonify({
            'device_id': device_id,
            'count': len(telemetry),
            'telemetry': telemetry
        }), 200

    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/v1/devices', methods=['GET'])
def list_devices():
    """List all registered devices"""
    try:
        conn = sqlite3.connect(CONFIG['database'])
        c = conn.cursor()

        c.execute('''
            SELECT device_id, device_name, manufacturer, model, hw_version, sw_version,
                   registered_at, last_seen, attestation_verified, status
            FROM devices
            ORDER BY registered_at DESC
        ''')

        rows = c.fetchall()
        conn.close()

        devices = []
        for row in rows:
            devices.append({
                'device_id': row[0],
                'device_name': row[1],
                'manufacturer': row[2],
                'model': row[3],
                'hw_version': row[4],
                'sw_version': row[5],
                'registered_at': row[6],
                'last_seen': row[7],
                'attestation_verified': bool(row[8]),
                'status': row[9]
            })

        return jsonify({
            'count': len(devices),
            'devices': devices
        }), 200

    except Exception as e:
        return jsonify({'error': str(e)}), 500

# ============================================================================
# Main
# ============================================================================

def main():
    """Main entry point"""
    print("=" * 60)
    print(" TF-M Cloud Server")
    print("=" * 60)
    print()

    # Initialize
    init_database()

    # Create directories
    Path(CONFIG['firmware_dir']).mkdir(exist_ok=True)
    Path(CONFIG['cert_dir']).mkdir(exist_ok=True)

    print()
    print("Server Configuration:")
    print(f"  Port: {CONFIG['server_port']}")
    print(f"  Database: {CONFIG['database']}")
    print(f"  Firmware dir: {CONFIG['firmware_dir']}")
    print(f"  Certificate dir: {CONFIG['cert_dir']}")
    print(f"  Attestation verification: {CONFIG['attestation_verify']}")
    print()

    print("API Endpoints:")
    print("  GET  /health")
    print("  POST /api/v1/device/register")
    print("  GET  /api/v1/device/<id>/challenge")
    print("  POST /api/v1/device/attest")
    print("  POST /api/v1/firmware/upload")
    print("  GET  /api/v1/firmware/latest")
    print("  GET  /api/v1/firmware/download/<id>")
    print("  POST /api/v1/device/<id>/telemetry")
    print("  GET  /api/v1/device/<id>/telemetry")
    print("  GET  /api/v1/devices")
    print()

    print("✓ Server ready!")
    print()

    # Start server
    app.run(host='0.0.0.0', port=CONFIG['server_port'], debug=True)

if __name__ == '__main__':
    main()

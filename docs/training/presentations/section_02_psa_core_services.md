---
marp: true
theme: default
paginate: true
backgroundColor: #ffffff
header: 'TF-M Training Package - Section 2'
footer: 'PSA Core Services | © 2025'
style: |
  section {
    font-family: 'Arial', sans-serif;
  }
  h1 {
    color: #00AA00;
    font-size: 44pt;
  }
  h2 {
    color: #0066CC;
    font-size: 32pt;
  }
  code {
    background: #1E1E1E;
    color: #D4D4D4;
    font-family: 'Consolas', monospace;
    font-size: 16pt;
  }
---

<!-- _class: lead -->
<!-- _paginate: false -->

# PSA Core Services

**Crypto, Storage & Attestation**

![bg right:40%](https://via.placeholder.com/400x300/0066CC/ffffff?text=PSA+Certified)

*TF-M Training Package - Section 2*

---

## Why Cryptography Matters

### Three Pillars of Security

**Confidentiality** 🔒
- Keep data secret (encryption)

**Authenticity** ✍️
- Verify sender identity (signatures)

**Integrity** ✓
- Detect tampering (hashing/MAC)

**Without crypto, there is no security**

---

## PSA Crypto Architecture

### Hardware Abstraction Layer

```
┌────────────────────────────────────────┐
│      Application Code                  │
├────────────────────────────────────────┤
│      PSA Crypto API                    │
│  (Algorithm-agnostic interface)        │
├────────────────────────────────────────┤
│      Crypto Driver Layer               │
│  ┌──────────┐  ┌────────────────────┐ │
│  │ Software │  │ Hardware Accel     │ │
│  │ mbedTLS  │  │ (AES, ECDSA, SHA)  │ │
│  └──────────┘  └────────────────────┘ │
└────────────────────────────────────────┘
```

**Benefit:** Change hardware without changing code

---

## Key Management

### Key Lifecycle

```
1. GENERATE
   └──> psa_generate_key()

2. USE
   └──> psa_cipher_encrypt()
   └──> psa_sign_hash()

3. DESTROY
   └──> psa_destroy_key()
```

**Keys stored securely in TF-M Internal Trusted Storage**

---

## Symmetric Encryption

### AES (Advanced Encryption Standard)

**Key Sizes:**
- AES-128 (16 bytes)
- AES-256 (32 bytes) ← **Recommended**

**Modes:**
- CBC (Cipher Block Chaining)
- CTR (Counter)
- **GCM** (Galois/Counter Mode) ← **Best**

**GCM = Encryption + Authentication**

---

## AES-GCM Explained

```
┌──────────┐     ┌──────────┐     ┌────────────┐
│Plaintext │────>│ AES-GCM  │────>│ Ciphertext │
│  +Nonce  │     │   +Key   │     │    +Tag    │
└──────────┘     └──────────┘     └────────────┘

Tag (16 bytes) = Integrity proof
```

**Benefits:**
- ✅ Confidentiality (AES encryption)
- ✅ Integrity (GMAC authentication tag)
- ✅ Fast hardware acceleration

---

## Code Example: AES Encryption

```c
/* Generate AES-256-GCM key */
psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
psa_set_key_usage_flags(&attributes,
    PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
psa_set_key_algorithm(&attributes, PSA_ALG_GCM);
psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
psa_set_key_bits(&attributes, 256);

psa_key_id_t key_id;
psa_generate_key(&attributes, &key_id);

/* Encrypt data */
uint8_t nonce[12];
psa_generate_random(nonce, sizeof(nonce));

psa_aead_encrypt(key_id, PSA_ALG_GCM,
                 nonce, sizeof(nonce),
                 NULL, 0,  /* No additional data */
                 plaintext, plaintext_len,
                 ciphertext, ciphertext_size, &ciphertext_len);
```

---

## Asymmetric Cryptography

### Public/Private Key Pairs

**Use Cases:**
- Digital signatures (ECDSA, RSA)
- Key exchange (ECDH)
- Certificate-based authentication

**Algorithms:**
- **ECDSA P-256** ← Recommended (IoT)
- RSA-2048/4096 (legacy systems)
- Ed25519 (modern, fast)

---

## Digital Signatures (ECDSA)

```c
/* Generate P-256 key pair */
psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
psa_set_key_usage_flags(&attributes,
    PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH);
psa_set_key_algorithm(&attributes,
    PSA_ALG_ECDSA(PSA_ALG_SHA_256));
psa_set_key_type(&attributes,
    PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
psa_set_key_bits(&attributes, 256);

psa_key_id_t key_id;
psa_generate_key(&attributes, &key_id);

/* Sign data */
uint8_t signature[64];
psa_sign_hash(key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256),
              hash, sizeof(hash),
              signature, sizeof(signature), &signature_len);
```

---

## Hash Functions

### SHA-256 (Secure Hash Algorithm)

```c
/* Compute SHA-256 hash */
uint8_t hash[32];
size_t hash_len;

psa_hash_compute(PSA_ALG_SHA_256,
                 data, data_len,
                 hash, sizeof(hash),
                 &hash_len);

printf("SHA-256: ");
for (int i = 0; i < 32; i++) {
    printf("%02x", hash[i]);
}
```

**Properties:**
- Fixed output size (32 bytes)
- One-way function (cannot reverse)
- Collision-resistant

---

## HMAC (Hash-based MAC)

### Message Authentication Code

```c
/* Generate HMAC key */
psa_key_id_t hmac_key;
psa_generate_key(&attributes, &hmac_key);

/* Compute HMAC-SHA256 */
uint8_t mac[32];
psa_mac_compute(hmac_key, PSA_ALG_HMAC(PSA_ALG_SHA_256),
                message, message_len,
                mac, sizeof(mac), &mac_len);

/* Verify HMAC */
psa_mac_verify(hmac_key, PSA_ALG_HMAC(PSA_ALG_SHA_256),
               message, message_len,
               mac, mac_len);
```

**Use Case:** API authentication, message integrity

---

## Key Derivation (KDF)

### Deriving Keys from Master Secret

```c
/* HKDF (HMAC-based KDF) */
psa_key_derivation_operation_t op =
    PSA_KEY_DERIVATION_OPERATION_INIT;

psa_key_derivation_setup(&op, PSA_ALG_HKDF(PSA_ALG_SHA_256));
psa_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_SALT,
                                 salt, sizeof(salt));
psa_key_derivation_input_key(&op, PSA_KEY_DERIVATION_INPUT_SECRET,
                               master_key);
psa_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_INFO,
                                 info, sizeof(info));

/* Derive AES key */
psa_key_derivation_output_key(&attributes, &op, &derived_key);
```

---

## Random Number Generation

### TRNG vs PRNG

**TRNG (True RNG):**
- Hardware noise source
- Physical entropy
- Unpredictable

**PRNG (Pseudo RNG):**
- Algorithm-based
- Seeded by TRNG
- Fast but deterministic

```c
/* PSA uses hardware TRNG */
uint8_t random_bytes[32];
psa_generate_random(random_bytes, sizeof(random_bytes));
```

---

## Lab 03: Interactive Crypto Demo

### NUCLEO-U545RE-Q Demo

**Features:**
- Press button to encrypt/decrypt
- LED feedback:
  - 🟢 Green: Encryption success
  - 🔵 Blue: Decryption success
  - 🔴 Red: Verification failed

**Operations:**
- AES-256-GCM encryption
- ECDSA P-256 signing
- SHA-256 hashing

---

## Security Considerations

### Side-Channel Attacks

**Timing Attacks:**
- Measure execution time to extract keys
- **Mitigation:** Constant-time algorithms

**Power Analysis:**
- Monitor power consumption patterns
- **Mitigation:** Hardware countermeasures

**Electromagnetic Analysis:**
- Measure EM emissions
- **Mitigation:** Shielding, noise injection

---

## Best Practices: Nonce Management

### CRITICAL: Never Reuse Nonces!

❌ **WRONG:**
```c
uint8_t nonce[12] = {0};  /* Static nonce */
for (int i = 0; i < 100; i++) {
    psa_aead_encrypt(..., nonce, ...);  /* REUSED! */
}
```

✅ **CORRECT:**
```c
for (int i = 0; i < 100; i++) {
    uint8_t nonce[12];
    psa_generate_random(nonce, sizeof(nonce));
    psa_aead_encrypt(..., nonce, ...);
}
```

**Nonce reuse = Complete security failure**

---

## Best Practices: Zeroize Secrets

### Clear Sensitive Data After Use

```c
void process_password(const char *password)
{
    uint8_t key[32];

    /* Derive key from password */
    derive_key(password, key);

    /* Use key */
    encrypt_data(key, ...);

    /* CRITICAL: Zeroize key */
    memset(key, 0, sizeof(key));

    /* Even better: Use secure zeroize */
    psa_crypto_init();  /* Ensures compiler doesn't optimize out */
}
```

---

## Why Secure Storage?

### Protecting Critical Data

**What to Store:**
- Cryptographic keys
- API credentials
- Device identity
- Configuration secrets
- User credentials

**Threats:**
- Flash memory dumps
- Debugger extraction
- Malware access

**Solution:** PSA Secure Storage

---

## ITS vs PS

### Two Storage Services

| Feature | ITS (Internal Trusted Storage) | PS (Protected Storage) |
|---------|-------------------------------|------------------------|
| **Location** | Internal flash only | Internal or external |
| **Capacity** | Small (~64 KB) | Large (MB+) |
| **Rollback** | Yes | Optional |
| **Use Case** | Keys, device ID | App data, logs |
| **Speed** | Fast | Medium |

---

## ITS (Internal Trusted Storage)

### Critical Data Storage

```c
/* Store device private key */
#define UID_DEVICE_KEY  0x00000001

uint8_t private_key[32] = { /* ... */ };

psa_status_t status = psa_its_set(
    UID_DEVICE_KEY,
    sizeof(private_key),
    private_key,
    PSA_STORAGE_FLAG_WRITE_ONCE  /* Immutable */
);

if (status == PSA_SUCCESS) {
    printf("Key stored securely\n");
}
```

---

## Retrieving from ITS

```c
/* Read device private key */
uint8_t key_buffer[32];
size_t actual_len;

psa_status_t status = psa_its_get(
    UID_DEVICE_KEY,
    0,  /* offset */
    sizeof(key_buffer),
    key_buffer,
    &actual_len
);

if (status == PSA_SUCCESS) {
    /* Use key */
    sign_data(key_buffer, ...);

    /* Zeroize after use */
    memset(key_buffer, 0, sizeof(key_buffer));
}
```

---

## Storage APIs

### Complete API Reference

```c
/* Set (create or update) */
psa_its_set(uid, data_len, data, flags);

/* Get (read) */
psa_its_get(uid, offset, size, buffer, &actual_len);

/* Get info (size, flags) */
struct psa_storage_info_t info;
psa_its_get_info(uid, &info);

/* Remove (delete) */
psa_its_remove(uid);
```

**UIDs must be unique across application**

---

## Data Encryption at Rest

### Automatic Encryption

```
User Data (plaintext)
        │
        ▼
   [PSA ITS/PS]
        │
        ▼
  Device-unique key
        │
        ▼
   AES-256-GCM
        │
        ▼
  Flash Memory (ciphertext)
```

**Transparent to application code**

---

## Write-Once Protection

### Immutable Storage

```c
/* Store certificate (cannot be modified) */
psa_its_set(UID_DEVICE_CERT,
            cert_len,
            certificate,
            PSA_STORAGE_FLAG_WRITE_ONCE);

/* Attempting to modify will fail */
psa_status_t status = psa_its_set(UID_DEVICE_CERT, ...);
/* status == PSA_ERROR_NOT_PERMITTED */
```

**Use for:**
- Device certificates
- Factory calibration data
- Permanent device ID

---

## Rollback Protection

### Preventing Downgrade Attacks

**Scenario:** Attacker restores old flash image with known vulnerability

**Protection:**
```c
/* Storage protected by monotonic counter */
uint32_t counter;
psa_its_get(UID_ROLLBACK_COUNTER, 0, 4, &counter, NULL);

if (new_version > counter) {
    counter = new_version;
    psa_its_set(UID_ROLLBACK_COUNTER, 4, &counter, 0);
} else {
    reject_update();  /* Rollback attack! */
}
```

---

## Use Case: API Key Storage

### Secure Cloud Credentials

```c
#define UID_AWS_API_KEY  0x00001000

void provision_api_key(const char *api_key)
{
    psa_its_set(UID_AWS_API_KEY,
                strlen(api_key) + 1,
                api_key,
                0);

    printf("API key stored securely\n");
}

void connect_to_cloud(void)
{
    char api_key[64];
    psa_its_get(UID_AWS_API_KEY, 0, sizeof(api_key),
                api_key, NULL);

    aws_connect(api_key);

    memset(api_key, 0, sizeof(api_key));  /* Zeroize */
}
```

---

## Use Case: Device Provisioning

### Factory Programming

```c
void factory_provision(void)
{
    /* Generate unique device ID */
    uint8_t device_id[16];
    psa_generate_random(device_id, sizeof(device_id));

    /* Store permanently */
    psa_its_set(UID_DEVICE_ID,
                sizeof(device_id),
                device_id,
                PSA_STORAGE_FLAG_WRITE_ONCE);

    /* Generate device key pair */
    psa_key_id_t key_id;
    psa_generate_key(&attributes, &key_id);

    /* Export public key for certificate */
    uint8_t pubkey[65];
    psa_export_public_key(key_id, pubkey, sizeof(pubkey), NULL);

    /* Create and store certificate */
    store_certificate(device_id, pubkey);
}
```

---

## Lab 04: Secure Storage Demo

### Interactive Storage Example

**Demonstration:**
1. Store API credentials via UART
2. Blink LED to confirm storage
3. Reboot device (power cycle)
4. Retrieve credentials successfully
5. Try write-once violation (LED red)

**Persistent through:**
- Reboots
- Power cycles
- Firmware updates

---

## Storage Security Analysis

### Encryption Details

**AES-256-GCM Parameters:**
- Key: Derived from device-unique secret (OTP)
- Nonce: Generated per write operation
- AAD: UID + metadata
- Tag: 16 bytes (128-bit authentication)

**Key Derivation:**
```
Device Root Key (OTP, 256-bit)
    └──> HKDF-SHA256
          └──> Storage Encryption Key (per-UID)
```

---

## Attack Scenarios

### Flash Dump Attack

**Attack:** Attacker extracts flash memory chip

**Protection:**
- Data encrypted with device-unique key
- Key stored in OTP (One-Time Programmable)
- Cannot decrypt on another device

### Rollback Attack

**Attack:** Restore old flash image

**Protection:**
- Monotonic counters in OTP
- Version downgrade rejected

---

## What is Attestation?

### Proving Device Identity and State

**Attestation Token Contains:**
- Device unique ID
- Firmware version and hash
- Boot measurements
- Security lifecycle state
- Hardware configuration

**Purpose:** Remote verification before granting access

---

## Attestation Use Cases

### When is Attestation Used?

1. **Device Onboarding**
   - Cloud verifies device authenticity

2. **Secure Firmware Update**
   - Server checks device is genuine

3. **Zero Trust Architecture**
   - Continuous device verification

4. **Compliance Audits**
   - Prove security configuration

---

## Attestation Token Format

### CBOR + COSE

```
COSE_Sign1 (signed token)
├── Protected headers
│   └── Algorithm: ECDSA P-256 + SHA-256
├── Unprotected headers
│   └── Key ID
├── Payload (CBOR-encoded claims)
│   ├── Device ID
│   ├── Firmware version
│   ├── Boot measurements
│   └── Lifecycle state
└── Signature (64 bytes)
```

**CBOR** = Compact Binary Object Representation
**COSE** = CBOR Object Signing and Encryption

---

## Claims in Attestation Token

```json
{
  "device-id": "a1b2c3d4e5f6...",
  "firmware-version": "1.2.3",
  "boot-measurements": [
    {"slot": 0, "hash": "3a4b5c..."},
    {"slot": 1, "hash": "7d8e9f..."}
  ],
  "lifecycle": "SECURED",
  "timestamp": 1234567890,
  "nonce": "random_challenge"
}
```

**Nonce prevents replay attacks**

---

## Generating Attestation Token

```c
/* Challenge from server (prevents replay) */
uint8_t challenge[32];
receive_from_server(challenge, sizeof(challenge));

/* Generate attestation token */
uint8_t token_buf[1024];
size_t token_len;

psa_status_t status = psa_initial_attest_get_token(
    challenge,
    sizeof(challenge),
    token_buf,
    sizeof(token_buf),
    &token_len
);

/* Send to server for verification */
send_to_server(token_buf, token_len);
```

---

## Token Verification Process

```
Device                          Server
  │                              │
  │ ◄──── Challenge (32 bytes)───│
  │                              │
  │ ──── Token (signed) ────────►│
  │                              │
  │                          [Verify]
  │                          1. Check signature
  │                          2. Validate nonce
  │                          3. Check FW version
  │                          4. Verify device ID
  │                              │
  │ ◄──── Access Granted/Denied──│
```

---

## Attestation Keys

### Device Attestation Key (DAK)

```c
/* Provisioned at factory */
psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
psa_set_key_usage_flags(&attributes,
    PSA_KEY_USAGE_SIGN_HASH);
psa_set_key_algorithm(&attributes,
    PSA_ALG_ECDSA(PSA_ALG_SHA_256));
psa_set_key_type(&attributes,
    PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
psa_set_key_bits(&attributes, 256);
psa_set_key_lifetime(&attributes,
    PSA_KEY_LIFETIME_PERSISTENT);

psa_key_id_t dak_key_id = DAK_KEY_ID;
psa_generate_key(&attributes, &dak_key_id);
```

**Never leaves device, used only for signing**

---

## Lab 05: Attestation Example

### Generate and Verify Token

**Steps:**
1. Generate attestation token
2. Parse CBOR claims
3. Display on UART:
   ```
   Device ID: a1b2c3d4...
   FW Version: 1.0.0
   Boot State: VERIFIED
   Lifecycle: SECURED
   ```
4. Verify signature locally (for demo)

**Real deployment:** Server verifies signature

---

## Cloud Integration: AWS IoT Core

### Device Attestation Flow

```c
void aws_device_onboarding(void)
{
    /* 1. Connect to AWS IoT */
    mqtt_connect("a1b2c3d4.iot.us-east-1.amazonaws.com");

    /* 2. Request challenge */
    uint8_t challenge[32];
    mqtt_publish("$aws/provisioning/challenge/request", "");
    mqtt_receive("$aws/provisioning/challenge", challenge, 32);

    /* 3. Generate attestation token */
    uint8_t token[1024];
    size_t token_len;
    psa_initial_attest_get_token(challenge, 32, token, 1024, &token_len);

    /* 4. Send token */
    mqtt_publish("$aws/provisioning/token", token, token_len);

    /* 5. Receive credentials */
    receive_aws_credentials();
}
```

---

## Cloud Integration: Azure IoT Hub

```c
void azure_device_registration(void)
{
    /* Device Provisioning Service (DPS) */
    https_post("global.azure-devices-provisioning.net",
               "/registrations/<id>/register",
               attestation_token);

    /* Receive IoT Hub assignment */
    char iot_hub[128];
    parse_response(iot_hub);

    /* Connect to assigned hub */
    mqtt_connect(iot_hub);
}
```

**Azure verifies token before hub assignment**

---

## Section Summary

### What You Learned

✓ **PSA Crypto API**
  - AES-256-GCM for encryption
  - ECDSA P-256 for signatures
  - SHA-256 for hashing
  - HMAC for authentication

✓ **Secure Storage**
  - ITS for critical data
  - PS for application data
  - Write-once protection
  - Rollback prevention

✓ **Attestation**
  - Device identity proof
  - Remote verification
  - Cloud integration

---

## Real-World Application

### GPS Tracker Security

**Scenario:** Secure GPS tracking device

```c
void gps_tracker_security(void)
{
    /* 1. Store API key securely */
    psa_its_set(UID_API_KEY, key_len, api_key, 0);

    /* 2. Encrypt location data */
    psa_aead_encrypt(key_id, PSA_ALG_GCM, ...);

    /* 3. Generate attestation for cloud */
    psa_initial_attest_get_token(...);

    /* 4. Secure communication */
    tls_connect_with_psa_crypto();
}
```

**All PSA services working together**

---

## Next Section Preview

### Section 3: Advanced TF-M Topics

**Topics:**
- Custom Secure Services
- Partition Design
- IPC Mechanism
- Isolation Levels
- Performance Tuning

**Labs:**
- Lab 06: MCUboot firmware update
- Lab 07: Secure boot measurements
- Lab 08: Advanced protected storage

---

## Additional Resources

### Documentation

- **PSA Crypto API:** arm-software.github.io/psa-api/crypto
- **PSA Storage API:** arm-software.github.io/psa-api/storage
- **PSA Attestation:** arm-software.github.io/psa-api/attestation
- **TF-M Docs:** tf-m-user-guide.trustedfirmware.org

### Code Examples
- GitHub: github.com/TrustedFirmwareM/tf-m-tests

---

<!-- _class: lead -->
<!-- _paginate: false -->

# Questions?

**Contact:** training@example.com
**Labs:** Lab 03 (Crypto), Lab 04 (Storage), Lab 05 (Attestation)

**Next:** Section 3 - Advanced TF-M Topics

---

**End of Section 2**
*Total Slides: 55*
*Estimated Duration: 3 hours*

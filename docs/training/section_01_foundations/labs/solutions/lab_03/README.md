# Lab 03: PSA Crypto API - Hands-On Encryption

**Objective:** Learn PSA Crypto API through hands-on encryption/decryption with visual feedback

**Hardware:** NUCLEO-U545RE-Q

**Duration:** 45 minutes

**Difficulty:** Beginner

---

## What You'll Learn

✓ PSA Crypto API initialization
✓ Key generation and management
✓ AES-256-GCM encryption/decryption
✓ Hash computation (SHA-256)
✓ HMAC for data integrity
✓ Random number generation
✓ Visual feedback with LEDs

---

## Hardware Setup

### Components Needed
- NUCLEO-U545RE-Q board
- USB cable (ST-LINK + serial console)
- Terminal program (115200 baud)

### LED Indicators
- **LD1 (Green):** Crypto operation success
- **LD2 (Blue):** Crypto operation in progress
- **LD3 (Red):** Crypto operation failed

### Button
- **B1 (Blue button):** Trigger crypto operations

---

## Theory Background

### PSA Crypto API Overview

The **PSA Crypto API** (Platform Security Architecture) provides a standardized interface for cryptographic operations on ARM devices.

**Key Benefits:**
- Hardware acceleration when available
- Secure key storage (keys never leave secure world)
- Side-channel attack resistance
- Standardized across vendors

**Basic Workflow:**
```
1. Initialize PSA Crypto: psa_crypto_init()
2. Generate/Import key: psa_generate_key()
3. Perform operation: psa_cipher_encrypt(), psa_hash_compute(), etc.
4. Destroy key when done: psa_destroy_key()
```

### AES-256-GCM

**AES-256-GCM** (Galois/Counter Mode) provides:
- **Confidentiality:** Data is encrypted (AES-256)
- **Integrity:** Tampering is detected (GCM authentication tag)
- **Performance:** Can be hardware-accelerated

**Parameters:**
- Key size: 256 bits (32 bytes)
- Nonce size: 96 bits (12 bytes) - must be unique per message!
- Tag size: 128 bits (16 bytes)

### HMAC-SHA256

**HMAC** (Hash-based Message Authentication Code) ensures data integrity and authenticity.

**Use Cases:**
- Verify data hasn't been tampered with
- Authenticate messages between devices
- Password verification (with proper key derivation)

---

## Experiments

### Experiment 1: Initialize PSA Crypto

**What You'll Do:** Initialize the PSA Crypto subsystem

**Expected Output:**
```
[CRYPTO] Initializing PSA Crypto...
[CRYPTO] ✓ PSA Crypto initialized
LD1 blinks GREEN (success)
```

**Code:**
```c
psa_status_t status = psa_crypto_init();
if (status == PSA_SUCCESS) {
    printf("[CRYPTO] ✓ PSA Crypto initialized\n");
    LED_Green_Blink(3);
}
```

---

### Experiment 2: Generate Random Numbers

**What You'll Do:** Generate cryptographically secure random numbers

**Expected Output:**
```
[RANDOM] Generating 16 random bytes...
[RANDOM] Random data: A3 F2 8D 1C 94 B7 E5 22 3A 6F D8 C1 4B 9E 7F 31
LD1 blinks GREEN
```

**Why It Matters:** Secure random numbers are critical for:
- Encryption nonces
- Cryptographic keys
- Session tokens
- Challenge-response protocols

**Code:**
```c
uint8_t random_data[16];
psa_status_t status = psa_generate_random(random_data, sizeof(random_data));

printf("[RANDOM] Random data: ");
for (int i = 0; i < 16; i++) {
    printf("%02X ", random_data[i]);
}
printf("\n");
```

---

### Experiment 3: Generate AES-256 Key

**What You'll Do:** Generate a persistent AES-256 encryption key

**Expected Output:**
```
[KEY] Generating AES-256 key...
[KEY] ✓ Key generated (ID: 1)
[KEY] Key is stored in secure memory
LD1 blinks GREEN (3 times)
```

**Security Note:** The key is generated in secure memory and **never** exposed to your application. You only get a handle (key ID) to reference it.

**Code:**
```c
psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
psa_key_id_t key_id;

// Configure key attributes
psa_set_key_usage_flags(&attributes,
    PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
psa_set_key_algorithm(&attributes, PSA_ALG_GCM);
psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
psa_set_key_bits(&attributes, 256);

// Generate key
psa_status_t status = psa_generate_key(&attributes, &key_id);

if (status == PSA_SUCCESS) {
    printf("[KEY] ✓ Key generated (ID: %lu)\n", key_id);
}
```

---

### Experiment 4: Encrypt Data with AES-256-GCM

**What You'll Do:** Encrypt a message and see the ciphertext

**Expected Output:**
```
[ENCRYPT] Plaintext: "Hello, TF-M Security!"
[ENCRYPT] Plaintext (hex): 48 65 6C 6C 6F 2C 20 54 46 2D 4D 20 53 65 63 75 72 69 74 79 21
[ENCRYPT] Encrypting with AES-256-GCM...
LD2 turns ON (Blue - operation in progress)
[ENCRYPT] ✓ Encryption successful
[ENCRYPT] Ciphertext: 9A 3F 2D 8B C7 E1 5F 43 A2 7D 9C 1E 6F B4 8A D3 2C 4E 7B 9F A1
[ENCRYPT] Tag: 5D 8C 3A 7F 1E 9B 4D 2F A6 C3 8E 1D 7F 4B 9A 2C
LD2 turns OFF, LD1 blinks GREEN (success)
```

**What Happened:**
1. Plain message converted to ciphertext (unreadable)
2. Authentication tag generated (proves integrity)
3. Even 1 bit change in ciphertext → decryption fails!

**Code:**
```c
const char *plaintext = "Hello, TF-M Security!";
uint8_t ciphertext[64];
uint8_t nonce[12];
size_t ciphertext_len;

// Generate random nonce
psa_generate_random(nonce, sizeof(nonce));

// Encrypt
LED_Blue_On();
psa_status_t status = psa_aead_encrypt(
    key_id,                          // Key to use
    PSA_ALG_GCM,                     // Algorithm
    nonce, sizeof(nonce),            // Nonce
    NULL, 0,                         // No additional data
    (uint8_t*)plaintext, strlen(plaintext),
    ciphertext, sizeof(ciphertext),
    &ciphertext_len
);
LED_Blue_Off();

if (status == PSA_SUCCESS) {
    printf("[ENCRYPT] ✓ Encryption successful\n");
    LED_Green_Blink(3);
}
```

---

### Experiment 5: Decrypt Data

**What You'll Do:** Decrypt the ciphertext back to original message

**Expected Output:**
```
[DECRYPT] Decrypting ciphertext...
LD2 turns ON (Blue)
[DECRYPT] ✓ Decryption successful
[DECRYPT] Decrypted: "Hello, TF-M Security!"
[DECRYPT] ✓ Plaintext matches original!
LD2 turns OFF, LD1 blinks GREEN
```

**Code:**
```c
uint8_t decrypted[64];
size_t decrypted_len;

LED_Blue_On();
psa_status_t status = psa_aead_decrypt(
    key_id,
    PSA_ALG_GCM,
    nonce, sizeof(nonce),
    NULL, 0,
    ciphertext, ciphertext_len,
    decrypted, sizeof(decrypted),
    &decrypted_len
);
LED_Blue_Off();

if (status == PSA_SUCCESS) {
    decrypted[decrypted_len] = '\0';
    printf("[DECRYPT] Decrypted: \"%s\"\n", decrypted);

    if (strcmp((char*)decrypted, plaintext) == 0) {
        printf("[DECRYPT] ✓ Plaintext matches original!\n");
        LED_Green_Blink(3);
    }
}
```

---

### Experiment 6: Tamper Detection

**What You'll Do:** Modify 1 bit in ciphertext and try to decrypt

**Expected Output:**
```
[TAMPER] Modifying ciphertext byte 5...
[TAMPER] Original: 0xC7 → Modified: 0xC6
[TAMPER] Attempting to decrypt tampered data...
LD2 turns ON (Blue)
[TAMPER] ✗ DECRYPTION FAILED! (as expected)
[TAMPER] Error code: PSA_ERROR_INVALID_SIGNATURE
[TAMPER] ✓ Tampering detected successfully!
LD2 turns OFF, LD3 blinks RED (tamper detected)
```

**What This Proves:**
GCM's authentication tag detects **any** modification to ciphertext, even a single bit flip!

**Code:**
```c
// Tamper with ciphertext
printf("[TAMPER] Modifying ciphertext byte 5...\n");
printf("[TAMPER] Original: 0x%02X", ciphertext[5]);
ciphertext[5] ^= 0x01;  // Flip 1 bit
printf(" → Modified: 0x%02X\n", ciphertext[5]);

// Try to decrypt
LED_Blue_On();
psa_status_t status = psa_aead_decrypt(
    key_id, PSA_ALG_GCM,
    nonce, sizeof(nonce),
    NULL, 0,
    ciphertext, ciphertext_len,
    decrypted, sizeof(decrypted),
    &decrypted_len
);
LED_Blue_Off();

if (status != PSA_SUCCESS) {
    printf("[TAMPER] ✗ DECRYPTION FAILED! (as expected)\n");
    printf("[TAMPER] ✓ Tampering detected successfully!\n");
    LED_Red_Blink(3);
}
```

---

### Experiment 7: Compute SHA-256 Hash

**What You'll Do:** Compute cryptographic hash of data

**Expected Output:**
```
[HASH] Computing SHA-256 of "Hello, TF-M Security!"
[HASH] SHA-256: 7A 8F 3D 9C 2E 5B 1F 4A 6D 8C 3B 9E 7F 2A 5D 8C
                1F 4E 7B 9A 3D 6C 8F 2E 5B 1A 4D 7C 9F 3E 8B 6A
LD1 blinks GREEN
```

**Use Cases:**
- File integrity verification
- Password storage (with salt!)
- Digital signatures
- Blockchain

**Code:**
```c
const char *data = "Hello, TF-M Security!";
uint8_t hash[32];  // SHA-256 produces 32 bytes
size_t hash_len;

psa_status_t status = psa_hash_compute(
    PSA_ALG_SHA_256,
    (uint8_t*)data, strlen(data),
    hash, sizeof(hash),
    &hash_len
);

if (status == PSA_SUCCESS) {
    printf("[HASH] SHA-256: ");
    for (int i = 0; i < hash_len; i++) {
        if (i % 16 == 0 && i != 0) printf("\n                ");
        printf("%02X ", hash[i]);
    }
    printf("\n");
    LED_Green_Blink(3);
}
```

---

### Experiment 8: HMAC for Message Authentication

**What You'll Do:** Generate and verify HMAC

**Expected Output:**
```
[HMAC] Message: "Transfer $100 to Account 12345"
[HMAC] Computing HMAC-SHA256...
[HMAC] HMAC: 3F 8D 2C 7E 1A 9F 4B 6D 5C 8E 3A 7F 2D 9C 1E 4B
             6A 8F 3D 2C 7E 1A 5F 9B 4D 8C 3E 7A 1F 6D 9C 2E

[HMAC] Verifying HMAC...
[HMAC] ✓ HMAC verification successful!
[HMAC] Message is authentic and unmodified
LD1 blinks GREEN
```

**Real-World Example:**
Banking app sending transaction:
```json
{
  "amount": 100,
  "to_account": "12345",
  "hmac": "3F8D2C7E..."
}
```
Server verifies HMAC to ensure message is authentic.

---

### Experiment 9: Interactive Crypto Demo

**What You'll Do:** Press button to cycle through crypto operations

**User Interaction:**
1. Press B1 → Generate random data
2. Press B1 → Encrypt message
3. Press B1 → Decrypt message
4. Press B1 → Compute hash
5. Press B1 → Compute HMAC
6. (Repeat)

**Expected Behavior:**
- LD2 (Blue) ON during operations
- LD1 (Green) blinks on success
- LD3 (Red) blinks on error
- Serial console shows all results

---

## Building and Running

### 1. Build the Lab

```bash
cd section_01_foundations/labs/solutions/lab_03/
./build.sh
```

### 2. Flash to Board

```bash
./flash.sh
```

### 3. Open Serial Console

```bash
screen /dev/ttyACM0 115200
# OR
minicom -D /dev/ttyACM0 -b 115200
```

### 4. Reset Board

Press the **RESET** button on NUCLEO board

### 5. Interact

Press **B1 (blue button)** to trigger crypto operations

---

## Expected Output

```
═══════════════════════════════════════════════════════════════
  🔐 Lab 03: PSA Crypto API Hands-On
═══════════════════════════════════════════════════════════════
Hardware: NUCLEO-U545RE-Q (STM32U545RET6Q)
Security: TF-M PSA Crypto
─────────────────────────────────────────────────────────────

[INIT] Initializing PSA Crypto...
[INIT] ✓ PSA Crypto initialized

Press B1 button to run crypto experiments...

[BUTTON] Button pressed! Running Experiment 1...

[RANDOM] Generating 16 random bytes...
[RANDOM] Random data: A3 F2 8D 1C 94 B7 E5 22 3A 6F D8 C1 4B 9E 7F 31
[RANDOM] ✓ Success

[KEY] Generating AES-256 key...
[KEY] ✓ Key generated (ID: 1)

[ENCRYPT] Plaintext: "Hello, TF-M Security!"
[ENCRYPT] Encrypting with AES-256-GCM...
[ENCRYPT] ✓ Encryption successful
[ENCRYPT] Ciphertext: 9A 3F 2D 8B C7 E1 5F 43 A2 7D 9C 1E 6F B4 8A D3

[DECRYPT] Decrypting ciphertext...
[DECRYPT] ✓ Decryption successful
[DECRYPT] Decrypted: "Hello, TF-M Security!"
[DECRYPT] ✓ Plaintext matches!

[BUTTON] Press B1 for next experiment...
```

---

## Troubleshooting

### PSA_ERROR_NOT_SUPPORTED
**Problem:** Algorithm not supported by hardware
**Solution:** Check that TF-M is configured with crypto support

### PSA_ERROR_INSUFFICIENT_MEMORY
**Problem:** Out of key slots
**Solution:** Destroy unused keys with `psa_destroy_key()`

### PSA_ERROR_INVALID_ARGUMENT
**Problem:** Invalid parameter (buffer too small, wrong key type, etc.)
**Solution:** Check buffer sizes and key attributes

### No Serial Output
**Problem:** UART not configured
**Solution:** Check that ST-LINK virtual COM port is enabled

---

## Quiz

**Q1:** What does GCM provide beyond basic AES encryption?
**A:** Authentication tag for integrity/tamper detection

**Q2:** Can you read the raw bytes of an AES key after `psa_generate_key()`?
**A:** No! The key stays in secure memory. You only get a handle (key ID).

**Q3:** What happens if you decrypt with the wrong nonce?
**A:** Decryption fails with PSA_ERROR_INVALID_SIGNATURE

**Q4:** Why must the nonce be unique for each encryption?
**A:** Reusing nonces breaks GCM security (allows key recovery attacks!)

**Q5:** What's the difference between hash and HMAC?
**A:** Hash has no key (public), HMAC uses secret key (authenticated)

---

## Key Takeaways

✓ **PSA Crypto API** provides standardized crypto operations
✓ **Keys are protected** - never exposed to application
✓ **AES-256-GCM** provides encryption + integrity
✓ **Nonces must be unique** for each encryption
✓ **Tampering is detected** automatically by GCM tag
✓ **SHA-256** for hashing, **HMAC** for authentication
✓ **Hardware acceleration** when available (transparent)

---

## Next Lab

**Lab 04:** PSA Secure Storage (ITS & PS)
Learn to store keys, credentials, and configuration data securely!

---

**Lab Duration:** ~45 minutes
**Difficulty:** ⭐⭐☆☆☆ Beginner
**Prerequisites:** Lab 01, Lab 02
**Hardware Required:** NUCLEO-U545RE-Q only

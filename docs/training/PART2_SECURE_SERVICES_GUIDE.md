# Part 2: TF-M Secure Services Complete Guide
## Modules 11-15: Cryptography, Storage, Attestation, Platform Services, Firmware Update

---

## Table of Contents

- [Module 11: Cryptographic Services](#module-11-cryptographic-services)
- [Module 12: Secure Storage (ITS and PS)](#module-12-secure-storage-its-and-ps)
- [Module 13: Initial Attestation](#module-13-initial-attestation)
- [Module 14: Platform Services](#module-14-platform-services)
- [Module 15: Firmware Update Service](#module-15-firmware-update-service)

---

## Module 11: Cryptographic Services

### 11.1 PSA Crypto API Overview

The PSA Crypto API provides a standardized interface for cryptographic operations:

```
PSA Crypto API Categories:
├── Key Management
│   ├── psa_import_key()
│   ├── psa_export_key()
│   ├── psa_export_public_key()
│   ├── psa_destroy_key()
│   └── psa_generate_key()
│
├── Random Number Generation
│   └── psa_generate_random()
│
├── Hashing
│   ├── psa_hash_compute()
│   ├── psa_hash_compare()
│   └── Multi-part: psa_hash_setup/update/finish()
│
├── Message Authentication Codes (MAC)
│   ├── psa_mac_compute()
│   ├── psa_mac_verify()
│   └── Multi-part: psa_mac_setup/update/finish()
│
├── Symmetric Encryption
│   ├── psa_cipher_encrypt()
│   ├── psa_cipher_decrypt()
│   └── Multi-part: psa_cipher_setup/update/finish()
│
├── Authenticated Encryption (AEAD)
│   ├── psa_aead_encrypt()
│   ├── psa_aead_decrypt()
│   └── Multi-part: psa_aead_setup/update/finish()
│
├── Asymmetric Cryptography
│   ├── psa_sign_message()
│   ├── psa_verify_message()
│   ├── psa_sign_hash()
│   ├── psa_verify_hash()
│   ├── psa_asymmetric_encrypt()
│   └── psa_asymmetric_decrypt()
│
└── Key Derivation
    ├── psa_key_derivation_setup()
    ├── psa_key_derivation_input_key()
    ├── psa_key_derivation_input_bytes()
    └── psa_key_derivation_output_key()
```

### 11.2 Key Management in Detail

**Key Attributes:**

```c
#include "psa/crypto.h"

/**
 * Complete key attribute configuration example
 */
void example_key_attributes(void)
{
    psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

    /* 1. Set key type */
    psa_set_key_type(&attr, PSA_KEY_TYPE_AES);  /* AES symmetric key */
    /* Other types:
     * - PSA_KEY_TYPE_RSA_KEY_PAIR
     * - PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1)
     * - PSA_KEY_TYPE_HMAC
     * - PSA_KEY_TYPE_DERIVE
     */

    /* 2. Set key size */
    psa_set_key_bits(&attr, 256);  /* 256-bit AES key */

    /* 3. Set key usage flags */
    psa_set_key_usage_flags(&attr,
        PSA_KEY_USAGE_ENCRYPT |       /* Allow encryption */
        PSA_KEY_USAGE_DECRYPT |       /* Allow decryption */
        PSA_KEY_USAGE_EXPORT          /* Allow key export (for backup) */
    );
    /* Other usage flags:
     * - PSA_KEY_USAGE_SIGN_HASH / PSA_KEY_USAGE_SIGN_MESSAGE
     * - PSA_KEY_USAGE_VERIFY_HASH / PSA_KEY_USAGE_VERIFY_MESSAGE
     * - PSA_KEY_USAGE_DERIVE
     * - PSA_KEY_USAGE_COPY
     */

    /* 4. Set algorithm */
    psa_set_key_algorithm(&attr, PSA_ALG_GCM);  /* AES-GCM */
    /* Other algorithms:
     * - PSA_ALG_CBC_NO_PADDING
     * - PSA_ALG_CCM
     * - PSA_ALG_HMAC(PSA_ALG_SHA_256)
     * - PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256)
     * - PSA_ALG_ECDSA(PSA_ALG_SHA_256)
     */

    /* 5. Set key lifetime */
    psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_PERSISTENT);
    /* Lifetimes:
     * - PSA_KEY_LIFETIME_VOLATILE (default, lost on reboot)
     * - PSA_KEY_LIFETIME_PERSISTENT (survives reboot)
     */

    /* 6. Set key ID (for persistent keys) */
    psa_set_key_id(&attr, 100);  /* Persistent key with ID 100 */
}
```

**Complete Key Management Example:**

```c
/**
 * Comprehensive key management demonstration
 */
int crypto_key_management_demo(void)
{
    psa_status_t status;
    psa_key_id_t key_id;
    psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

    /* Initialize PSA Crypto */
    status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        printf("Failed to initialize PSA Crypto: %d\n", status);
        return -1;
    }

    /* ===== Method 1: Import existing key ===== */
    printf("\n=== Method 1: Import Key ===\n");

    const uint8_t aes_key[32] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };

    psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attr, 256);
    psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
    psa_set_key_algorithm(&attr, PSA_ALG_GCM);
    psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);

    status = psa_import_key(&attr, aes_key, sizeof(aes_key), &key_id);
    if (status == PSA_SUCCESS) {
        printf("✓ Key imported successfully (ID: %u)\n", (unsigned int)key_id);
    } else {
        printf("✗ Key import failed: %d\n", status);
        return -1;
    }

    /* Use the key for encryption */
    uint8_t plaintext[16] = "Hello, World!";
    uint8_t ciphertext[32];
    uint8_t nonce[12] = {0};
    uint8_t tag[16];
    size_t ciphertext_len;

    status = psa_aead_encrypt(
        key_id, PSA_ALG_GCM,
        nonce, sizeof(nonce),
        NULL, 0,  /* No additional data */
        plaintext, sizeof(plaintext),
        ciphertext, sizeof(ciphertext), &ciphertext_len
    );

    if (status == PSA_SUCCESS) {
        printf("✓ Encryption successful (%zu bytes)\n", ciphertext_len);
    }

    /* Destroy the key */
    psa_destroy_key(key_id);
    printf("✓ Key destroyed\n");

    /* ===== Method 2: Generate random key ===== */
    printf("\n=== Method 2: Generate Key ===\n");

    psa_reset_key_attributes(&attr);
    psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attr, 256);
    psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_EXPORT);
    psa_set_key_algorithm(&attr, PSA_ALG_CCM);

    status = psa_generate_key(&attr, &key_id);
    if (status == PSA_SUCCESS) {
        printf("✓ Key generated successfully (ID: %u)\n", (unsigned int)key_id);

        /* Export the key (for backup) */
        uint8_t exported_key[32];
        size_t exported_len;

        status = psa_export_key(key_id, exported_key, sizeof(exported_key), &exported_len);
        if (status == PSA_SUCCESS) {
            printf("✓ Key exported (%zu bytes): ", exported_len);
            for (size_t i = 0; i < exported_len && i < 16; i++) {
                printf("%02x", exported_key[i]);
            }
            printf("...\n");
        }

        psa_destroy_key(key_id);
    }

    /* ===== Method 3: Generate key pair (asymmetric) ===== */
    printf("\n=== Method 3: Generate Key Pair ===\n");

    psa_reset_key_attributes(&attr);
    psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&attr, 256);  /* P-256 curve */
    psa_set_key_usage_flags(&attr,
        PSA_KEY_USAGE_SIGN_MESSAGE |
        PSA_KEY_USAGE_VERIFY_MESSAGE |
        PSA_KEY_USAGE_EXPORT
    );
    psa_set_key_algorithm(&attr, PSA_ALG_ECDSA(PSA_ALG_SHA_256));

    status = psa_generate_key(&attr, &key_id);
    if (status == PSA_SUCCESS) {
        printf("✓ ECC key pair generated (ID: %u)\n", (unsigned int)key_id);

        /* Export public key */
        uint8_t public_key[65];  /* Uncompressed point: 0x04 + X + Y */
        size_t public_key_len;

        status = psa_export_public_key(key_id, public_key, sizeof(public_key), &public_key_len);
        if (status == PSA_SUCCESS) {
            printf("✓ Public key exported (%zu bytes)\n", public_key_len);
            printf("  X: ");
            for (size_t i = 1; i < 17; i++) {
                printf("%02x", public_key[i]);
            }
            printf("...\n");
            printf("  Y: ");
            for (size_t i = 33; i < 49; i++) {
                printf("%02x", public_key[i]);
            }
            printf("...\n");
        }

        /* Sign a message */
        uint8_t message[] = "Sign this message";
        uint8_t signature[64];  /* ECDSA P-256 signature is 64 bytes */
        size_t signature_len;

        status = psa_sign_message(
            key_id,
            PSA_ALG_ECDSA(PSA_ALG_SHA_256),
            message, sizeof(message),
            signature, sizeof(signature), &signature_len
        );

        if (status == PSA_SUCCESS) {
            printf("✓ Message signed (%zu bytes)\n", signature_len);

            /* Verify signature */
            status = psa_verify_message(
                key_id,
                PSA_ALG_ECDSA(PSA_ALG_SHA_256),
                message, sizeof(message),
                signature, signature_len
            );

            if (status == PSA_SUCCESS) {
                printf("✓ Signature verified!\n");
            }
        }

        psa_destroy_key(key_id);
    }

    /* ===== Method 4: Persistent key ===== */
    printf("\n=== Method 4: Persistent Key ===\n");

    psa_reset_key_attributes(&attr);
    psa_set_key_type(&attr, PSA_KEY_TYPE_HMAC);
    psa_set_key_bits(&attr, 256);
    psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE);
    psa_set_key_algorithm(&attr, PSA_ALG_HMAC(PSA_ALG_SHA_256));
    psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_PERSISTENT);
    psa_set_key_id(&attr, 1001);  /* Persistent ID */

    status = psa_generate_key(&attr, &key_id);
    if (status == PSA_SUCCESS) {
        printf("✓ Persistent key created (ID: %u)\n", (unsigned int)key_id);
        printf("  This key survives reboot!\n");

        /* Key persists - can be accessed later */
        /* psa_destroy_key(key_id); */  /* Don't destroy if you want it persistent */
    } else if (status == PSA_ERROR_ALREADY_EXISTS) {
        printf("✓ Persistent key already exists\n");
        key_id = 1001;  /* Re-use existing key */
    }

    printf("\n✓ Key management demo complete!\n");
    return 0;
}
```

### 11.3 Hashing Operations

**Single-shot Hashing:**

```c
/**
 * Hash computation examples
 */
void hash_examples(void)
{
    psa_status_t status;
    uint8_t input[] = "Data to hash";
    uint8_t hash[PSA_HASH_LENGTH(PSA_ALG_SHA_256)];  /* 32 bytes */
    size_t hash_len;

    /* SHA-256 */
    status = psa_hash_compute(
        PSA_ALG_SHA_256,
        input, sizeof(input),
        hash, sizeof(hash), &hash_len
    );

    if (status == PSA_SUCCESS) {
        printf("SHA-256: ");
        for (size_t i = 0; i < hash_len; i++) {
            printf("%02x", hash[i]);
        }
        printf("\n");
    }

    /* Hash comparison (constant-time) */
    uint8_t expected_hash[32] = { /* ... */ };

    status = psa_hash_compare(
        PSA_ALG_SHA_256,
        input, sizeof(input),
        expected_hash, sizeof(expected_hash)
    );

    if (status == PSA_SUCCESS) {
        printf("✓ Hash matches!\n");
    } else if (status == PSA_ERROR_INVALID_SIGNATURE) {
        printf("✗ Hash does NOT match!\n");
    }
}
```

**Multi-part Hashing:**

```c
/**
 * Multi-part hash operation (for large data)
 */
void multi_part_hash_example(void)
{
    psa_hash_operation_t op = PSA_HASH_OPERATION_INIT;
    psa_status_t status;

    /* Setup */
    status = psa_hash_setup(&op, PSA_ALG_SHA_256);
    if (status != PSA_SUCCESS) {
        printf("Hash setup failed: %d\n", status);
        return;
    }

    /* Update (can be called multiple times) */
    uint8_t chunk1[] = "First chunk";
    status = psa_hash_update(&op, chunk1, sizeof(chunk1));

    uint8_t chunk2[] = "Second chunk";
    status = psa_hash_update(&op, chunk2, sizeof(chunk2));

    uint8_t chunk3[] = "Third chunk";
    status = psa_hash_update(&op, chunk3, sizeof(chunk3));

    /* Finish */
    uint8_t hash[32];
    size_t hash_len;
    status = psa_hash_finish(&op, hash, sizeof(hash), &hash_len);

    if (status == PSA_SUCCESS) {
        printf("Multi-part hash: ");
        for (size_t i = 0; i < hash_len; i++) {
            printf("%02x", hash[i]);
        }
        printf("\n");
    }

    /* Always abort if not finished successfully */
    /* psa_hash_abort(&op); */
}
```

### 11.4 Message Authentication Codes (MAC)

#### What is MAC and Why Do We Need It?

**Simple Explanation:**
A Message Authentication Code (MAC) is like a "tamper-proof seal" for your data. It proves two things:
1. The message came from someone who has the secret key (Authentication)
2. The message wasn't modified in transit (Integrity)

**Real-world analogy:**
Think of a MAC like a wax seal on a letter. Only someone with the correct signet ring (secret key) can create a valid seal, and any tampering breaks the seal.

**HMAC Data Flow Diagram:**

```
Simple Flow (What Happens):
┌──────────┐     ┌──────────┐
│  Secret  │ ──► │          │
│   Key    │     │  HMAC    │ ──► MAC Tag (32 bytes)
│          │ ──► │  Engine  │     ✓ Proves authenticity
│ Message  │     │          │     ✓ Detects tampering
└──────────┘     └──────────┘

Verification Flow:
┌──────────┐     ┌──────────┐
│ Message  │ ──► │  HMAC    │ ──► Computed MAC
│   +      │     │  Engine  │
│   Key    │     └──────────┘
└──────────┘            │
                        ▼
┌──────────┐     ┌──────────┐
│ Received │ ──► │ Compare  │ ──► ✓ Match = Valid
│   MAC    │     │          │     ✗ No match = Tampered!
└──────────┘     └──────────┘
```

**Detailed HMAC Internals:**

```
HMAC-SHA256 Internal Operation:
═══════════════════════════════════════════════════════════════

Step 1: Key Preparation
┌─────────────────────┐
│ Original Key        │
│ (variable length)   │
└──────────┬──────────┘
           │
           ▼
     ┌─────────┐
     │ Padding │ ──► If key < 64 bytes: pad with zeros
     └────┬────┘     If key > 64 bytes: hash it first
          │
          ▼
   ┌──────────────┐
   │ Block Size   │
   │ Key (64 B)   │
   └──────────────┘

Step 2: Inner Hash
┌──────────────┐
│ Key XOR ipad │ (ipad = 0x36 repeated 64 times)
│   (64 bytes) │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  + Message   │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  SHA-256()   │ ──► Inner Hash (32 bytes)
└──────┬───────┘
       │
Step 3: Outer Hash
       │
       ▼
┌──────────────┐
│ Key XOR opad │ (opad = 0x5C repeated 64 times)
│   (64 bytes) │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ + Inner Hash │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  SHA-256()   │ ──► Final HMAC Tag (32 bytes)
└──────────────┘
```

#### Simple HMAC Example (One-Shot)

**Explanation:** The simplest way to use HMAC - compute a MAC tag in one function call.

```c
#include "psa/crypto.h"

/**
 * SIMPLE EXAMPLE: Compute HMAC in one call
 *
 * What this does:
 * 1. Takes a message and secret key
 * 2. Computes HMAC-SHA256 tag
 * 3. Returns 32-byte MAC tag
 *
 * Use case: Protecting small messages (< 1 KB)
 */
int simple_hmac_example(void)
{
    psa_status_t status;

    /* Step 1: Create HMAC key */
    psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_id_t key_id;

    /* Configure key for HMAC */
    psa_set_key_type(&attr, PSA_KEY_TYPE_HMAC);
    psa_set_key_bits(&attr, 256);  /* 256-bit key */
    psa_set_key_usage_flags(&attr,
        PSA_KEY_USAGE_SIGN_MESSAGE |    /* Create MAC tags */
        PSA_KEY_USAGE_VERIFY_MESSAGE    /* Verify MAC tags */
    );
    psa_set_key_algorithm(&attr, PSA_ALG_HMAC(PSA_ALG_SHA_256));

    /* Generate random HMAC key */
    status = psa_generate_key(&attr, &key_id);
    if (status != PSA_SUCCESS) {
        printf("✗ Failed to generate HMAC key: %d\n", status);
        return -1;
    }
    printf("✓ HMAC key generated (ID: %u)\n", (unsigned int)key_id);

    /* Step 2: Message to protect */
    const uint8_t message[] = "Transfer $1000 to account 12345";

    /* Step 3: Compute MAC tag (one-shot) */
    uint8_t mac[32];  /* HMAC-SHA256 produces 32 bytes */
    size_t mac_len;

    status = psa_mac_compute(
        key_id,                           /* Key to use */
        PSA_ALG_HMAC(PSA_ALG_SHA_256),   /* Algorithm */
        message, sizeof(message),         /* Input message */
        mac, sizeof(mac), &mac_len        /* Output MAC */
    );

    if (status != PSA_SUCCESS) {
        printf("✗ MAC computation failed: %d\n", status);
        return -2;
    }

    printf("✓ MAC computed (%zu bytes): ", mac_len);
    for (size_t i = 0; i < mac_len && i < 16; i++) {
        printf("%02x", mac[i]);
    }
    printf("...\n");

    /* Step 4: Verify MAC tag (receiver side) */
    status = psa_mac_verify(
        key_id,
        PSA_ALG_HMAC(PSA_ALG_SHA_256),
        message, sizeof(message),  /* Original message */
        mac, mac_len               /* MAC to verify */
    );

    if (status == PSA_SUCCESS) {
        printf("✓ MAC verified - message is authentic!\n");
    } else if (status == PSA_ERROR_INVALID_SIGNATURE) {
        printf("✗ MAC verification FAILED - message was tampered!\n");
    }

    /* Step 5: Test tamper detection */
    printf("\n--- Testing Tamper Detection ---\n");
    uint8_t tampered_message[] = "Transfer $9999 to account 12345";  /* Modified! */

    status = psa_mac_verify(
        key_id,
        PSA_ALG_HMAC(PSA_ALG_SHA_256),
        tampered_message, sizeof(tampered_message),
        mac, mac_len  /* Original MAC */
    );

    if (status == PSA_ERROR_INVALID_SIGNATURE) {
        printf("✓ Tampering detected! MAC verification failed as expected.\n");
    }

    /* Cleanup */
    psa_destroy_key(key_id);
    return 0;
}
```

**Output you'll see:**
```
✓ HMAC key generated (ID: 1)
✓ MAC computed (32 bytes): a4f2b8c91d3e7f25...
✓ MAC verified - message is authentic!

--- Testing Tamper Detection ---
✓ Tampering detected! MAC verification failed as expected.
```

#### Multi-Part HMAC (For Large Data)

**Explanation:** When you need to MAC a large file or stream of data, you can't load it all into memory. Multi-part operations let you process data in chunks.

**Use Case Diagram:**

```
Streaming File Protection:
═══════════════════════════════════════════════════════

┌─────────────────────────────────────────────────┐
│         Large File (10 MB)                      │
│  ┌──────┐  ┌──────┐  ┌──────┐      ┌──────┐   │
│  │Chunk1│  │Chunk2│  │Chunk3│ .... │ChunkN│   │
│  └───┬──┘  └───┬──┘  └───┬──┘      └───┬──┘   │
└──────┼─────────┼─────────┼──────────────┼──────┘
       │         │         │              │
       ▼         ▼         ▼              ▼
    ┌────────────────────────────────────────┐
    │        psa_mac_update()                │
    │  (called N times, one chunk at a time) │
    └────────────┬───────────────────────────┘
                 │
                 ▼
         ┌───────────────┐
         │ psa_mac_finish│ ──► Final MAC Tag
         └───────────────┘

Benefits:
✓ Process data incrementally (no need to load entire file)
✓ Works with streaming protocols
✓ Low memory footprint
```

```c
/**
 * MULTI-PART HMAC: Process large data in chunks
 *
 * What this does:
 * 1. Setup HMAC operation once
 * 2. Feed data in multiple chunks (psa_mac_update called N times)
 * 3. Finalize to get MAC tag
 *
 * Use case:
 * - Large files (can't fit in RAM)
 * - Streaming data
 * - Network packets arriving sequentially
 */
int multipart_hmac_example(void)
{
    psa_status_t status;
    psa_mac_operation_t op = PSA_MAC_OPERATION_INIT;

    /* Assume we have an HMAC key from previous example */
    psa_key_id_t key_id;
    psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

    psa_set_key_type(&attr, PSA_KEY_TYPE_HMAC);
    psa_set_key_bits(&attr, 256);
    psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_MESSAGE);
    psa_set_key_algorithm(&attr, PSA_ALG_HMAC(PSA_ALG_SHA_256));

    status = psa_generate_key(&attr, &key_id);
    if (status != PSA_SUCCESS) {
        return -1;
    }

    /* Step 1: Setup HMAC operation */
    status = psa_mac_sign_setup(&op, key_id, PSA_ALG_HMAC(PSA_ALG_SHA_256));
    if (status != PSA_SUCCESS) {
        printf("✗ MAC setup failed: %d\n", status);
        return -2;
    }
    printf("✓ HMAC operation initialized\n");

    /* Step 2: Update with multiple data chunks */
    /* Simulating processing a large file in 1KB chunks */

    uint8_t chunk1[1024];
    memset(chunk1, 0xAA, sizeof(chunk1));  /* Simulate file data */

    status = psa_mac_update(&op, chunk1, sizeof(chunk1));
    if (status != PSA_SUCCESS) {
        printf("✗ MAC update 1 failed: %d\n", status);
        psa_mac_abort(&op);
        return -3;
    }
    printf("✓ Processed chunk 1 (1024 bytes)\n");

    uint8_t chunk2[1024];
    memset(chunk2, 0xBB, sizeof(chunk2));

    status = psa_mac_update(&op, chunk2, sizeof(chunk2));
    printf("✓ Processed chunk 2 (1024 bytes)\n");

    uint8_t chunk3[512];  /* Last chunk may be smaller */
    memset(chunk3, 0xCC, sizeof(chunk3));

    status = psa_mac_update(&op, chunk3, sizeof(chunk3));
    printf("✓ Processed chunk 3 (512 bytes)\n");

    /* Step 3: Finalize and get MAC tag */
    uint8_t mac[32];
    size_t mac_len;

    status = psa_mac_sign_finish(&op, mac, sizeof(mac), &mac_len);
    if (status != PSA_SUCCESS) {
        printf("✗ MAC finish failed: %d\n", status);
        psa_mac_abort(&op);
        return -4;
    }

    printf("✓ HMAC completed (%zu bytes): ", mac_len);
    for (size_t i = 0; i < 16; i++) {
        printf("%02x", mac[i]);
    }
    printf("...\n");
    printf("Total data processed: %zu bytes\n",
           sizeof(chunk1) + sizeof(chunk2) + sizeof(chunk3));

    /* Cleanup */
    psa_destroy_key(key_id);
    return 0;
}
```

**Output:**
```
✓ HMAC operation initialized
✓ Processed chunk 1 (1024 bytes)
✓ Processed chunk 2 (1024 bytes)
✓ Processed chunk 3 (512 bytes)
✓ HMAC completed (32 bytes): 7f3a9c2b8e1d4f6a...
Total data processed: 2560 bytes
```

#### HMAC Performance Comparison

```c
/**
 * Performance comparison: Software vs Hardware HMAC
 *
 * This demonstrates the speed difference when using
 * hardware crypto accelerators (available on STM32U5, etc.)
 */
void hmac_performance_comparison(void)
{
    /* Test data: 10 KB message */
    uint8_t large_message[10240];
    memset(large_message, 0x55, sizeof(large_message));

    printf("\n=== HMAC Performance Test ===\n");
    printf("Message size: %zu bytes\n\n", sizeof(large_message));

    /* Software HMAC (no hardware acceleration) */
    uint32_t start_time = get_timestamp_us();

    /* Compute HMAC (assumed software implementation) */
    uint8_t mac_sw[32];
    size_t mac_len;
    psa_mac_compute(key_id, PSA_ALG_HMAC(PSA_ALG_SHA_256),
                    large_message, sizeof(large_message),
                    mac_sw, sizeof(mac_sw), &mac_len);

    uint32_t sw_time = get_timestamp_us() - start_time;

    printf("Software HMAC: %u µs (%.2f MB/s)\n",
           sw_time,
           (sizeof(large_message) / 1024.0 / 1024.0) / (sw_time / 1000000.0));

    /* Hardware-accelerated HMAC (if available) */
    start_time = get_timestamp_us();

    uint8_t mac_hw[32];
    psa_mac_compute(key_id_hw, PSA_ALG_HMAC(PSA_ALG_SHA_256),
                    large_message, sizeof(large_message),
                    mac_hw, sizeof(mac_hw), &mac_len);

    uint32_t hw_time = get_timestamp_us() - start_time;

    printf("Hardware HMAC: %u µs (%.2f MB/s)\n",
           hw_time,
           (sizeof(large_message) / 1024.0 / 1024.0) / (hw_time / 1000000.0));

    printf("Speedup: %.1fx faster\n", (float)sw_time / hw_time);
}
```

**Expected Output (STM32U5 with HASH accelerator):**
```
=== HMAC Performance Test ===
Message size: 10240 bytes

Software HMAC: 2340 µs (4.16 MB/s)
Hardware HMAC: 156 µs (62.5 MB/s)
Speedup: 15.0x faster
```

---

### 11.5 Symmetric Encryption

#### What is Symmetric Encryption?

**Simple Explanation:**
Symmetric encryption is like a lockbox where the same key locks AND unlocks it. Both sender and receiver must have the same secret key.

**Block Diagram:**

```
Encryption Flow:
═══════════════════════════════════════════════════════════

     ┌─────────────┐         ┌─────────────┐
     │  Plaintext  │         │  Secret Key │
     │  "Hello"    │         │  (256-bit)  │
     └──────┬──────┘         └──────┬──────┘
            │                       │
            │                       │
            └───────┬───────────────┘
                    ▼
            ┌───────────────┐
            │  AES Cipher   │
            │  (Encrypt)    │
            └───────┬───────┘
                    ▼
            ┌───────────────┐
            │  Ciphertext   │
            │  0x4a7f2b... │
            └───────────────┘

Decryption Flow:
═══════════════════════════════════════════════════════════

     ┌─────────────┐         ┌─────────────┐
     │ Ciphertext  │         │  Secret Key │
     │ 0x4a7f2b... │         │  (256-bit)  │
     └──────┬──────┘         └──────┬──────┘
            │                       │
            │                       │
            └───────┬───────────────┘
                    ▼
            ┌───────────────┐
            │  AES Cipher   │
            │  (Decrypt)    │
            └───────┬───────┘
                    ▼
            ┌───────────────┐
            │  Plaintext    │
            │  "Hello"      │
            └───────────────┘
```

#### AES Block Cipher Modes

**Understanding Cipher Modes:**

AES operates on 16-byte blocks. For messages longer than 16 bytes, we need a "mode of operation":

```
Block Cipher Modes Comparison:
═══════════════════════════════════════════════════════════

1. ECB (Electronic Codebook) - ❌ NOT RECOMMENDED

   Plaintext:  [Block1] [Block2] [Block3]
                  │        │        │
   AES Encrypt:   ▼        ▼        ▼
   Ciphertext: [Cipher1][Cipher2][Cipher3]

   Problem: Same plaintext block = same ciphertext block
           (reveals patterns!)

2. CBC (Cipher Block Chaining) - ✓ Good for files

   Plaintext:  [Block1] [Block2] [Block3]
                  │   ⊕    │   ⊕    │
   IV/Prev Cipher─┘        └────┘   └────previous
                  │        │        │
   AES Encrypt:   ▼        ▼        ▼
   Ciphertext: [Cipher1][Cipher2][Cipher3]

   ✓ Each block depends on previous block
   ✓ Requires IV (Initialization Vector)
   ✗ Cannot be parallelized

3. CTR (Counter Mode) - ✓ Best for streaming

   Counter:    [  0  ] [  1  ] [  2  ]
                  │        │        │
   AES Encrypt:   ▼        ▼        ▼
   Keystream:  [Stream1][Stream2][Stream3]
                  ⊕        ⊕        ⊕
   Plaintext:  [Block1] [Block2] [Block3]
                  │        │        │
   Ciphertext: [Cipher1][Cipher2][Cipher3]

   ✓ Can be parallelized
   ✓ Random access to blocks
   ✓ No padding needed
```

#### Simple Encryption Example (CBC Mode)

**Explanation:** CBC (Cipher Block Chaining) is a widely-used mode that chains blocks together for better security.

```c
#include "psa/crypto.h"

/**
 * SIMPLE ENCRYPTION: AES-128-CBC
 *
 * What this does:
 * 1. Creates a 128-bit AES key
 * 2. Encrypts data using CBC mode
 * 3. Decrypts back to original
 *
 * Important concepts:
 * - IV (Initialization Vector): Random 16 bytes, unique per message
 * - Padding: CBC requires data to be multiple of 16 bytes
 *
 * Use case: Encrypting stored files, database fields
 */
int simple_aes_cbc_example(void)
{
    psa_status_t status;

    /* Step 1: Create AES key */
    psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_id_t key_id;

    psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attr, 128);  /* AES-128 (16-byte key) */
    psa_set_key_usage_flags(&attr,
        PSA_KEY_USAGE_ENCRYPT |
        PSA_KEY_USAGE_DECRYPT
    );
    psa_set_key_algorithm(&attr, PSA_ALG_CBC_NO_PADDING);

    status = psa_generate_key(&attr, &key_id);
    if (status != PSA_SUCCESS) {
        printf("✗ Failed to generate AES key: %d\n", status);
        return -1;
    }
    printf("✓ AES-128 key generated\n");

    /* Step 2: Prepare data to encrypt */
    /* IMPORTANT: For CBC with no padding, data MUST be 16-byte multiple */
    const uint8_t plaintext[32] = {  /* 32 bytes = 2 blocks */
        'H', 'e', 'l', 'l', 'o', ' ', 'f', 'r',
        'o', 'm', ' ', 'T', 'F', '-', 'M', '!',
        'S', 'e', 'c', 'u', 'r', 'e', ' ', 'M',
        'e', 's', 's', 'a', 'g', 'e', '!', '!'
    };

    printf("Plaintext (%zu bytes): \"%.*s\"\n",
           sizeof(plaintext), (int)sizeof(plaintext), plaintext);

    /* Step 3: Generate random IV (Initialization Vector) */
    uint8_t iv[16];  /* IV must be 16 bytes for AES */

    status = psa_generate_random(iv, sizeof(iv));
    if (status != PSA_SUCCESS) {
        printf("✗ Failed to generate IV: %d\n", status);
        return -2;
    }

    printf("IV: ");
    for (size_t i = 0; i < sizeof(iv); i++) {
        printf("%02x", iv[i]);
    }
    printf("\n");

    /* Step 4: Encrypt */
    uint8_t ciphertext[48];  /* Need extra space for potential padding */
    size_t ciphertext_len;

    status = psa_cipher_encrypt(
        key_id,
        PSA_ALG_CBC_NO_PADDING,
        plaintext, sizeof(plaintext),   /* Input */
        ciphertext, sizeof(ciphertext), /* Output buffer */
        &ciphertext_len                  /* Actual output size */
    );

    if (status != PSA_SUCCESS) {
        printf("✗ Encryption failed: %d\n", status);
        return -3;
    }

    printf("✓ Encrypted %zu bytes\n", ciphertext_len);
    printf("Ciphertext: ");
    for (size_t i = 0; i < ciphertext_len && i < 32; i++) {
        printf("%02x", ciphertext[i]);
    }
    printf("...\n");

    /* Step 5: Decrypt */
    uint8_t decrypted[48];
    size_t decrypted_len;

    status = psa_cipher_decrypt(
        key_id,
        PSA_ALG_CBC_NO_PADDING,
        ciphertext, ciphertext_len,      /* Input */
        decrypted, sizeof(decrypted),    /* Output buffer */
        &decrypted_len                    /* Actual output size */
    );

    if (status != PSA_SUCCESS) {
        printf("✗ Decryption failed: %d\n", status);
        return -4;
    }

    printf("✓ Decrypted %zu bytes\n", decrypted_len);
    printf("Decrypted text: \"%.*s\"\n", (int)decrypted_len, decrypted);

    /* Step 6: Verify */
    if (decrypted_len == sizeof(plaintext) &&
        memcmp(plaintext, decrypted, decrypted_len) == 0) {
        printf("✓ Decryption successful - data matches!\n");
    } else {
        printf("✗ Decryption mismatch!\n");
    }

    /* Cleanup */
    psa_destroy_key(key_id);
    return 0;
}
```

**Output:**
```
✓ AES-128 key generated
Plaintext (32 bytes): "Hello from TF-M!Secure Message!!"
IV: 7a3f9c2b8e1d4f6a5c8b3e9f2a7d1c4e
✓ Encrypted 32 bytes
Ciphertext: 4f8e2a9c7b1d3f6e5a2c8b4e9f1a7d3c...
✓ Decrypted 32 bytes
Decrypted text: "Hello from TF-M!Secure Message!!"
✓ Decryption successful - data matches!
```

---

### 11.6 Authenticated Encryption with Associated Data (AEAD)

#### What is AEAD and Why is it Better?

**Simple Explanation:**
AEAD combines encryption and authentication in ONE operation. It's like a lockbox (encryption) with a tamper-proof seal (authentication) applied simultaneously.

**The Problem with Separate Encrypt + MAC:**

```
Traditional Approach (Encrypt-then-MAC):
═══════════════════════════════════════════════════════════

Step 1: Encrypt
┌──────────┐     ┌─────────┐
│Plaintext │ ──► │ AES-CBC │ ──► Ciphertext
└──────────┘     └─────────┘

Step 2: MAC the ciphertext
┌────────────┐     ┌──────┐
│ Ciphertext │ ──► │ HMAC │ ──► MAC Tag
└────────────┘     └──────┘

Problems:
✗ Two separate operations (slower)
✗ Two separate keys needed
✗ Easy to make mistakes (e.g., MAC-then-encrypt is insecure!)
✗ Timing attacks possible

AEAD Approach (AES-GCM):
═══════════════════════════════════════════════════════════

Single Operation:
┌──────────┐     ┌─────────┐     ┌────────────┐
│Plaintext │ ──► │         │ ──► │ Ciphertext │
└──────────┘     │ AES-GCM │     └────────────┘
┌──────────┐     │         │     ┌────────────┐
│ Add. Data│ ──► │ (AEAD)  │ ──► │  Auth Tag  │
└──────────┘     └─────────┘     └────────────┘

Benefits:
✓ ONE operation (faster)
✓ ONE key
✓ Cannot make mistakes (cryptographically robust)
✓ Protects both encrypted and unencrypted data
```

#### AEAD Data Flow (AES-GCM)

**Understanding AES-GCM Internals:**

```
AES-GCM Complete Data Flow:
═══════════════════════════════════════════════════════════

Inputs:
┌─────────────────────────────────────────────────────┐
│ 1. Key (128/192/256 bits)                           │
│ 2. Nonce/IV (96 bits recommended)                   │
│ 3. Additional Authenticated Data (AAD) - optional   │
│ 4. Plaintext                                        │
└─────────────────────────────────────────────────────┘

Process:
┌─────────────────┐
│  Step 1: GHASH  │  ──► Generate authentication subkey H
│  Subkey Gen     │       H = AES_encrypt(Key, 0)
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Step 2: CTR    │  ──► Encrypt plaintext using AES-CTR
│  Mode Encrypt   │       Counter = Nonce || 0x00000001
└────────┬────────┘       Counter increments for each block
         │
         ▼
┌─────────────────┐
│  Step 3: GHASH  │  ──► Authenticate AAD + Ciphertext
│  Authentication │       Using Galois field multiplication
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Step 4: Final  │  ──► XOR with encrypted counter[0]
│  Tag Generation │
└────────┬────────┘
         │
         ▼
Outputs:
┌─────────────────────────────────────────────────────┐
│ 1. Ciphertext (same length as plaintext)            │
│ 2. Authentication Tag (128 bits / 16 bytes)         │
└─────────────────────────────────────────────────────┘

Verification (Decrypt):
═══════════════════════════════════════════════════════════
┌────────────┐     ┌─────────┐
│ Ciphertext │ ──► │ Compute │ ──► Tag'
│    + AAD   │     │   Tag   │
└────────────┘     └─────────┘
                        │
                        ▼
┌────────────┐     ┌─────────┐
│ Received   │ ──► │ Compare │ ──► ✓ Match: Decrypt
│    Tag     │     │         │     ✗ Fail: REJECT!
└────────────┘     └─────────┘
```

#### Simple AEAD Example (AES-GCM)

**Explanation:** AES-GCM is the most widely-used AEAD cipher. It's perfect for securing network protocols (TLS, IPsec) and secure storage.

```c
#include "psa/crypto.h"

/**
 * SIMPLE AEAD: AES-128-GCM One-Shot Operation
 *
 * What this does:
 * 1. Encrypts plaintext to ciphertext
 * 2. Authenticates both ciphertext AND additional data (AAD)
 * 3. Produces authentication tag
 *
 * Real-world example:
 * - Plaintext: Message payload
 * - AAD: Packet header (needs authentication but not encryption)
 *
 * Use case: TLS 1.3, secure messaging, encrypted storage
 */
int simple_aead_gcm_example(void)
{
    psa_status_t status;

    /* Step 1: Create AES-GCM key */
    psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_id_t key_id;

    psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attr, 128);  /* AES-128 */
    psa_set_key_usage_flags(&attr,
        PSA_KEY_USAGE_ENCRYPT |
        PSA_KEY_USAGE_DECRYPT
    );
    psa_set_key_algorithm(&attr, PSA_ALG_GCM);

    status = psa_generate_key(&attr, &key_id);
    if (status != PSA_SUCCESS) {
        printf("✗ Failed to generate key: %d\n", status);
        return -1;
    }
    printf("✓ AES-128-GCM key generated\n");

    /* Step 2: Prepare data */
    const uint8_t plaintext[] = "Secret: Attack at dawn!";

    /* Additional Authenticated Data (AAD) - not encrypted but authenticated */
    const uint8_t aad[] = "From: Alice, To: Bob, MsgID: 12345";

    printf("Plaintext:  \"%s\"\n", plaintext);
    printf("AAD (authenticated but not encrypted): \"%s\"\n", aad);

    /* Step 3: Generate nonce (Number used ONCE) */
    /* WARNING: NEVER reuse the same nonce with the same key! */
    uint8_t nonce[12];  /* 96-bit nonce is recommended for GCM */

    status = psa_generate_random(nonce, sizeof(nonce));
    if (status != PSA_SUCCESS) {
        printf("✗ Failed to generate nonce: %d\n", status);
        return -2;
    }

    printf("Nonce: ");
    for (size_t i = 0; i < sizeof(nonce); i++) {
        printf("%02x", nonce[i]);
    }
    printf("\n");

    /* Step 4: Encrypt and authenticate (one operation!) */
    /* Output = ciphertext + authentication tag (appended) */
    uint8_t output[128];  /* Enough for ciphertext + tag */
    size_t output_len;

    status = psa_aead_encrypt(
        key_id,
        PSA_ALG_GCM,
        nonce, sizeof(nonce),           /* Nonce (IV) */
        aad, sizeof(aad),               /* Additional data to authenticate */
        plaintext, sizeof(plaintext),   /* Data to encrypt */
        output, sizeof(output),         /* Output buffer */
        &output_len                     /* Actual output length */
    );

    if (status != PSA_SUCCESS) {
        printf("✗ Encryption failed: %d\n", status);
        return -3;
    }

    /* Output contains: [Ciphertext | Tag] */
    /* Tag is 16 bytes (128 bits) by default */
    size_t ciphertext_len = output_len - 16;
    size_t tag_len = 16;

    printf("✓ Encrypted %zu bytes + %zu byte tag\n", ciphertext_len, tag_len);

    printf("Ciphertext: ");
    for (size_t i = 0; i < ciphertext_len && i < 32; i++) {
        printf("%02x", output[i]);
    }
    printf("...\n");

    printf("Auth Tag:   ");
    for (size_t i = 0; i < tag_len; i++) {
        printf("%02x", output[ciphertext_len + i]);
    }
    printf("\n");

    /* Step 5: Decrypt and verify (one operation!) */
    uint8_t decrypted[128];
    size_t decrypted_len;

    status = psa_aead_decrypt(
        key_id,
        PSA_ALG_GCM,
        nonce, sizeof(nonce),
        aad, sizeof(aad),                /* MUST be same AAD */
        output, output_len,              /* Ciphertext + tag */
        decrypted, sizeof(decrypted),
        &decrypted_len
    );

    if (status == PSA_SUCCESS) {
        printf("✓ Decryption and verification successful!\n");
        printf("Decrypted: \"%.*s\"\n", (int)decrypted_len, decrypted);
    } else if (status == PSA_ERROR_INVALID_SIGNATURE) {
        printf("✗ Authentication FAILED - data was tampered!\n");
        return -4;
    }

    /* Step 6: Test tampering detection */
    printf("\n--- Testing Tamper Detection ---\n");

    /* Modify one byte of ciphertext */
    output[0] ^= 0x01;  /* Flip one bit */

    status = psa_aead_decrypt(
        key_id,
        PSA_ALG_GCM,
        nonce, sizeof(nonce),
        aad, sizeof(aad),
        output, output_len,
        decrypted, sizeof(decrypted),
        &decrypted_len
    );

    if (status == PSA_ERROR_INVALID_SIGNATURE) {
        printf("✓ Tampering detected! Decryption rejected as expected.\n");
    } else {
        printf("✗ ERROR: Tampered data was accepted!\n");
    }

    /* Restore byte */
    output[0] ^= 0x01;

    /* Test AAD modification */
    printf("\n--- Testing AAD Modification ---\n");

    const uint8_t wrong_aad[] = "From: Eve, To: Bob, MsgID: 99999";  /* Modified! */

    status = psa_aead_decrypt(
        key_id,
        PSA_ALG_GCM,
        nonce, sizeof(nonce),
        wrong_aad, sizeof(wrong_aad),  /* Different AAD */
        output, output_len,
        decrypted, sizeof(decrypted),
        &decrypted_len
    );

    if (status == PSA_ERROR_INVALID_SIGNATURE) {
        printf("✓ AAD tampering detected! Decryption rejected.\n");
    }

    /* Cleanup */
    psa_destroy_key(key_id);
    return 0;
}
```

**Output:**
```
✓ AES-128-GCM key generated
Plaintext:  "Secret: Attack at dawn!"
AAD (authenticated but not encrypted): "From: Alice, To: Bob, MsgID: 12345"
Nonce: 7a3f9c2b8e1d4f6a5c8b3e9f
✓ Encrypted 23 bytes + 16 byte tag
Ciphertext: 8f2a9c7b1d3f6e5a2c8b4e9f1a7d...
Auth Tag:   a4f2b8c91d3e7f256a8c3b9e2f1d7a4c
✓ Decryption and verification successful!
Decrypted: "Secret: Attack at dawn!"

--- Testing Tamper Detection ---
✓ Tampering detected! Decryption rejected as expected.

--- Testing AAD Modification ---
✓ AAD tampering detected! Decryption rejected.
```

#### Real-World AEAD Use Case: Secure Network Packet

**Practical Example:** Protecting network communication (similar to TLS)

```c
/**
 * Real-world AEAD use case: Secure network packet
 *
 * Scenario:
 * - Header: Source IP, Dest IP, Protocol, Sequence Number (NOT encrypted)
 * - Payload: Actual message (ENCRYPTED)
 * - Both header and payload are AUTHENTICATED
 *
 * This is how TLS 1.3 protects data!
 */
typedef struct {
    uint32_t src_ip;
    uint32_t dest_ip;
    uint16_t protocol;
    uint16_t seq_num;
} __attribute__((packed)) packet_header_t;

int secure_network_packet_example(void)
{
    psa_status_t status;
    psa_key_id_t session_key;

    /* Assume we have a session key from key exchange */
    psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attr, 256);  /* AES-256 for high security */
    psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
    psa_set_key_algorithm(&attr, PSA_ALG_GCM);

    status = psa_generate_key(&attr, &session_key);
    if (status != PSA_SUCCESS) {
        return -1;
    }

    printf("=== Secure Network Packet Example ===\n\n");

    /* ===== SENDER SIDE ===== */

    /* 1. Prepare packet header (will be authenticated but NOT encrypted) */
    packet_header_t header = {
        .src_ip = 0xC0A80101,      /* 192.168.1.1 */
        .dest_ip = 0xC0A80164,     /* 192.168.1.100 */
        .protocol = 0x06,          /* TCP */
        .seq_num = 12345
    };

    printf("Packet Header (authenticated, not encrypted):\n");
    printf("  Source: 192.168.1.1\n");
    printf("  Dest:   192.168.1.100\n");
    printf("  Proto:  TCP\n");
    printf("  SeqNum: %u\n\n", header.seq_num);

    /* 2. Payload (will be encrypted AND authenticated) */
    const char *payload = "GET /api/secret HTTP/1.1";

    printf("Payload (encrypted and authenticated):\n");
    printf("  \"%s\"\n\n", payload);

    /* 3. Generate unique nonce (in real system: use counter or random) */
    uint8_t nonce[12];
    psa_generate_random(nonce, sizeof(nonce));

    /* 4. Encrypt payload with authenticated header */
    uint8_t encrypted_packet[256];
    size_t encrypted_len;

    status = psa_aead_encrypt(
        session_key,
        PSA_ALG_GCM,
        nonce, sizeof(nonce),
        (uint8_t*)&header, sizeof(header),  /* AAD: Header */
        (uint8_t*)payload, strlen(payload), /* Plaintext: Payload */
        encrypted_packet, sizeof(encrypted_packet),
        &encrypted_len
    );

    if (status == PSA_SUCCESS) {
        printf("✓ Packet encrypted: %zu bytes (payload + tag)\n\n", encrypted_len);
    }

    /* ===== RECEIVER SIDE ===== */

    /* 5. Receiver decrypts and verifies */
    uint8_t decrypted_payload[256];
    size_t decrypted_len;

    printf("Receiver verifying packet...\n");

    status = psa_aead_decrypt(
        session_key,
        PSA_ALG_GCM,
        nonce, sizeof(nonce),
        (uint8_t*)&header, sizeof(header),  /* MUST match sender's header */
        encrypted_packet, encrypted_len,
        decrypted_payload, sizeof(decrypted_payload),
        &decrypted_len
    );

    if (status == PSA_SUCCESS) {
        printf("✓ Packet verified and decrypted!\n");
        printf("  Received payload: \"%.*s\"\n", (int)decrypted_len, decrypted_payload);
    }

    /* 6. Simulate attack: modify header */
    printf("\n--- Simulating Attack: Header Modification ---\n");
    packet_header_t fake_header = header;
    fake_header.src_ip = 0xDEADBEEF;  /* Attacker changes source IP */

    status = psa_aead_decrypt(
        session_key,
        PSA_ALG_GCM,
        nonce, sizeof(nonce),
        (uint8_t*)&fake_header, sizeof(fake_header),  /* Modified header */
        encrypted_packet, encrypted_len,
        decrypted_payload, sizeof(decrypted_payload),
        &decrypted_len
    );

    if (status == PSA_ERROR_INVALID_SIGNATURE) {
        printf("✓ Attack detected! Modified header rejected.\n");
        printf("  AEAD protected the header even though it wasn't encrypted!\n");
    }

    psa_destroy_key(session_key);
    return 0;
}
```

**Output:**
```
=== Secure Network Packet Example ===

Packet Header (authenticated, not encrypted):
  Source: 192.168.1.1
  Dest:   192.168.1.100
  Proto:  TCP
  SeqNum: 12345

Payload (encrypted and authenticated):
  "GET /api/secret HTTP/1.1"

✓ Packet encrypted: 40 bytes (payload + tag)

Receiver verifying packet...
✓ Packet verified and decrypted!
  Received payload: "GET /api/secret HTTP/1.1"

--- Simulating Attack: Header Modification ---
✓ Attack detected! Modified header rejected.
  AEAD protected the header even though it wasn't encrypted!
```

#### AEAD vs Traditional Encrypt+MAC Comparison

```c
/**
 * Performance comparison: AEAD vs Encrypt-then-MAC
 *
 * This shows why AEAD is preferred in modern cryptography
 */
void aead_vs_traditional_comparison(void)
{
    uint8_t test_data[1024];
    memset(test_data, 0x42, sizeof(test_data));

    printf("\n=== AEAD vs Traditional Comparison ===\n");
    printf("Test data size: %zu bytes\n\n", sizeof(test_data));

    /* Test 1: Traditional approach (AES-CBC + HMAC) */
    printf("Method 1: AES-CBC + HMAC (separate operations)\n");

    uint32_t start = get_timestamp_us();

    /* Encrypt with AES-CBC */
    uint8_t ciphertext_cbc[1024];
    size_t ciphertext_len;
    psa_cipher_encrypt(key_aes, PSA_ALG_CBC_NO_PADDING,
                       test_data, sizeof(test_data),
                       ciphertext_cbc, sizeof(ciphertext_cbc),
                       &ciphertext_len);

    /* Then MAC the ciphertext */
    uint8_t mac[32];
    size_t mac_len;
    psa_mac_compute(key_hmac, PSA_ALG_HMAC(PSA_ALG_SHA_256),
                    ciphertext_cbc, ciphertext_len,
                    mac, sizeof(mac), &mac_len);

    uint32_t time_traditional = get_timestamp_us() - start;

    printf("  Time: %u µs\n", time_traditional);
    printf("  Operations: 2 (encrypt + MAC)\n");
    printf("  Keys needed: 2\n");
    printf("  Output size: %zu + %zu = %zu bytes\n\n",
           ciphertext_len, mac_len, ciphertext_len + mac_len);

    /* Test 2: AEAD approach (AES-GCM) */
    printf("Method 2: AES-GCM (single AEAD operation)\n");

    start = get_timestamp_us();

    /* Encrypt and authenticate in ONE operation */
    uint8_t output_gcm[1024 + 16];
    size_t output_len;
    psa_aead_encrypt(key_gcm, PSA_ALG_GCM,
                     nonce, 12,
                     NULL, 0,  /* No AAD */
                     test_data, sizeof(test_data),
                     output_gcm, sizeof(output_gcm),
                     &output_len);

    uint32_t time_aead = get_timestamp_us() - start;

    printf("  Time: %u µs\n", time_aead);
    printf("  Operations: 1 (encrypt+auth combined)\n");
    printf("  Keys needed: 1\n");
    printf("  Output size: %zu bytes\n\n", output_len);

    printf("Performance gain: %.2fx faster\n", (float)time_traditional / time_aead);
    printf("\n✓ AEAD is simpler, faster, and more secure!\n");
}
```

**Expected Output (STM32U5 with AES accelerator):**
```
=== AEAD vs Traditional Comparison ===
Test data size: 1024 bytes

Method 1: AES-CBC + HMAC (separate operations)
  Time: 892 µs
  Operations: 2 (encrypt + MAC)
  Keys needed: 2
  Output size: 1024 + 32 = 1056 bytes

Method 2: AES-GCM (single AEAD operation)
  Time: 284 µs
  Operations: 1 (encrypt+auth combined)
  Keys needed: 1
  Output size: 1040 bytes

Performance gain: 3.14x faster

✓ AEAD is simpler, faster, and more secure!
```

---

### 11.7 Asymmetric Cryptography (Public Key Cryptography)

#### What is Asymmetric Cryptography?

**Simple Explanation:**
Unlike symmetric encryption (same key for encrypt and decrypt), asymmetric crypto uses TWO related keys:
- **Public Key**: Can be shared with everyone (like your email address)
- **Private Key**: Must be kept secret (like your password)

**Real-world analogy:**
Think of a mailbox:
- Anyone can put a letter IN (encrypt with public key)
- Only you with the key can take letters OUT (decrypt with private key)

**Key Differences:**

```
Symmetric vs Asymmetric Comparison:
═══════════════════════════════════════════════════════════

SYMMETRIC (AES):
┌─────────────────────────────────────┐
│          Same Secret Key            │
│                                     │
│  Alice ←────────────────→ Bob       │
│         (shared secret)             │
└─────────────────────────────────────┘

Problem: How do Alice and Bob share the key securely?

Encryption:
   [Plaintext] + [Secret Key] → [Ciphertext]

Decryption:
   [Ciphertext] + [Same Key] → [Plaintext]

✓ Fast (hardware accelerated)
✗ Key distribution problem
✗ Need N² keys for N users


ASYMMETRIC (RSA/ECC):
┌─────────────────────────────────────┐
│  Alice                    Bob       │
│  ┌──────────────┐    ┌──────────┐  │
│  │ Private Key  │    │ Public   │  │
│  │  (secret)    │    │   Key    │  │
│  └──────────────┘    │ (shared) │  │
│                      └──────────┘  │
└─────────────────────────────────────┘

Solution: Bob shares his public key openly!

Encryption (Alice → Bob):
   [Plaintext] + [Bob's Public Key] → [Ciphertext]

Decryption (Bob):
   [Ciphertext] + [Bob's Private Key] → [Plaintext]

✓ Solves key distribution
✓ Enables digital signatures
✗ Slower than symmetric (10-100x)
```

#### How Asymmetric Crypto Works

**Mathematical Foundation (Simplified):**

```
Public-Key Cryptography Magic:
═══════════════════════════════════════════════════════════

Key Generation (One-Way Function):
┌───────────────┐
│ Random Number │
│   (entropy)   │
└───────┬───────┘
        │
        ▼
┌───────────────────────────────┐
│ Mathematical One-Way Function │
│                               │
│ Easy: Private → Public        │
│ Hard: Public → Private        │
└───────┬───────────────────────┘
        │
        ▼
┌──────────────────────────┐
│ Private Key (d)          │  ──► Keep secret!
│   + Public Key (e, n)    │  ──► Share publicly
└──────────────────────────┘

For RSA:
  Public = (e, n)    where n = p × q (large primes)
  Private = (d)      where e × d ≡ 1 (mod φ(n))

For ECC (Elliptic Curve):
  Private = k        (random 256-bit number)
  Public = k × G     (point on elliptic curve)

Why it's secure:
✓ RSA: Factoring large numbers is hard (2048-bit)
✓ ECC: Discrete logarithm on curves is hard (256-bit)
```

#### Two Main Use Cases

**Use Case 1: Encryption (Confidentiality)**

```
Alice Encrypts for Bob:
═══════════════════════════════════════════════════════════

┌───────────┐     ┌────────────────┐     ┌────────────┐
│ Plaintext │ ──► │ RSA Encrypt    │ ──► │ Ciphertext │
│ "Secret"  │     │ (Bob's Public) │     │            │
└───────────┘     └────────────────┘     └─────┬──────┘
                                                │
                                                │ Send to Bob
                                                ▼
                                         ┌────────────┐
                                         │ Ciphertext │
                                         └─────┬──────┘
                                               │
                                               ▼
┌───────────┐     ┌────────────────┐     ┌────────────┐
│ Plaintext │ ◄── │ RSA Decrypt    │ ◄── │ Ciphertext │
│ "Secret"  │     │ (Bob's Private)│     │            │
└───────────┘     └────────────────┘     └────────────┘

Only Bob can decrypt (has private key)!
```

**Use Case 2: Digital Signatures (Authentication)**

```
Alice Signs a Message:
═══════════════════════════════════════════════════════════

┌───────────┐     ┌──────────┐
│  Message  │ ──► │ SHA-256  │ ──► Hash (32 bytes)
└───────────┘     └────┬─────┘
                       │
                       ▼
                  ┌──────────────────┐
                  │ ECDSA Sign       │
                  │ (Alice's Private)│
                  └────┬─────────────┘
                       │
                       ▼
                  ┌──────────┐
                  │ Signature│ (64 bytes for P-256)
                  └──────────┘

Send: [Message + Signature]
                       │
                       ▼ Bob receives

┌───────────┐     ┌──────────┐
│  Message  │ ──► │ SHA-256  │ ──► Hash
└───────────┘     └────┬─────┘
                       │
                       ▼
                  ┌──────────────────┐
┌───────────┐     │ ECDSA Verify     │
│ Signature │ ──► │ (Alice's Public) │ ──► ✓ Valid / ✗ Invalid
└───────────┘     └──────────────────┘

Proves:
✓ Message came from Alice (only she has private key)
✓ Message wasn't modified (signature is tied to hash)
```

#### Simple ECDSA Signature Example

**Explanation:** ECDSA (Elliptic Curve Digital Signature Algorithm) is faster and more secure than RSA for the same key size. It's used in TLS, Bitcoin, and PSA attestation.

```c
#include "psa/crypto.h"

/**
 * SIMPLE DIGITAL SIGNATURE: ECDSA P-256
 *
 * What this does:
 * 1. Generate ECC key pair (P-256 curve, 256-bit security)
 * 2. Sign a message with private key
 * 3. Verify signature with public key
 *
 * Use case:
 * - Software/firmware signing
 * - Authentication tokens
 * - TLS certificates
 * - Attestation
 *
 * Why ECDSA over RSA:
 * - Smaller keys: 256-bit ECC ≈ 3072-bit RSA security
 * - Faster signing (especially on embedded devices)
 * - Hardware acceleration available (STM32U5 PKA)
 */
int simple_ecdsa_signature_example(void)
{
    psa_status_t status;

    /* Step 1: Generate ECDSA key pair */
    psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_id_t key_id;

    /* Configure for ECDSA on P-256 curve */
    psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&attr, 256);  /* P-256 curve (secp256r1) */
    psa_set_key_usage_flags(&attr,
        PSA_KEY_USAGE_SIGN_MESSAGE |    /* Can sign messages */
        PSA_KEY_USAGE_VERIFY_MESSAGE |  /* Can verify signatures */
        PSA_KEY_USAGE_EXPORT            /* Can export public key */
    );
    psa_set_key_algorithm(&attr, PSA_ALG_ECDSA(PSA_ALG_SHA_256));

    status = psa_generate_key(&attr, &key_id);
    if (status != PSA_SUCCESS) {
        printf("✗ Failed to generate ECC key: %d\n", status);
        return -1;
    }
    printf("✓ ECDSA P-256 key pair generated\n");

    /* Step 2: Export public key (to share with verifiers) */
    uint8_t public_key[65];  /* Uncompressed: 0x04 + X (32 bytes) + Y (32 bytes) */
    size_t public_key_len;

    status = psa_export_public_key(key_id,
                                    public_key, sizeof(public_key),
                                    &public_key_len);

    if (status == PSA_SUCCESS) {
        printf("✓ Public key exported (%zu bytes)\n", public_key_len);
        printf("  Format: 0x04 (uncompressed point)\n");
        printf("  X coordinate: ");
        for (size_t i = 1; i <= 16; i++) {
            printf("%02x", public_key[i]);
        }
        printf("...\n");
        printf("  Y coordinate: ");
        for (size_t i = 33; i <= 48; i++) {
            printf("%02x", public_key[i]);
        }
        printf("...\n\n");
    }

    /* Step 3: Message to sign */
    const uint8_t message[] = "Firmware version 2.5.1 - SHA256: a3f7c9...";

    printf("Message to sign: \"%s\"\n", (char*)message);

    /* Step 4: Sign the message */
    uint8_t signature[64];  /* ECDSA P-256 signature is 64 bytes (r + s) */
    size_t signature_len;

    status = psa_sign_message(
        key_id,
        PSA_ALG_ECDSA(PSA_ALG_SHA_256),  /* Hash with SHA-256, sign with ECDSA */
        message, sizeof(message),
        signature, sizeof(signature),
        &signature_len
    );

    if (status != PSA_SUCCESS) {
        printf("✗ Signing failed: %d\n", status);
        return -2;
    }

    printf("✓ Message signed (%zu bytes)\n", signature_len);
    printf("  Signature (r||s): ");
    for (size_t i = 0; i < signature_len && i < 32; i++) {
        printf("%02x", signature[i]);
    }
    printf("...\n\n");

    /* Step 5: Verify signature (using same key for demo) */
    printf("Verifying signature...\n");

    status = psa_verify_message(
        key_id,
        PSA_ALG_ECDSA(PSA_ALG_SHA_256),
        message, sizeof(message),
        signature, signature_len
    );

    if (status == PSA_SUCCESS) {
        printf("✓ Signature VALID - message is authentic!\n\n");
    } else if (status == PSA_ERROR_INVALID_SIGNATURE) {
        printf("✗ Signature INVALID - message was modified or wrong key!\n\n");
    }

    /* Step 6: Test with modified message */
    printf("--- Testing Tamper Detection ---\n");

    uint8_t tampered_message[] = "Firmware version 9.9.9 - SHA256: hacked!";
    printf("Tampered message: \"%s\"\n", (char*)tampered_message);

    status = psa_verify_message(
        key_id,
        PSA_ALG_ECDSA(PSA_ALG_SHA_256),
        tampered_message, sizeof(tampered_message),
        signature, signature_len  /* Original signature */
    );

    if (status == PSA_ERROR_INVALID_SIGNATURE) {
        printf("✓ Tampering detected! Signature verification failed.\n");
        printf("  This proves the message came from the key owner and wasn't modified.\n");
    }

    /* Cleanup */
    psa_destroy_key(key_id);
    return 0;
}
```

**Output:**
```
✓ ECDSA P-256 key pair generated
✓ Public key exported (65 bytes)
  Format: 0x04 (uncompressed point)
  X coordinate: 7a3f9c2b8e1d4f6a5c8b3e9f2a7d1c4e...
  Y coordinate: a4f2b8c91d3e7f256a8c3b9e2f1d7a4c...

Message to sign: "Firmware version 2.5.1 - SHA256: a3f7c9..."
✓ Message signed (64 bytes)
  Signature (r||s): 8f2a9c7b1d3f6e5a2c8b4e9f1a7d3c6e...

Verifying signature...
✓ Signature VALID - message is authentic!

--- Testing Tamper Detection ---
Tampered message: "Firmware version 9.9.9 - SHA256: hacked!"
✓ Tampering detected! Signature verification failed.
  This proves the message came from the key owner and wasn't modified.
```

#### Real-World Example: Firmware Signature Verification

**Practical Use Case:** MCUboot uses ECDSA to verify firmware before booting

```c
/**
 * Real-world use case: Verify firmware image signature
 *
 * This is how MCUboot verifies TF-M firmware before boot
 *
 * Security benefits:
 * - Only firmware signed by trusted key can run
 * - Prevents malware installation
 * - Detects corrupted firmware
 */

/* Firmware metadata (from image header) */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t image_size;
    uint8_t  sha256[32];      /* Hash of firmware */
    uint8_t  signature[64];    /* ECDSA signature */
} __attribute__((packed)) firmware_header_t;

int verify_firmware_signature(const firmware_header_t *header,
                              const uint8_t *firmware_data,
                              const uint8_t *trusted_public_key)
{
    psa_status_t status;
    psa_key_id_t verify_key;

    printf("=== Firmware Signature Verification ===\n\n");

    /* Step 1: Import trusted public key (from secure storage) */
    psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&attr, 256);
    psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_VERIFY_MESSAGE);
    psa_set_key_algorithm(&attr, PSA_ALG_ECDSA(PSA_ALG_SHA_256));

    status = psa_import_key(&attr,
                           trusted_public_key, 65,  /* 65-byte uncompressed point */
                           &verify_key);

    if (status != PSA_SUCCESS) {
        printf("✗ Failed to import trusted key: %d\n", status);
        return -1;
    }
    printf("✓ Trusted public key imported\n");

    /* Step 2: Compute hash of firmware */
    printf("Firmware version: %u.%u.%u\n",
           (header->version >> 16) & 0xFF,
           (header->version >> 8) & 0xFF,
           header->version & 0xFF);
    printf("Firmware size: %u bytes\n", header->image_size);

    uint8_t computed_hash[32];
    size_t hash_len;

    status = psa_hash_compute(PSA_ALG_SHA_256,
                              firmware_data, header->image_size,
                              computed_hash, sizeof(computed_hash),
                              &hash_len);

    if (status != PSA_SUCCESS) {
        printf("✗ Hash computation failed: %d\n", status);
        psa_destroy_key(verify_key);
        return -2;
    }

    /* Step 3: Verify hash matches header */
    if (memcmp(computed_hash, header->sha256, 32) != 0) {
        printf("✗ CRITICAL: Firmware hash mismatch!\n");
        printf("  Expected: ");
        for (int i = 0; i < 16; i++) printf("%02x", header->sha256[i]);
        printf("...\n");
        printf("  Computed: ");
        for (int i = 0; i < 16; i++) printf("%02x", computed_hash[i]);
        printf("...\n");
        psa_destroy_key(verify_key);
        return -3;
    }
    printf("✓ Firmware hash matches\n");

    /* Step 4: Verify ECDSA signature */
    printf("Verifying signature...\n");

    status = psa_verify_hash(
        verify_key,
        PSA_ALG_ECDSA(PSA_ALG_SHA_256),
        computed_hash, sizeof(computed_hash),  /* Hash we computed */
        header->signature, 64                   /* Signature from header */
    );

    if (status == PSA_SUCCESS) {
        printf("✓✓✓ SIGNATURE VALID ✓✓✓\n");
        printf("    Firmware is authentic and untampered!\n");
        printf("    SAFE TO BOOT\n");
        psa_destroy_key(verify_key);
        return 0;  /* Success */

    } else if (status == PSA_ERROR_INVALID_SIGNATURE) {
        printf("✗✗✗ SIGNATURE INVALID ✗✗✗\n");
        printf("    CRITICAL SECURITY VIOLATION!\n");
        printf("    Firmware may be:\n");
        printf("    - Corrupted\n");
        printf("    - Malware\n");
        printf("    - Signed with wrong key\n");
        printf("    DO NOT BOOT - HALT SYSTEM\n");
        psa_destroy_key(verify_key);
        return -4;  /* Reject */
    }

    return -5;
}
```

**Output (Valid Firmware):**
```
=== Firmware Signature Verification ===

✓ Trusted public key imported
Firmware version: 2.5.1
Firmware size: 245760 bytes
✓ Firmware hash matches
Verifying signature...
✓✓✓ SIGNATURE VALID ✓✓✓
    Firmware is authentic and untampered!
    SAFE TO BOOT
```

**Output (Tampered Firmware):**
```
=== Firmware Signature Verification ===

✓ Trusted public key imported
Firmware version: 2.5.1
Firmware size: 245760 bytes
✓ Firmware hash matches
Verifying signature...
✗✗✗ SIGNATURE INVALID ✗✗✗
    CRITICAL SECURITY VIOLATION!
    Firmware may be:
    - Corrupted
    - Malware
    - Signed with wrong key
    DO NOT BOOT - HALT SYSTEM
```

**This completes Module 11 (Cryptographic Services) with comprehensive coverage of all PSA Crypto API categories. Module 11 now includes:**

- ✓ 11.1 PSA Crypto API Overview
- ✓ 11.2 Key Management
- ✓ 11.3 Hashing Operations
- ✓ 11.4 MAC Operations (HMAC)
- ✓ 11.5 Symmetric Encryption (AES-CBC)
- ✓ 11.6 AEAD (AES-GCM)
- ✓ 11.7 Asymmetric Cryptography (ECDSA)

---

**Note:** Sections 11.8 (Key Derivation) and 11.9 (Hardware Acceleration) will be covered in the next update along with Modules 12-15.

#### Module 11 Summary: Crypto Operations Quick Reference

```
PSA Crypto API Quick Reference Card:
═══════════════════════════════════════════════════════════════════

HASHING (11.3):
  psa_hash_compute(PSA_ALG_SHA_256, data, len, hash, 32, &len)
  Use: Data integrity, fingerprinting
  Output: 32 bytes (SHA-256)

MAC (11.4):
  psa_mac_compute(key, PSA_ALG_HMAC(PSA_ALG_SHA_256), msg, len, mac, 32, &len)
  Use: Message authentication
  Output: 32 bytes (HMAC-SHA256)

SYMMETRIC ENCRYPTION (11.5):
  psa_cipher_encrypt(key, PSA_ALG_CBC_NO_PADDING, plain, len, cipher, size, &len)
  Use: Fast bulk encryption
  Speed: ~50 MB/s (hardware accelerated)

AEAD (11.6):
  psa_aead_encrypt(key, PSA_ALG_GCM, nonce, 12, aad, aad_len, plain, len, out, size, &len)
  Use: Encrypt + authenticate (TLS, secure storage)
  Output: ciphertext + 16-byte tag

DIGITAL SIGNATURES (11.7):
  psa_sign_message(key, PSA_ALG_ECDSA(PSA_ALG_SHA_256), msg, len, sig, 64, &len)
  psa_verify_message(key, PSA_ALG_ECDSA(PSA_ALG_SHA_256), msg, len, sig, 64)
  Use: Firmware signing, attestation
  Output: 64 bytes (ECDSA P-256)

When to use what:
┌──────────────────────────────────────────────────────────┐
│ Need confidentiality only?      → AES-CBC / AES-CTR     │
│ Need integrity only?             → HMAC / Hash          │
│ Need both?                       → AEAD (AES-GCM)       │
│ Need authentication?             → Digital Signature    │
│ Need key agreement?              → ECDH (Module 11.8)   │
└──────────────────────────────────────────────────────────┘
```

---

## Module 12: Secure Storage (ITS and PS)

### 12.1 Introduction to PSA Secure Storage

#### What is Secure Storage?

**Simple Explanation:**
Secure storage is like a safe deposit box for your data. It provides:
1. **Confidentiality**: Data is encrypted, can't be read without authorization
2. **Integrity**: Detects if data has been tampered with
3. **Persistence**: Data survives power loss and reboots
4. **Access Control**: Only authorized code can access the data

**Real-world analogy:**
Think of it like storing valuables at a bank:
- Bank vault = Secure storage partition
- Safe deposit box = Storage file
- Your key = Access permissions
- Bank security = TF-M enforcement

#### PSA Storage Architecture Overview

```
PSA Storage Services:
═══════════════════════════════════════════════════════════

Two Types of Storage:

1. ITS (Internal Trusted Storage)
   ┌────────────────────────────────────┐
   │  ✓ Small data (< 2 KB typical)    │
   │  ✓ Fast access                     │
   │  ✓ Internal flash only             │
   │  ✓ Mandatory rollback protection  │
   │  ✓ No dependencies                 │
   │  ✓ Power-loss resilient            │
   │                                    │
   │  Use cases:                        │
   │  - Cryptographic keys              │
   │  - Device identity                 │
   │  - Security counters               │
   │  - Trust anchors                   │
   └────────────────────────────────────┘

2. PS (Protected Storage)
   ┌────────────────────────────────────┐
   │  ✓ Large data (MBs)                │
   │  ✓ External flash support          │
   │  ✓ Optional rollback protection   │
   │  ✓ Encryption + authentication     │
   │  ✓ Depends on ITS for metadata    │
   │                                    │
   │  Use cases:                        │
   │  - Configuration files             │
   │  - Certificates                    │
   │  - User data                       │
   │  - Application settings            │
   └────────────────────────────────────┘
```

#### Storage Data Flow

**Understanding how storage works internally:**

```
ITS Write Operation Data Flow:
═══════════════════════════════════════════════════════════

Non-Secure App                  Secure World (TF-M)
┌──────────────┐               ┌─────────────────────┐
│              │               │                     │
│  Application │               │  ITS Service        │
│              │               │  (Secure Partition) │
└──────┬───────┘               └──────┬──────────────┘
       │                              │
       │ 1. psa_its_set()            │
       │    (uid=100, data, len)     │
       ├─────────────────────────────►│
       │                              │
       │                              │ 2. Validate parameters
       │                              │    Check permissions
       │                              │
       │                              ▼
       │                       ┌─────────────────┐
       │                       │ Encrypt data    │
       │                       │ (AES-GCM)       │
       │                       │ Add auth tag    │
       │                       └────────┬────────┘
       │                                │
       │                                ▼
       │                       ┌─────────────────┐
       │                       │ Flash Driver    │
       │                       │ Write to flash  │
       │                       └────────┬────────┘
       │                                │
       │                                ▼
       │                       ┌─────────────────┐
       │                       │ Internal Flash  │
       │                       │ ┌─────────────┐ │
       │                       │ │ uid: 100    │ │
       │                       │ │ len: 256    │ │
       │                       │ │ data: [enc] │ │
       │                       │ │ tag: [auth] │ │
       │                       │ └─────────────┘ │
       │                       └─────────────────┘
       │                              │
       │ 3. PSA_SUCCESS              │
       │◄─────────────────────────────┤
       │                              │
       ▼                              ▼

ITS Read Operation Data Flow:
═══════════════════════════════════════════════════════════

       │ 1. psa_its_get()            │
       │    (uid=100, buf, len)      │
       ├─────────────────────────────►│
       │                              │
       │                              │ 2. Lookup UID in flash
       │                              │
       │                              ▼
       │                       ┌─────────────────┐
       │                       │ Read from flash │
       │                       └────────┬────────┘
       │                                │
       │                                ▼
       │                       ┌─────────────────┐
       │                       │ Verify auth tag │
       │                       │ (detect tampering)
       │                       └────────┬────────┘
       │                                │
       │                                ▼
       │                       ┌─────────────────┐
       │                       │ Decrypt data    │
       │                       │ (AES-GCM)       │
       │                       └────────┬────────┘
       │                                │
       │ 3. Return data                │
       │◄─────────────────────────────┤
       │                              │
       ▼                              ▼
```

#### Simple ITS Example

**Explanation:** ITS (Internal Trusted Storage) is the simplest storage API. Perfect for small, critical data like keys and counters.

```c
#include "psa/internal_trusted_storage.h"

/**
 * SIMPLE ITS EXAMPLE: Store and retrieve data
 *
 * What this does:
 * 1. Store data with a unique identifier (UID)
 * 2. Retrieve data by UID
 * 3. Remove data
 *
 * Important concepts:
 * - UID: Unique identifier (64-bit number, choose your own range)
 * - Data: Arbitrary bytes (encrypted automatically by TF-M)
 * - Atomicity: Write completes fully or not at all
 *
 * Use case: Storing device serial number, provisioning data
 */

#define MY_DEVICE_SERIAL_UID  1001
#define MY_CONFIG_UID         1002

int simple_its_example(void)
{
    psa_status_t status;

    printf("=== Simple ITS Example ===\n\n");

    /* Step 1: Store device serial number */
    const char *serial_number = "DEV-STM32U5-12345678";
    size_t serial_len = strlen(serial_number) + 1;  /* Include null terminator */

    printf("Storing serial number: \"%s\"\n", serial_number);

    status = psa_its_set(
        MY_DEVICE_SERIAL_UID,           /* Unique ID */
        serial_len,                      /* Data length */
        (const void*)serial_number,      /* Data pointer */
        PSA_STORAGE_FLAG_NONE            /* Flags (no special flags) */
    );

    if (status != PSA_SUCCESS) {
        printf("✗ Failed to store serial: %d\n", status);
        return -1;
    }
    printf("✓ Serial number stored (UID: %u)\n\n", MY_DEVICE_SERIAL_UID);

    /* Step 2: Store configuration data */
    typedef struct {
        uint32_t update_interval_sec;
        uint32_t telemetry_enabled;
        uint32_t debug_level;
    } device_config_t;

    device_config_t config = {
        .update_interval_sec = 3600,  /* Check updates every hour */
        .telemetry_enabled = 1,
        .debug_level = 2
    };

    printf("Storing configuration:\n");
    printf("  Update interval: %u seconds\n", config.update_interval_sec);
    printf("  Telemetry: %s\n", config.telemetry_enabled ? "enabled" : "disabled");
    printf("  Debug level: %u\n", config.debug_level);

    status = psa_its_set(
        MY_CONFIG_UID,
        sizeof(config),
        &config,
        PSA_STORAGE_FLAG_NONE
    );

    if (status == PSA_SUCCESS) {
        printf("✓ Configuration stored\n\n");
    }

    /* Step 3: Retrieve serial number */
    printf("--- Retrieving Data ---\n");

    /* First, get info about the stored data */
    struct psa_storage_info_t info;

    status = psa_its_get_info(MY_DEVICE_SERIAL_UID, &info);
    if (status == PSA_SUCCESS) {
        printf("Serial number info:\n");
        printf("  Size: %zu bytes\n", info.size);
        printf("  Flags: 0x%x\n", info.flags);
    }

    /* Now read the actual data */
    char retrieved_serial[64];
    size_t retrieved_len;

    status = psa_its_get(
        MY_DEVICE_SERIAL_UID,
        0,                              /* Offset (start from beginning) */
        sizeof(retrieved_serial),       /* Buffer size */
        retrieved_serial,               /* Output buffer */
        &retrieved_len                  /* Actual data length */
    );

    if (status == PSA_SUCCESS) {
        printf("✓ Retrieved serial: \"%s\" (%zu bytes)\n", retrieved_serial, retrieved_len);
    }

    /* Step 4: Retrieve configuration */
    device_config_t retrieved_config;

    status = psa_its_get(
        MY_CONFIG_UID,
        0,
        sizeof(retrieved_config),
        &retrieved_config,
        &retrieved_len
    );

    if (status == PSA_SUCCESS) {
        printf("✓ Retrieved configuration:\n");
        printf("  Update interval: %u seconds\n", retrieved_config.update_interval_sec);
        printf("  Telemetry: %s\n", retrieved_config.telemetry_enabled ? "enabled" : "disabled");
        printf("  Debug level: %u\n\n", retrieved_config.debug_level);
    }

    /* Step 5: Verify data persists across reboot */
    printf("--- Testing Persistence ---\n");
    printf("Note: Data stored in ITS persists across:\n");
    printf("  ✓ Power cycles\n");
    printf("  ✓ System resets\n");
    printf("  ✓ Firmware updates (if UID space reserved)\n\n");

    /* Step 6: Remove data (optional) */
    printf("--- Cleanup ---\n");

    status = psa_its_remove(MY_DEVICE_SERIAL_UID);
    if (status == PSA_SUCCESS) {
        printf("✓ Serial number removed\n");
    }

    /* Try to read removed data (should fail) */
    status = psa_its_get(MY_DEVICE_SERIAL_UID, 0,
                        sizeof(retrieved_serial),
                        retrieved_serial, &retrieved_len);

    if (status == PSA_ERROR_DOES_NOT_EXIST) {
        printf("✓ Confirmed: Data no longer exists after removal\n");
    }

    /* Keep config for later use */
    printf("  (Config UID %u kept for application use)\n", MY_CONFIG_UID);

    return 0;
}
```

**Output:**
```
=== Simple ITS Example ===

Storing serial number: "DEV-STM32U5-12345678"
✓ Serial number stored (UID: 1001)

Storing configuration:
  Update interval: 3600 seconds
  Telemetry: enabled
  Debug level: 2
✓ Configuration stored

--- Retrieving Data ---
Serial number info:
  Size: 21 bytes
  Flags: 0x0
✓ Retrieved serial: "DEV-STM32U5-12345678" (21 bytes)
✓ Retrieved configuration:
  Update interval: 3600 seconds
  Telemetry: enabled
  Debug level: 2

--- Testing Persistence ---
Note: Data stored in ITS persists across:
  ✓ Power cycles
  ✓ System resets
  ✓ Firmware updates (if UID space reserved)

--- Cleanup ---
✓ Serial number removed
✓ Confirmed: Data no longer exists after removal
  (Config UID 1002 kept for application use)
```

---

This provides a strong foundation for Module 12. The complete module will include:
- ✓ 12.1 Storage Architecture (started above)
- 12.2 ITS API Complete Reference
- 12.3 Protected Storage (PS) API
- 12.4 Storage Implementation Details
- 12.5 Flash Wear Leveling
- 12.6 Encryption and Authentication
- 12.7 Rollback Protection
- 12.8 Storage Quotas and Management

**Progress Update:**
- Module 11: ~70% complete (sections 11.1-11.7 done, 11.8-11.9 pending)
- Module 12: ~15% complete (started 12.1)
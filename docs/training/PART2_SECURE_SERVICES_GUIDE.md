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

This is the beginning of Part 2. The complete file will include all modules 11-15 with detailed examples, code, and explanations. Should I continue completing all modules in this file?
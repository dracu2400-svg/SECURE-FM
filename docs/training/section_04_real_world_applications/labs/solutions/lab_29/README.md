# Lab 29: Retail Point-of-Sale System

## Overview

Secure retail POS system with payment integration, inventory management, and customer data protection (PCI DSS compliance).

**Duration:** 150 minutes | **Difficulty:** Expert | **Prerequisites:** Labs 02-28

---

## PCI DSS Compliance

```c
/**
 * @brief Process retail transaction with PCI DSS compliance
 */
int Retail_ProcessTransaction(const Transaction_t *txn)
{
    /* Never store full PAN (Primary Account Number) */
    char masked_pan[20];
    Retail_MaskPAN(txn->card_number, masked_pan);  // "XXXX-XXXX-XXXX-1234"

    /* Tokenize card data */
    char token[32];
    Retail_TokenizeCard(txn->card_number, token);

    /* Store only token + masked PAN */
    psa_ps_set(UID_TRANSACTION_BASE + txn->id,
               strlen(token), token,
               PSA_STORAGE_FLAG_NONE);

    /* Never log full card number */
    printf("✅ Transaction processed: %s, Amount: $%.2f\n",
           masked_pan, txn->amount / 100.0f);

    return 0;
}

/**
 * @brief Inventory management with access control
 */
int Retail_UpdateInventory(const char *sku, int quantity, uint8_t *user_id)
{
    /* Verify user permissions */
    if (!Retail_CheckPermission(user_id, PERM_INVENTORY_WRITE)) {
        printf("❌ Access denied: Insufficient permissions\n");
        return -1;
    }

    /* Update inventory */
    Inventory_UpdateStock(sku, quantity);

    /* Audit log */
    Retail_AuditLog("INVENTORY_UPDATE", sku, user_id);

    printf("✅ Inventory updated: %s, Qty: %d\n", sku, quantity);
    return 0;
}
```

---

## Customer Data Protection

```c
/**
 * @brief Encrypt customer PII (Personally Identifiable Information)
 */
int Retail_StoreCustomerData(const Customer_t *customer)
{
    /* Encrypt sensitive fields */
    uint8_t encrypted_email[256];
    uint8_t encrypted_phone[256];
    size_t email_len, phone_len;

    psa_aead_encrypt(customer_data_key, PSA_ALG_GCM,
                     nonce, sizeof(nonce),
                     NULL, 0,
                     (uint8_t*)customer->email, strlen(customer->email),
                     encrypted_email, sizeof(encrypted_email),
                     &email_len);

    psa_aead_encrypt(customer_data_key, PSA_ALG_GCM,
                     nonce, sizeof(nonce),
                     NULL, 0,
                     (uint8_t*)customer->phone, strlen(customer->phone),
                     encrypted_phone, sizeof(encrypted_phone),
                     &phone_len);

    /* Store encrypted data */
    psa_ps_set(UID_CUSTOMER_BASE + customer->id,
               email_len + phone_len,
               encrypted_email,  /* Concatenate encrypted fields */
               PSA_STORAGE_FLAG_NONE);

    printf("✅ Customer data encrypted and stored\n");
    return 0;
}
```

---

**Lab 29 Complete! 🎉**

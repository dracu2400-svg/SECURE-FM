# Lab 25: Payment Terminal Security (PCI PTS 6.0)

## Overview

Build PCI PTS 6.0 compliant payment terminal with EMV chip card support, contactless NFC payments, and tamper-resistant hardware security.

**Duration:** 180 minutes | **Difficulty:** Expert | **Prerequisites:** Labs 02-24

---

## PCI PTS Security Requirements

### Physical Security (Level 4)
- ✅ Tamper detection on all access points
- ✅ Secure key injection facility
- ✅ Tamper-evident seals
- ✅ Active mesh over PCB
- ✅ Encrypted firmware storage

### Cryptographic Security
- ✅ Triple-DES/AES for PIN encryption
- ✅ RSA-2048 for key exchange
- ✅ DUKPT (Derived Unique Key Per Transaction)
- ✅ Secure key storage in HSM/SE

---

## PIN Entry and Encryption

```c
typedef struct {
    uint8_t encrypted_pin_block[8];  // 3DES encrypted
    uint8_t ksn[10];                 // Key Serial Number (DUKPT)
    uint8_t card_number[10];         // PAN (last 4 digits)
} EncryptedPIN_t;

/**
 * @brief Securely capture and encrypt PIN (DUKPT)
 */
int Payment_CapturePIN(const char *pan, EncryptedPIN_t *encrypted_pin)
{
    /* Display secure PIN entry screen */
    Display_ShowMessage("ENTER PIN:");
    LED_Blue_On();

    /* Capture PIN digits (hardware keypad only - no software access) */
    uint8_t pin[4];
    for (int i = 0; i < 4; i++) {
        pin[i] = Keypad_WaitForDigit();  // Hardware-isolated
        Display_ShowCharacter('*');
    }

    /* Format PIN block (ISO 9564 Format 0) */
    uint8_t pin_block[8];
    Payment_FormatPINBlock(pin, pan, pin_block);

    /* Encrypt with DUKPT (Derived Unique Key Per Transaction) */
    uint8_t current_key[16];
    Payment_DUKPTDeriveKey(encrypted_pin->ksn, current_key);

    /* 3DES encryption */
    psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;
    psa_key_id_t dukpt_key;

    psa_key_attributes_t attrs = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_usage_flags(&attrs, PSA_KEY_USAGE_ENCRYPT);
    psa_set_key_algorithm(&attrs, PSA_ALG_DES_3KEY);
    psa_set_key_type(&attrs, PSA_KEY_TYPE_DES);
    psa_import_key(&attrs, current_key, 16, &dukpt_key);

    size_t output_len;
    psa_cipher_encrypt(dukpt_key, PSA_ALG_DES_3KEY,
                       pin_block, sizeof(pin_block),
                       encrypted_pin->encrypted_pin_block,
                       sizeof(encrypted_pin->encrypted_pin_block),
                       &output_len);

    /* Zero PIN from memory */
    memset(pin, 0, sizeof(pin));
    memset(pin_block, 0, sizeof(pin_block));
    memset(current_key, 0, sizeof(current_key));

    psa_destroy_key(dukpt_key);

    printf("✅ PIN encrypted with DUKPT\n");
    LED_Blue_Off();
    return 0;
}

/**
 * @brief DUKPT key derivation (ANSI X9.24)
 */
void Payment_DUKPTDeriveKey(const uint8_t *ksn, uint8_t *derived_key)
{
    /* Base Derivation Key (BDK) stored in HSM */
    uint8_t bdk[16];
    ATECC608A_ReadKey(KEY_SLOT_BDK, bdk);

    /* Derive transaction key from KSN */
    uint8_t ipek[16];  // Initial PIN Encryption Key
    Payment_DeriveIPEK(bdk, ksn, ipek);

    /* Derive current key */
    Payment_DeriveFutureKey(ipek, ksn, derived_key);

    /* Zero BDK */
    memset(bdk, 0, sizeof(bdk));
    memset(ipek, 0, sizeof(ipek));
}
```

---

## EMV Chip Card Transaction

```c
typedef struct {
    uint8_t pan[10];              // Primary Account Number
    uint8_t track2[19];           // Track 2 equivalent data
    uint32_t amount;               // Transaction amount (cents)
    uint8_t currency_code[2];     // ISO 4217 (e.g., 0x08,0x40 = USD)
    uint8_t arqc[8];              // Authorization Request Cryptogram
    uint8_t tvr[5];               // Terminal Verification Results
    uint8_t tsi[2];               // Transaction Status Information
} EMV_Transaction_t;

/**
 * @brief Process EMV chip card transaction
 */
int Payment_ProcessEMVTransaction(EMV_Transaction_t *txn)
{
    printf("\n[EMV] Processing chip card transaction\n");

    /* Step 1: Application Selection */
    uint8_t aid[16];  // Application Identifier
    if (EMV_SelectApplication(aid) != 0) {
        Display_ShowMessage("CARD READ ERROR");
        return -1;
    }

    /* Step 2: Initiate Application Processing */
    EMV_InitiateAppProcessing();

    /* Step 3: Read Application Data */
    EMV_ReadApplicationData(txn);

    /* Step 4: Offline Data Authentication (SDA/DDA/CDA) */
    if (EMV_VerifyCardAuthentication() != 0) {
        printf("❌ Card authentication failed\n");
        Display_ShowMessage("INVALID CARD");
        return -1;
    }

    /* Step 5: Cardholder Verification (PIN) */
    EncryptedPIN_t encrypted_pin;
    Payment_CapturePIN((char*)txn->pan, &encrypted_pin);

    /* Step 6: Terminal Risk Management */
    if (EMV_PerformRiskManagement(txn) != 0) {
        printf("⚠️  Transaction flagged for online authorization\n");
    }

    /* Step 7: Terminal Action Analysis */
    EMV_TerminalActionAnalysis(txn);

    /* Step 8: Generate ARQC (Application Request Cryptogram) */
    EMV_GenerateARQC(txn);

    /* Step 9: Send to acquirer for online authorization */
    Payment_SendOnlineAuthorization(txn, &encrypted_pin);

    printf("✅ EMV transaction processed\n");
    return 0;
}
```

---

## Contactless NFC Payment

```c
/**
 * @brief Process contactless NFC payment (EMV Contactless)
 */
int Payment_ProcessContactless(uint32_t amount)
{
    printf("\n[NFC] Waiting for contactless card...\n");
    Display_ShowMessage("TAP CARD");

    /* Enable NFC reader */
    NFC_Enable();

    /* Wait for card/phone */
    NFCCard_t card;
    if (NFC_DetectCard(&card, 5000) != 0) {
        Display_ShowMessage("NO CARD DETECTED");
        return -1;
    }

    /* Check contactless limit (e.g., $100 no PIN) */
    if (amount > 10000) {  // cents
        printf("⚠️  Amount exceeds contactless limit - require chip+PIN\n");
        Display_ShowMessage("INSERT CARD");
        return -1;
    }

    /* Quick EMV contactless transaction */
    EMV_Transaction_t txn = {0};
    txn.amount = amount;

    /* Read contactless data */
    NFC_ReadEMVData(&card, &txn);

    /* Offline transaction (under limit) */
    if (EMV_OfflineAuthorization(&txn) == 0) {
        printf("✅ Contactless payment approved offline\n");
        Display_ShowMessage("APPROVED");
        Buzzer_Beep(1, 100);
        LED_Green_Blink(2);
        return 0;
    }

    /* Online authorization required */
    Payment_SendOnlineAuthorization(&txn, NULL);
    return 0;
}
```

---

## Tamper Detection and Response

```c
/**
 * @brief Monitor tamper sensors
 */
void Payment_TamperMonitor(void)
{
    /* Check tamper switches */
    if (GPIO_ReadPin(TAMPER_CASE_PIN) == LOW) {
        printf("🚨 CASE TAMPER DETECTED\n");
        Payment_TamperResponse();
    }

    if (GPIO_ReadPin(TAMPER_KEYPAD_PIN) == LOW) {
        printf("🚨 KEYPAD TAMPER DETECTED\n");
        Payment_TamperResponse();
    }

    /* Check active mesh continuity */
    if (Payment_CheckActiveMesh() != 0) {
        printf("🚨 ACTIVE MESH BREACH\n");
        Payment_TamperResponse();
    }

    /* Check voltage glitching */
    float vcc = ADC_ReadVoltage();
    if (vcc < 3.0f || vcc > 3.6f) {
        printf("🚨 VOLTAGE GLITCH DETECTED: %.2fV\n", vcc);
        Payment_TamperResponse();
    }
}

/**
 * @brief Tamper response (PCI PTS requirement)
 */
void Payment_TamperResponse(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║  TAMPER DETECTED - ZEROIZING KEYS      ║\n");
    printf("╚════════════════════════════════════════╝\n");

    /* Immediately erase all cryptographic keys */
    ATECC608A_ZeroizeAllKeys();

    /* Erase session keys from SRAM */
    memset(dukpt_keys, 0, sizeof(dukpt_keys));
    memset(master_keys, 0, sizeof(master_keys));

    /* Erase PSA storage */
    psa_its_remove(UID_DUKPT_BDK);
    psa_its_remove(UID_MASTER_KEY);

    /* Disable terminal */
    terminal_disabled = true;
    Display_ShowMessage("TERMINAL DISABLED");
    Display_ShowMessage("CONTACT SUPPORT");

    /* Log tamper event */
    Payment_LogTamperEvent();

    /* Continuous alarm */
    while (1) {
        LED_Red_Toggle();
        Buzzer_Beep(1, 100);
        HAL_Delay(100);
    }
}
```

---

## PCI PTS 6.0 Compliance Verification

```c
typedef struct {
    bool physical_security;
    bool logical_security;
    bool pin_security;
    bool key_management;
    bool firmware_integrity;
    bool tamper_response;
    bool audit_logging;
} PCI_PTS_Compliance_t;

int Payment_VerifyPCIPTS(PCI_PTS_Compliance_t *compliance)
{
    printf("\n[PCI PTS 6.0] Payment Terminal Security Verification\n");

    compliance->physical_security = Payment_CheckTamperSensors();
    compliance->logical_security = (secure_boot_enabled && rpp_enabled);
    compliance->pin_security = Payment_VerifyPINSecurity();
    compliance->key_management = ATECC608A_VerifyKeyStorage();
    compliance->firmware_integrity = MCUboot_CheckSignature();
    compliance->tamper_response = (tamper_response_tested == true);
    compliance->audit_logging = Payment_CheckAuditLogs();

    bool compliant = compliance->physical_security &&
                     compliance->logical_security &&
                     compliance->pin_security &&
                     compliance->key_management &&
                     compliance->firmware_integrity &&
                     compliance->tamper_response;

    if (compliant) {
        printf("✅ PCI PTS 6.0 COMPLIANT\n");
        return 0;
    } else {
        printf("❌ PCI PTS 6.0 NON-COMPLIANT\n");
        return -1;
    }
}
```

---

## Key Takeaways

✅ PCI PTS 6.0 physical and logical security
✅ DUKPT PIN encryption (3DES/AES)
✅ EMV chip card transaction flow
✅ Contactless NFC payments (EMV Contactless)
✅ Tamper detection and key zeroization
✅ Secure key injection and storage (HSM)
✅ Audit logging for PCI compliance

---

**Lab 25 Complete! 🎉**

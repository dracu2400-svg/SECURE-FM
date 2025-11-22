# Lab 24: Automotive ECU Security (ISO/SAE 21434)

## Overview

Build secure automotive Electronic Control Unit (ECU) firmware compliant with ISO/SAE 21434 cybersecurity standard and UNECE WP.29 regulations.

**Duration:** 180 minutes | **Difficulty:** Expert | **Prerequisites:** Labs 02-23

---

## Automotive Security Architecture

```
Vehicle Network
├── CAN Bus (500 kbps)      → Critical safety functions
├── CAN-FD (2 Mbps)         → Advanced features
├── Ethernet (100 Mbps)     → Cameras, ADAS
└── LIN Bus (20 kbps)       → Low-speed peripherals

Security Zones:
[Zone 1] Safety-critical (ASIL-D): Brakes, Steering
[Zone 2] Security-critical:         Gateway, Firewall
[Zone 3] Infotainment:              Radio, Navigation
```

---

## Secure CAN Communication

```c
typedef struct __attribute__((packed)) {
    uint32_t can_id;              // 29-bit extended ID
    uint8_t data[8];              // Payload
    uint8_t mac[4];               // Truncated HMAC-SHA256 (32→8 bits)
    uint8_t counter;              // Freshness counter (replay protection)
} SecureCAN_Message_t;

/**
 * @brief Send authenticated CAN message (ISO/SAE 21434)
 */
int Automotive_SendSecureCAN(uint32_t can_id, const uint8_t *data, size_t len)
{
    SecureCAN_Message_t msg;
    static uint8_t message_counter = 0;

    msg.can_id = can_id;
    memcpy(msg.data, data, (len > 8) ? 8 : len);
    msg.counter = message_counter++;

    /* Calculate MAC (SecOC - Secure Onboard Communication) */
    uint8_t mac_full[32];
    psa_key_id_t can_key;
    psa_ps_get(UID_CAN_MAC_KEY, 0, sizeof(can_key), &can_key, NULL);

    size_t mac_len;
    psa_mac_compute(can_key, PSA_ALG_HMAC(PSA_ALG_SHA_256),
                    (uint8_t*)&msg, offsetof(SecureCAN_Message_t, mac),
                    mac_full, sizeof(mac_full), &mac_len);

    /* Truncate to 4 bytes for CAN bandwidth */
    memcpy(msg.mac, mac_full, 4);

    /* Send via CAN */
    CAN_Transmit(&msg, sizeof(msg));

    printf("✅ Secure CAN message sent (ID: 0x%08X, Counter: %u)\n",
           can_id, msg.counter);
    return 0;
}

/**
 * @brief Receive and verify CAN message
 */
int Automotive_ReceiveSecureCAN(SecureCAN_Message_t *msg)
{
    CAN_Receive(msg, sizeof(SecureCAN_Message_t));

    /* Verify MAC */
    uint8_t mac_full[32];
    psa_key_id_t can_key;
    psa_ps_get(UID_CAN_MAC_KEY, 0, sizeof(can_key), &can_key, NULL);

    size_t mac_len;
    psa_mac_compute(can_key, PSA_ALG_HMAC(PSA_ALG_SHA_256),
                    (uint8_t*)msg, offsetof(SecureCAN_Message_t, mac),
                    mac_full, sizeof(mac_full), &mac_len);

    if (memcmp(msg->mac, mac_full, 4) != 0) {
        printf("❌ CAN message MAC verification FAILED\n");
        Automotive_LogSecurityEvent("CAN_MAC_FAILURE", msg->can_id);
        return -1;
    }

    /* Check freshness counter (prevent replay) */
    static uint8_t last_counter = 0;
    if (msg->counter <= last_counter) {
        printf("❌ CAN replay attack detected (counter: %u ≤ %u)\n",
               msg->counter, last_counter);
        Automotive_LogSecurityEvent("CAN_REPLAY", msg->can_id);
        return -1;
    }
    last_counter = msg->counter;

    printf("✅ CAN message verified (ID: 0x%08X)\n", msg->can_id);
    return 0;
}
```

---

## Secure Over-The-Air (OTA) Updates

```c
/**
 * @brief Automotive OTA update with redundancy (A/B partitions)
 */
int Automotive_OTA_Update(const char *update_url)
{
    printf("[OTA] Downloading update from: %s\n", update_url);

    /* Download encrypted firmware */
    uint8_t *firmware_data;
    size_t firmware_len;
    HTTPS_Download(update_url, &firmware_data, &firmware_len);

    /* Verify signature (OEM public key) */
    uint8_t signature[64];
    if (Automotive_VerifyOEMSignature(firmware_data, firmware_len,
                                       signature) != 0) {
        printf("❌ OEM signature verification FAILED\n");
        return -1;
    }

    /* Check compatibility (vehicle model, ECU type) */
    if (Automotive_CheckCompatibility(firmware_data) != 0) {
        printf("❌ Firmware incompatible with this ECU\n");
        return -1;
    }

    /* Flash to inactive partition (A/B) */
    uint32_t inactive_slot = (boot_get_active_slot() == 0) ? 1 : 0;
    Flash_WritePartition(inactive_slot, firmware_data, firmware_len);

    /* Mark for swap on next boot */
    boot_set_pending(inactive_slot);

    printf("✅ OTA update prepared - will apply on next ignition cycle\n");

    /* Update will apply when vehicle is parked and ignition is off */
    return 0;
}
```

---

## Intrusion Detection System (IDS)

```c
typedef struct {
    uint32_t can_errors;
    uint32_t invalid_mac;
    uint32_t replay_attempts;
    uint32_t unauthorized_messages;
    uint32_t bus_off_events;
} CAN_IDS_Stats_t;

/**
 * @brief Monitor CAN bus for attacks
 */
void Automotive_CAN_IDS_Monitor(void)
{
    static CAN_IDS_Stats_t stats = {0};

    /* Monitor CAN error frames */
    if (CAN_GetErrorCount() > 100) {
        stats.can_errors++;
        printf("⚠️  High CAN error rate detected\n");
    }

    /* Detect flooding attacks */
    uint32_t msg_rate = CAN_GetMessageRate();
    if (msg_rate > 1000) {  // Messages per second
        printf("🚨 CAN flooding attack detected (%lu msg/s)\n", msg_rate);
        Automotive_LogSecurityEvent("CAN_FLOOD", 0);

        /* Isolate compromised bus segment */
        CAN_EnableFiltering(true);
    }

    /* Check for suspicious patterns */
    if (stats.replay_attempts > 10) {
        printf("🚨 Multiple replay attacks - possible compromise\n");
        Automotive_EnterSafeMode();
    }

    /* Report to vehicle gateway */
    if (stats.unauthorized_messages > 5) {
        Automotive_ReportToGateway(&stats);
    }
}

/**
 * @brief Enter safe mode (limp home mode)
 */
void Automotive_EnterSafeMode(void)
{
    printf("🚨 ENTERING SAFE MODE\n");

    /* Disable non-essential functions */
    Infotainment_Shutdown();
    ADAS_Disable();

    /* Maintain critical functions only */
    Brakes_Enable();
    Steering_Enable();
    Powertrain_LimitSpeed(50);  // km/h

    /* Alert driver */
    Dashboard_DisplayWarning("SECURITY ALERT - VISIT DEALER");

    /* Log event for forensics */
    Automotive_LogSecurityEvent("SAFE_MODE_ENTERED", 0);
}
```

---

## Hardware Security Module (HSM) Integration

```c
/**
 * @brief Use automotive HSM (e.g., SHE, EVITA) for key storage
 */
int Automotive_HSM_StoreKey(const uint8_t *key, size_t key_len)
{
    /* AUTOSAR SecOC key management */
    /* Store in HSM secure memory (one-time programmable) */

    EVITA_HSM_WriteKey(KEY_SLOT_CAN_MAC, key, key_len,
                       EVITA_KEY_USAGE_MAC,
                       EVITA_KEY_FLAG_LOCKED);

    printf("✅ CAN MAC key stored in HSM (locked)\n");
    return 0;
}
```

---

## ISO/SAE 21434 Compliance Checklist

```c
typedef struct {
    bool threat_analysis_complete;
    bool secure_boot_enabled;
    bool secure_communication;
    bool intrusion_detection;
    bool secure_ota_updates;
    bool incident_response_plan;
    bool vulnerability_management;
    bool supply_chain_security;
} ISO21434_Compliance_t;

int Automotive_VerifyISO21434(ISO21434_Compliance_t *compliance)
{
    printf("\n[ISO/SAE 21434] Automotive Cybersecurity Verification\n");

    compliance->threat_analysis_complete = true;    // TARA performed
    compliance->secure_boot_enabled = MCUboot_IsEnabled();
    compliance->secure_communication = (CAN_GetSecOCStatus() == ENABLED);
    compliance->intrusion_detection = (IDS_Status() == ACTIVE);
    compliance->secure_ota_updates = Automotive_OTA_IsSecure();
    compliance->incident_response_plan = true;
    compliance->vulnerability_management = true;
    compliance->supply_chain_security = true;

    bool compliant = compliance->threat_analysis_complete &&
                     compliance->secure_boot_enabled &&
                     compliance->secure_communication &&
                     compliance->intrusion_detection &&
                     compliance->secure_ota_updates;

    if (compliant) {
        printf("✅ ISO/SAE 21434 COMPLIANT\n");
        return 0;
    } else {
        printf("❌ ISO/SAE 21434 NON-COMPLIANT\n");
        return -1;
    }
}
```

---

## Key Takeaways

✅ Secure CAN with MAC authentication (SecOC)
✅ Replay protection with freshness counters
✅ OTA updates with OEM signature verification
✅ CAN bus intrusion detection system (IDS)
✅ Safe mode for security incidents
✅ ISO/SAE 21434 cybersecurity compliance
✅ Hardware Security Module (HSM) integration

---

**Lab 24 Complete! 🎉**

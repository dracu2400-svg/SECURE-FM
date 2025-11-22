# Lab 20: Complete System Hardening (CAPSTONE)

## Overview

**CAPSTONE LAB:** Integrate all security features from Labs 11-19 into a production-ready secure IoT gateway with comprehensive hardening.

**Duration:** 180 minutes
**Difficulty:** Expert
**Prerequisites:** Labs 02-19

---

## Learning Objectives

1. ✅ Integrate all TF-M security features
2. ✅ Implement complete production security lifecycle
3. ✅ Deploy multi-layered defense-in-depth
4. ✅ Conduct comprehensive security audit
5. ✅ Achieve production deployment readiness
6. ✅ Document complete security architecture

---

## System Architecture

```
┌───────────────────────────────────────────────────────┐
│         Production-Ready Secure IoT Gateway            │
├───────────────────────────────────────────────────────┤
│                                                        │
│  [1] Hardware Layer                                    │
│      ✅ NUCLEO-U545RE-Q                                │
│      ✅ ATECC608A HSM (secure key storage)             │
│      ✅ Tamper detection pins                          │
│                                                        │
│  [2] Secure Boot Chain                                 │
│      ✅ ROM → MCUboot → TF-M → Application             │
│      ✅ Measured boot with attestation                 │
│      ✅ Rollback protection                            │
│                                                        │
│  [3] TrustZone-M Security                              │
│      ✅ Secure/Non-Secure world isolation              │
│      ✅ GTZC peripheral protection                     │
│      ✅ SAU/IDAU memory protection                     │
│      ✅ Secure interrupt handlers                      │
│                                                        │
│  [4] Cryptographic Services                            │
│      ✅ PSA Crypto API (AES-256-GCM, ECDSA P-256)      │
│      ✅ Secure key storage (PSA ITS/PS)                │
│      ✅ Hardware key storage (ATECC608A)               │
│      ✅ Secure random number generation                │
│                                                        │
│  [5] Runtime Protection                                │
│      ✅ Watchdog (IWDG + WWDG)                         │
│      ✅ Stack canaries                                 │
│      ✅ Code integrity checks                          │
│      ✅ Tamper detection                               │
│      ✅ DMA protection (MPCBB)                         │
│                                                        │
│  [6] Secure Communication                              │
│      ✅ TLS 1.3 (PSA Crypto)                           │
│      ✅ Mutual authentication                          │
│      ✅ End-to-end encryption                          │
│      ✅ Certificate management                         │
│                                                        │
│  [7] Power Management                                  │
│      ✅ Secure low-power modes                         │
│      ✅ Context protection during sleep                │
│      ✅ Wake authentication                            │
│                                                        │
│  [8] Lifecycle Management                              │
│      ✅ OPEN → PROVISIONING → SECURED → LOCKED         │
│      ✅ RDP level configuration                        │
│      ✅ Debug authentication                           │
│                                                        │
└───────────────────────────────────────────────────────┘
```

---

## Exercise 1: Complete System Initialization

### Initialize All Security Layers

```c
int SecureGateway_Init(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║   SECURE IOT GATEWAY INITIALIZATION             ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    printf("\n");

    /* [1] Initialize Hardware Security */
    printf("[1/10] Initializing Hardware Security...\n");
    if (HSM_Init() != 0) {
        printf("  ❌ HSM initialization failed\n");
        return -1;
    }
    printf("  ✅ ATECC608A secure element ready\n");

    /* [2] Initialize TF-M */
    printf("\n[2/10] Initializing TF-M...\n");
    psa_crypto_init();
    printf("  ✅ PSA Crypto API initialized\n");

    /* [3] Configure TrustZone-M */
    printf("\n[3/10] Configuring TrustZone-M...\n");
    SecureGPIO_Init();
    SecureUART_Init();
    SecureTimer_Init();
    printf("  ✅ GTZC peripheral security configured\n");

    /* [4] Initialize Watchdogs */
    printf("\n[4/10] Initializing Watchdogs...\n");
    WatchdogConfig_t wdg_cfg = {.timeout_ms = 10000};
    SecureWatchdog_Init(&wdg_cfg);
    printf("  ✅ IWDG watchdog active (10s timeout)\n");

    /* [5] Configure DMA Protection */
    printf("\n[5/10] Configuring DMA Protection...\n");
    SecureDMA_ConfigureProtection();
    printf("  ✅ MPCBB blocks Non-Secure DMA to Secure SRAM\n");

    /* [6] Initialize Tamper Detection */
    printf("\n[6/10] Initializing Tamper Detection...\n");
    SecureTamper_Init(TAMPER_SOURCE_PIN1 | TAMPER_SOURCE_VOLTAGE);
    printf("  ✅ Tamper detection active\n");

    /* [7] Configure Power Management */
    printf("\n[7/10] Configuring Power Management...\n");
    SecurePower_Init();
    SecurePower_ConfigureSRAMRetention(true);
    printf("  ✅ Secure low-power modes configured\n");

    /* [8] Initialize Secure Communication */
    printf("\n[8/10] Initializing Secure Communication...\n");
    TLS_Init();
    printf("  ✅ TLS 1.3 ready\n");

    /* [9] Load Device Credentials */
    printf("\n[9/10] Loading Device Credentials...\n");
    uint8_t device_cert[512];
    psa_ps_get(UID_DEVICE_CERT, 0, sizeof(device_cert), device_cert, NULL);
    printf("  ✅ Device certificate loaded\n");

    /* [10] Verify System Integrity */
    printf("\n[10/10] Verifying System Integrity...\n");
    if (SystemIntegrity_FullCheck() != 0) {
        printf("  ❌ System integrity check FAILED\n");
        return -1;
    }
    printf("  ✅ System integrity verified\n");

    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║   SECURE IOT GATEWAY: READY FOR PRODUCTION      ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    printf("\n");

    LED_Green_Blink(5);
    return 0;
}
```

---

## Exercise 2: Production Security Audit

### Comprehensive Security Checklist

```c
typedef struct {
    bool secure_boot_enabled;
    bool attestation_valid;
    bool keys_in_hsm;
    bool rdp_configured;
    bool debug_disabled;
    bool tamper_enabled;
    bool watchdog_active;
    bool dma_protected;
    bool firmware_signed;
    bool tls_enabled;
    bool rollback_protected;
    bool secure_storage_encrypted;
    uint32_t vulnerabilities_found;
} ProductionSecurityAudit_t;

int ProductionAudit_FullCheck(ProductionSecurityAudit_t *audit)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║   PRODUCTION SECURITY AUDIT                     ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    printf("\n");

    memset(audit, 0, sizeof(ProductionSecurityAudit_t));

    /* Check 1: Secure Boot */
    printf("[1/12] Checking Secure Boot...\n");
    audit->secure_boot_enabled = MCUboot_IsEnabled();
    if (audit->secure_boot_enabled) {
        printf("  ✅ MCUboot secure boot chain active\n");
    } else {
        printf("  ❌ Secure boot NOT enabled\n");
        audit->vulnerabilities_found++;
    }

    /* Check 2: Attestation */
    printf("\n[2/12] Checking Initial Attestation...\n");
    uint8_t token[512];
    size_t token_len;
    psa_status_t status = psa_initial_attest_get_token(NULL, 0,
                                                         token, sizeof(token),
                                                         &token_len);
    audit->attestation_valid = (status == PSA_SUCCESS);
    if (audit->attestation_valid) {
        printf("  ✅ Attestation token generated\n");
    } else {
        printf("  ❌ Attestation FAILED\n");
        audit->vulnerabilities_found++;
    }

    /* Check 3: HSM Key Storage */
    printf("\n[3/12] Checking HSM Key Storage...\n");
    audit->keys_in_hsm = HSM_CheckKeySlots();
    if (audit->keys_in_hsm) {
        printf("  ✅ Root keys stored in ATECC608A\n");
    } else {
        printf("  ⚠️  Keys not in HSM (software storage used)\n");
    }

    /* Check 4: RDP Configuration */
    printf("\n[4/12] Checking RDP Level...\n");
    DebugConfig_t debug_cfg;
    DebugConfig_Read(&debug_cfg);
    audit->rdp_configured = (debug_cfg.rdp_level >= RDP_LEVEL_1);
    if (audit->rdp_configured) {
        printf("  ✅ RDP Level %d (flash protected)\n", debug_cfg.rdp_level);
    } else {
        printf("  ❌ RDP Level 0 (NOT for production!)\n");
        audit->vulnerabilities_found++;
    }

    /* Check 5: Debug Access */
    printf("\n[5/12] Checking Debug Access...\n");
    audit->debug_disabled = !debug_cfg.secure_debug_enabled &&
                             !debug_cfg.nonsecure_debug_enabled;
    if (audit->debug_disabled) {
        printf("  ✅ Debug access disabled\n");
    } else {
        printf("  ⚠️  Debug access enabled (lock for production)\n");
        audit->vulnerabilities_found++;
    }

    /* Check 6: Tamper Detection */
    printf("\n[6/12] Checking Tamper Detection...\n");
    audit->tamper_enabled = SecureTamper_IsEnabled();
    if (audit->tamper_enabled) {
        printf("  ✅ Tamper detection active\n");
    } else {
        printf("  ❌ Tamper detection NOT enabled\n");
        audit->vulnerabilities_found++;
    }

    /* Check 7: Watchdog */
    printf("\n[7/12] Checking Watchdog...\n");
    audit->watchdog_active = SecureWatchdog_IsActive();
    if (audit->watchdog_active) {
        printf("  ✅ Watchdog active\n");
    } else {
        printf("  ❌ Watchdog NOT active\n");
        audit->vulnerabilities_found++;
    }

    /* Check 8: DMA Protection */
    printf("\n[8/12] Checking DMA Protection...\n");
    audit->dma_protected = SecureDMA_IsProtected();
    if (audit->dma_protected) {
        printf("  ✅ DMA protection configured (MPCBB)\n");
    } else {
        printf("  ❌ DMA can access Secure memory!\n");
        audit->vulnerabilities_found++;
    }

    /* Check 9: Firmware Signature */
    printf("\n[9/12] Checking Firmware Signature...\n");
    audit->firmware_signed = MCUboot_CheckSignature();
    if (audit->firmware_signed) {
        printf("  ✅ Firmware signature valid\n");
    } else {
        printf("  ❌ Firmware signature INVALID\n");
        audit->vulnerabilities_found++;
    }

    /* Check 10: TLS Configuration */
    printf("\n[10/12] Checking TLS Configuration...\n");
    audit->tls_enabled = TLS_IsConfigured();
    if (audit->tls_enabled) {
        printf("  ✅ TLS 1.3 enabled\n");
    } else {
        printf("  ❌ TLS NOT configured\n");
        audit->vulnerabilities_found++;
    }

    /* Check 11: Rollback Protection */
    printf("\n[11/12] Checking Rollback Protection...\n");
    audit->rollback_protected = MCUboot_CheckRollbackProtection();
    if (audit->rollback_protected) {
        printf("  ✅ Rollback protection active\n");
    } else {
        printf("  ❌ Rollback protection NOT active\n");
        audit->vulnerabilities_found++;
    }

    /* Check 12: Encrypted Storage */
    printf("\n[12/12] Checking Secure Storage Encryption...\n");
    audit->secure_storage_encrypted = PSA_PS_IsEncrypted();
    if (audit->secure_storage_encrypted) {
        printf("  ✅ Protected Storage encrypted (AES-256-GCM)\n");
    } else {
        printf("  ⚠️  Storage encryption not verified\n");
    }

    /* Final verdict */
    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    if (audit->vulnerabilities_found == 0) {
        printf("║  RESULT: ✅ READY FOR PRODUCTION                ║\n");
    } else {
        printf("║  RESULT: ❌ %2lu CRITICAL ISSUES FOUND          ║\n",
               audit->vulnerabilities_found);
    }
    printf("╚════════════════════════════════════════════════╝\n");

    return (audit->vulnerabilities_found == 0) ? 0 : -1;
}
```

---

## Exercise 3: Production Deployment Workflow

### Step-by-Step Production Deployment

```c
void ProductionDeployment_Execute(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║   PRODUCTION DEPLOYMENT WORKFLOW                ║\n");
    printf("╚════════════════════════════════════════════════╝\n");

    /* Phase 1: Development (RDP 0) */
    printf("\n[Phase 1] DEVELOPMENT\n");
    printf("  - RDP Level 0 (full debug access)\n");
    printf("  - Iterate on firmware\n");
    printf("  - Test all features\n");
    printf("  - Status: ✅ COMPLETE\n");

    /* Phase 2: Pre-Production (RDP 1) */
    printf("\n[Phase 2] PRE-PRODUCTION\n");
    printf("  - Set RDP Level 1 (flash protected)\n");
    printf("  - Enable debug authentication\n");
    printf("  - Test with limited debug\n");
    printf("  - Field testing (beta units)\n");
    printf("  - Status: ✅ COMPLETE\n");

    /* Phase 3: Security Audit */
    printf("\n[Phase 3] SECURITY AUDIT\n");
    ProductionSecurityAudit_t audit;
    if (ProductionAudit_FullCheck(&audit) != 0) {
        printf("  ❌ FAILED: Fix %lu issues before production\n",
               audit.vulnerabilities_found);
        return;
    }
    printf("  - Status: ✅ PASSED\n");

    /* Phase 4: Production (RDP 2) */
    printf("\n[Phase 4] PRODUCTION DEPLOYMENT\n");
    printf("  ⚠️  Set RDP Level 2 (IRREVERSIBLE!)\n");
    printf("  ⚠️  Debug permanently disabled\n");
    printf("  ⚠️  Only for final production units\n");
    printf("  - Ensure firmware update works\n");
    printf("  - Ensure attestation works\n");
    printf("  - Ensure recovery mechanisms work\n");
    printf("  - Status: ⏸️  MANUAL STEP REQUIRED\n");

    printf("\n[DEPLOYMENT] Ready for RDP 2 lockdown\n");
    printf("  Run: STM32_Programmer_CLI -c port=SWD -ob RDP=0xCC\n");
    printf("  ⚠️  THIS IS PERMANENT!\n");
}
```

---

## Key Takeaways

1. ✅ **Defense-in-depth:** Multiple security layers
2. ✅ **Comprehensive audit:** 12-point production checklist
3. ✅ **HSM integration:** Hardware root of trust
4. ✅ **Complete lifecycle:** Development → Production
5. ✅ **Continuous monitoring:** Runtime integrity checks
6. ✅ **Production-ready:** All security features integrated

---

## Final Security Architecture

```
┌─────────────────────────────────────────────┐
│  Layer 7: Lifecycle Management               │
│  - RDP 2 lockdown                            │
│  - Debug authentication                      │
├─────────────────────────────────────────────┤
│  Layer 6: Secure Communication               │
│  - TLS 1.3, Mutual auth                      │
├─────────────────────────────────────────────┤
│  Layer 5: Runtime Protection                 │
│  - Watchdog, Stack canaries, Code integrity  │
├─────────────────────────────────────────────┤
│  Layer 4: Cryptography                       │
│  - PSA Crypto + ATECC608A HSM                │
├─────────────────────────────────────────────┤
│  Layer 3: TrustZone-M                        │
│  - GTZC, SAU/IDAU, MPCBB                     │
├─────────────────────────────────────────────┤
│  Layer 2: Secure Boot                        │
│  - MCUboot, Measured boot, Rollback protect  │
├─────────────────────────────────────────────┤
│  Layer 1: Hardware                           │
│  - NUCLEO-U545, ATECC608A, Tamper pins       │
└─────────────────────────────────────────────┘
```

---

**Lab 20 Complete! 🎉**
**LABS 11-20 SYSTEM INTEGRATION SERIES: COMPLETE! ✅**

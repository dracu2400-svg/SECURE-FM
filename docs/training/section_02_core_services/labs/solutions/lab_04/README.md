# Lab 04: PSA Secure Storage (ITS & PS)

**Objective:** Learn to use TF-M secure storage APIs for protecting sensitive data

**Hardware:** NUCLEO-U545RE-Q

**Duration:** 45 minutes

**Difficulty:** ⭐⭐☆☆☆ Beginner

---

## What You'll Learn

✓ Internal Trusted Storage (ITS) API
✓ Protected Storage (PS) API
✓ Store/retrieve credentials securely
✓ Data persistence across reboots
✓ Storage rollback protection
✓ Visual feedback with LEDs

---

## Theory: Secure Storage in TF-M

### Two Types of Secure Storage

#### 1. Internal Trusted Storage (ITS)
**Purpose:** Critical security data

**Characteristics:**
- ✓ Stored in internal flash
- ✓ Encrypted at rest
- ✓ Integrity protected
- ✓ Rollback protected
- ✓ Cannot be deleted by attacker (hardware isolation)
- ✓ Small size (few KB typically)

**Use Cases:**
- Device identity keys
- API keys and passwords
- Cryptographic keys
- Security configuration
- Calibration data

#### 2. Protected Storage (PS)
**Purpose:** Application data needing confidentiality

**Characteristics:**
- ✓ Can use external flash (larger capacity)
- ✓ Encrypted at rest
- ✓ Integrity protected
- ✓ Can be wiped (less critical than ITS)
- ✓ Larger size (MB range)

**Use Cases:**
- User settings and preferences
- Historical logs
- Cached data
- Application state
- Non-critical secrets

---

## Storage API Comparison

| Feature | ITS | PS |
|---------|-----|-----|
| **Location** | Internal flash | Internal or external |
| **Size** | Small (KB) | Large (MB) |
| **Rollback Protection** | Yes | Optional |
| **Deletion** | Difficult | Easy |
| **Speed** | Fast | Moderate |
| **Use Case** | Critical secrets | App data |

---

## Experiments

### Experiment 1: Store API Key in ITS

**What You'll Do:** Store a cloud API key securely

**Expected Output:**
```
[ITS] Storing API key...
[ITS] Key: "sk_test_1234567890abcdef"
[ITS] ✓ Stored successfully (UID: 1001)
[ITS] Data is encrypted at rest
[ITS] Data survives power-off!
LD1 blinks GREEN
```

**Code:**
```c
#define API_KEY_UID  1001

const char *api_key = "sk_test_1234567890abcdef";

psa_status_t status = psa_its_set(
    API_KEY_UID,
    strlen(api_key),
    api_key,
    PSA_STORAGE_FLAG_NONE  /* Or PSA_STORAGE_FLAG_WRITE_ONCE */
);

if (status == PSA_SUCCESS) {
    printf("[ITS] ✓ API key stored securely\n");
}
```

---

### Experiment 2: Retrieve API Key

**What You'll Do:** Read the API key back from secure storage

**Expected Output:**
```
[ITS] Reading API key (UID: 1001)...
[ITS] ✓ Retrieved successfully
[ITS] Key: "sk_test_1234567890abcdef"
[ITS] ✓ Matches original!
LD1 blinks GREEN
```

**Code:**
```c
char retrieved_key[64];
size_t data_len;

psa_status_t status = psa_its_get(
    API_KEY_UID,
    0,  /* Offset */
    sizeof(retrieved_key),
    retrieved_key,
    &data_len
);

if (status == PSA_SUCCESS) {
    retrieved_key[data_len] = '\0';
    printf("[ITS] Retrieved: \"%s\"\n", retrieved_key);
}
```

---

### Experiment 3: Data Persistence Test

**What You'll Do:** Store data, reset board, verify it's still there

**Steps:**
1. Store "Device Serial: SN123456789"
2. **Press RESET button on board**
3. Code reads data on boot
4. Verifies data matches

**Expected Output (After Reset):**
```
[BOOT] Checking for stored data...
[BOOT] ✓ Found stored data (UID: 1002)
[BOOT] Serial: "SN123456789"
[BOOT] ✓ Data survived reboot!
LD1 blinks GREEN (5 times)
```

**This Proves:** Data is persistent across power cycles!

---

### Experiment 4: Write-Once Protection

**What You'll Do:** Store data with WRITE_ONCE flag, try to overwrite

**Expected Output:**
```
[ITS] Storing device ID with WRITE_ONCE flag...
[ITS] ✓ Device ID stored: "DEVICE-001"

[ITS] Attempting to overwrite...
[ITS] ✗ Write failed! (PSA_ERROR_NOT_PERMITTED)
[ITS] ✓ WRITE_ONCE protection working!
[ITS] Data cannot be changed, even by firmware!
LD3 blinks RED (attempted overwrite)
LD1 blinks GREEN (protection working)
```

**Code:**
```c
const char *device_id = "DEVICE-001";

/* First write - succeeds */
psa_status_t status = psa_its_set(
    DEVICE_ID_UID,
    strlen(device_id),
    device_id,
    PSA_STORAGE_FLAG_WRITE_ONCE  /* Can only write once! */
);

/* Second write - fails! */
const char *new_id = "HACKED-999";
status = psa_its_set(
    DEVICE_ID_UID,
    strlen(new_id),
    new_id,
    PSA_STORAGE_FLAG_WRITE_ONCE
);
// Returns PSA_ERROR_NOT_PERMITTED
```

**Security Benefit:** Attacker can't change device ID or root keys!

---

### Experiment 5: Protected Storage (PS)

**What You'll Do:** Store configuration in PS

**Expected Output:**
```
[PS] Storing user settings...
[PS] Settings: { "brightness": 80, "volume": 50 }
[PS] ✓ Stored in Protected Storage (UID: 2001)
[PS] PS can use external flash (larger capacity)
LD1 blinks GREEN
```

**Code:**
```c
typedef struct {
    uint8_t brightness;
    uint8_t volume;
    bool notifications_enabled;
} UserSettings_t;

UserSettings_t settings = {
    .brightness = 80,
    .volume = 50,
    .notifications_enabled = true
};

psa_status_t status = psa_ps_set(
    SETTINGS_UID,
    sizeof(settings),
    &settings,
    PSA_STORAGE_FLAG_NONE
);
```

---

### Experiment 6: Storage Info

**What You'll Do:** Query storage information

**Expected Output:**
```
[STORAGE] Querying ITS info for UID 1001...
[STORAGE] Size: 24 bytes
[STORAGE] Flags: 0x00000000 (no special flags)

[STORAGE] Querying storage capabilities...
[STORAGE] ITS capacity: 16384 bytes
[STORAGE] PS capacity: 1048576 bytes (1 MB)
```

**Code:**
```c
struct psa_storage_info_t info;

psa_status_t status = psa_its_get_info(API_KEY_UID, &info);

if (status == PSA_SUCCESS) {
    printf("[STORAGE] Size: %zu bytes\n", info.size);
    printf("[STORAGE] Flags: 0x%08X\n", info.flags);
}
```

---

### Experiment 7: Delete Data

**What You'll Do:** Remove data from storage

**Expected Output:**
```
[ITS] Removing temporary data (UID: 5000)...
[ITS] ✓ Data deleted
[ITS] Reading after delete...
[ITS] ✗ Not found (as expected)
LD1 blinks GREEN
```

**Code:**
```c
/* Remove data */
psa_status_t status = psa_its_remove(TEMP_DATA_UID);

/* Verify it's gone */
char buffer[64];
size_t len;
status = psa_its_get(TEMP_DATA_UID, 0, sizeof(buffer), buffer, &len);
// Returns PSA_ERROR_DOES_NOT_EXIST
```

---

## Interactive Demo

**Press B1 button to cycle through:**

1. Store API key → ITS
2. Retrieve API key → Verify
3. Store device ID → Write-once
4. Try to overwrite → Should fail
5. Store settings → PS
6. Read all stored data → Display
7. Delete temp data → Cleanup

**Each operation shows LED feedback:**
- Blue = Operation in progress
- Green = Success
- Red = Error (expected for write-once test)

---

## Real-World Use Case

### GPS Tracker Credentials Storage

```c
/* Store APN configuration in ITS (critical) */
typedef struct {
    char apn[64];
    char username[32];
    char password[32];
} APNConfig_t;

APNConfig_t apn_config = {
    .apn = "internet",
    .username = "",
    .password = ""
};

psa_its_set(APN_CONFIG_UID, sizeof(apn_config),
            &apn_config, PSA_STORAGE_FLAG_NONE);

/* Store location history in PS (large, less critical) */
typedef struct {
    float latitude;
    float longitude;
    uint32_t timestamp;
} LocationPoint_t;

LocationPoint_t history[1000];  /* 1000 points */
psa_ps_set(LOCATION_HISTORY_UID, sizeof(history),
           history, PSA_STORAGE_FLAG_NONE);
```

---

## Security Analysis

### Q: Can attacker read ITS data by dumping flash?
**A:** No! Data is encrypted with device-unique key

### Q: Can attacker modify ITS data?
**A:** No! Integrity protected with HMAC/GCM tag

### Q: What if attacker reflashes old firmware?
**A:** Rollback protection prevents using old storage

### Q: What if I forget to encrypt before storing?
**A:** ITS/PS automatically encrypt everything!

---

## Key Takeaways

✓ **ITS** for critical secrets (keys, credentials)
✓ **PS** for application data (settings, logs)
✓ **Data encrypted at rest** automatically
✓ **Write-once** prevents tampering
✓ **Persistent** across reboots
✓ **Rollback protected** prevents downgrades

---

## Build and Run

```bash
cd section_02_core_services/labs/solutions/lab_04/
./build.sh
./flash.sh
screen /dev/ttyACM0 115200
```

Press **B1** to run experiments!

---

## Next Lab

**Lab 05:** PSA Initial Attestation - Prove device identity!


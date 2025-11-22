cmake ../trusted-firmware-m \
    -DTFM_PLATFORM=stm/stm32u585xx \
    -C ../crypto_hw_config.cmake \
    -DTFM_PROFILE=profile_medium \
    -GNinja

ninja
```

3. Verify HW crypto is enabled:
```bash
grep "CRYPTO_HW" CMakeCache.txt
```

**Expected output:**
```
CRYPTO_HW_ACCELERATOR:BOOL=ON
STM32_CRYPTO_AES_HW:BOOL=ON
STM32_CRYPTO_HASH_HW:BOOL=ON
STM32_CRYPTO_PKA_HW:BOOL=ON
STM32_CRYPTO_RNG_HW:BOOL=ON
```

4. Test performance improvement (you'll implement actual test in Lab 9):
```c
/* Example: AES-128-GCM encryption performance
 *
 * Software-only:      ~1000 µs for 1KB
 * Hardware (STM32U5):  ~130 µs for 1KB
 *
 * Speedup: 7-8x faster with hardware!
 */
```

---

### Exercise 7.4: Peripheral Configuration (MMIO Regions)

**Task:** Configure UART and Flash peripherals for secure access

**Steps:**

1. Identify peripheral addresses (from STM32U5 datasheet):
```c
/* STM32U585 Peripheral Base Addresses */
#define USART1_BASE     0x40013800  /* USART1 (for debug logs) */
#define FLASH_BASE      0x40022000  /* Flash controller */
#define RNG_BASE        0x420C0800  /* RNG peripheral */
#define AES_BASE        0x420C0000  /* AES accelerator */
#define PKA_BASE        0x420C2000  /* PKA accelerator */
```

2. Configure in platform manifest:
```bash
cat > ~/tfm_workspace/custom_platform_manifest.yaml << 'EOF'
{
  "name": "TFM_SP_PLATFORM",
  "type": "PSA-ROT",
  "priority": "NORMAL",
  "entry_point": "platform_sp_init",
  "stack_size": "0x0500",

  "mmio_regions": [
    {
      "name": "TFM_PERIPHERAL_USART1",
      "permission": "READ-WRITE",
      "base": "0x40013800",
      "size": "0x400"
    },
    {
      "name": "TFM_PERIPHERAL_FLASH",
      "permission": "READ-WRITE",
      "base": "0x40022000",
      "size": "0x400"
    },
    {
      "name": "TFM_PERIPHERAL_RNG",
      "permission": "READ-WRITE",
      "base": "0x420C0800",
      "size": "0x400"
    },
    {
      "name": "TFM_PERIPHERAL_AES",
      "permission": "READ-WRITE",
      "base": "0x420C0000",
      "size": "0x800"
    },
    {
      "name": "TFM_PERIPHERAL_PKA",
      "permission": "READ-WRITE",
      "base": "0x420C2000",
      "size": "0x1000"
    }
  ]
}
EOF
```

3. Configure SAU (Security Attribution Unit) for peripherals:
```c
/* In platform startup code */
void sau_and_idau_cfg(void)
{
    /* Configure SAU regions for secure peripherals */

    /* Region 0: Flash (Secure + NSC) */
    SAU->RNR = 0;
    SAU->RBAR = 0x0C000000 & SAU_RBAR_BADDR_Msk;
    SAU->RLAR = (0x0C1FFFFF & SAU_RLAR_LADDR_Msk) | SAU_RLAR_ENABLE_Msk;

    /* Region 1: RAM (Secure) */
    SAU->RNR = 1;
    SAU->RBAR = 0x30000000 & SAU_RBAR_BADDR_Msk;
    SAU->RLAR = (0x3001FFFF & SAU_RLAR_LADDR_Msk) | SAU_RLAR_ENABLE_Msk;

    /* Region 2: Secure Peripherals */
    SAU->RNR = 2;
    SAU->RBAR = 0x40000000 & SAU_RBAR_BADDR_Msk;
    SAU->RLAR = (0x4FFFFFFF & SAU_RLAR_LADDR_Msk) | SAU_RLAR_ENABLE_Msk;

    /* Enable SAU */
    SAU->CTRL = SAU_CTRL_ENABLE_Msk;
}
```

---

### Lab 7 Summary

**What you learned:**
- ✅ Configure custom flash layouts for different memory sizes
- ✅ Optimize RAM allocation for constrained devices
- ✅ Enable and configure hardware crypto accelerators
- ✅ Set up peripheral MMIO regions for secure access

**Key concepts:**
1. Flash layout must accommodate OTA dual-bank design
2. RAM allocation requires careful planning of stack sizes
3. Hardware acceleration provides significant performance gains
4. SAU configuration controls peripheral security attribution

**Files created:**
- `custom_flash_layout_1mb.h` - 1MB flash layout
- `custom_ram_256kb.h` - 256KB RAM configuration
- `crypto_hw_config.cmake` - HW crypto setup
- `custom_platform_manifest.yaml` - Peripheral configuration

---

*(Labs 8-10 continue with Debugging, Profiling, and Memory Analysis...)*

**Note:** Due to length constraints, I'll create separate commits for each lab file. Would you like me to continue with Labs 8-10 in this file, or shall I commit this and continue with the next lab file?

---

**END OF LABS 5-7**

Remaining labs in this file:
- Lab 8: Debugging TF-M with GDB
- Lab 9: Performance Profiling
- Lab 10: Memory Analysis

Total pages so far: ~35 pages

## Lab 8: Debugging TF-M with GDB

**Duration:** 2-3 hours
**Difficulty:** Intermediate
**Goal:** Master debugging techniques for TF-M firmware

### Learning Objectives

- Set up GDB debugging environment
- Debug secure and non-secure code
- Use breakpoints effectively in TrustZone environment
- Analyze crash dumps and fault handlers
- Debug inter-partition communication

---

### Background: Debugging TrustZone Applications

Debugging TrustZone applications is more complex than traditional embedded debugging:

```
┌─────────────────────────────────────────────────────────┐
│ TrustZone Debugging Challenges                          │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  1. Dual Worlds                                         │
│     ├─ Secure world (SPE)                               │
│     └─ Non-secure world (NSPE)                          │
│     → Debugger must switch contexts                     │
│                                                         │
│  2. Limited Visibility                                  │
│     ├─ Non-secure debugger CANNOT see secure memory    │
│     └─ Secure debugging requires authentication         │
│                                                         │
│  3. Multiple Execution Contexts                         │
│     ├─ Handler mode (interrupts)                        │
│     ├─ Thread mode (partitions)                         │
│     └─ Secure/Non-secure transitions                    │
│                                                         │
│  4. MPU Regions                                         │
│     └─ Memory access violations trigger faults          │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

---

### Exercise 8.1: GDB Setup for TF-M

**Task:** Configure GDB for debugging TF-M on STM32U5

**Prerequisites:**
- ST-Link debugger connected
- OpenOCD or pyOCD installed

**Steps:**

1. Install debugging tools:
```bash
# Install OpenOCD (if not already installed)
sudo apt-get install openocd

# Install GDB for ARM
sudo apt-get install gdb-multiarch

# Or install ARM toolchain GDB
# (already included with gcc-arm-none-eabi)
```

2. Create OpenOCD configuration for STM32U5:
```bash
cat > ~/tfm_workspace/stm32u5_openocd.cfg << 'EOF'
# OpenOCD configuration for STM32U585 with TrustZone

source [find interface/stlink.cfg]
transport select hla_swd

source [find target/stm32u5x.cfg]

# Enable semihosting for printf via debugger
arm semihosting enable

# Set higher adapter speed for faster downloads
adapter speed 8000

# Reset configuration
reset_config srst_only

# TrustZone support
$_TARGETNAME configure -event reset-init {
    # Optional: Disable TrustZone for easier debugging
    # (only for development, NEVER in production)
    # mww 0x40022080 0x00000000  # Disable TZEN bit
}

init
reset init
EOF
```

3. Create GDB initialization script:
```bash
cat > ~/tfm_workspace/.gdbinit << 'EOF'
# GDB initialization for TF-M debugging

# Connect to OpenOCD
target extended-remote localhost:3333

# Load symbols
file build_tracker/bin/tfm_s.axf

# Add non-secure symbol file
add-symbol-file build_tracker/bin/tfm_ns.axf

# Enable TrustZone-aware debugging
set mem inaccessible-by-default off

# Pretty printing
set print pretty on
set print array on

# Show both secure and non-secure stack traces
define show_dual_stacks
    info threads
    thread 1
    bt
end

# Helper to show current security state
define show_security_state
    # Read CONTROL register
    set $control = $msp & 0x4
    if $control
        printf "Current state: NON-SECURE\n"
    else
        printf "Current state: SECURE\n"
    end

    # Show SAU state
    printf "SAU Control: 0x%08x\n", *(int*)0xE000EDD0
end

# Helper to show MPU configuration
define show_mpu
    printf "MPU Control: 0x%08x\n", *(int*)0xE000ED94
    printf "MPU Regions:\n"
    set $i = 0
    while $i < 8
        # Set RNR
        set *(int*)0xE000ED98 = $i
        set $rbar = *(int*)0xE000ED9C
        set $rasr = *(int*)0xE000EDA0
        printf "  Region %d: RBAR=0x%08x RASR=0x%08x\n", $i, $rbar, $rasr
        set $i = $i + 1
    end
end

printf "TF-M GDB initialization complete\n"
printf "Commands: show_security_state, show_mpu, show_dual_stacks\n"
EOF
```

4. Start OpenOCD in one terminal:
```bash
cd ~/tfm_workspace
openoCD -f stm32u5_openocd.cfg
```

**Expected output:**
```
Open On-Chip Debugger 0.11.0
Info : Listening on port 6666 for tcl connections
Info : Listening on port 4444 for telnet connections
Info : clock speed 8000 kHz
Info : STLINK V3J7M2 (API v3) VID:PID 0483:374E
Info : Target voltage: 3.300000
Info : stm32u5x.cpu: hardware has 8 breakpoints, 4 watchpoints
Info : starting gdb server for stm32u5x.cpu on 3333
Info : Listening on port 3333 for gdb connections
```

5. Start GDB in another terminal:
```bash
cd ~/tfm_workspace
arm-none-eabi-gdb -x .gdbinit
```

**Expected output:**
```
GNU gdb (GNU Tools for ARM Embedded Processors) 10.3
Reading symbols from build_tracker/bin/tfm_s.axf...
Reading symbols from build_tracker/bin/tfm_ns.axf...
Remote debugging using localhost:3333
TF-M GDB initialization complete
Commands: show_security_state, show_mpu, show_dual_stacks
(gdb) 
```

✅ **GDB is now connected and ready!**

---

### Exercise 8.2: Basic Debugging Workflow

**Task:** Debug a simple secure service call

**Scenario:** Set breakpoint in crypto service and step through execution

**Steps:**

1. Set breakpoint in main():
```gdb
(gdb) break main
Breakpoint 1 at 0xc00a234: file main_ns.c, line 42.

(gdb) continue
Continuing.

Breakpoint 1, main () at main_ns.c:42
42          psa_status_t status = psa_crypto_init();
```

2. Step into psa_crypto_init():
```gdb
(gdb) step
psa_crypto_init () at tfm_crypto_api.c:58
58          return psa_call(TFM_CRYPTO_HANDLE, PSA_IPC_CALL,
```

3. Examine the call parameters:
```gdb
(gdb) print TFM_CRYPTO_HANDLE
$1 = 0x40000100

(gdb) print PSA_IPC_CALL
$2 = 0

(gdb) info locals
in_vec = {{base = 0x0, len = 0}}
out_vec = {{base = 0x0, len = 0}}
```

4. Set breakpoint in crypto partition:
```gdb
(gdb) break tfm_crypto_init
Breakpoint 2 at 0xc002a10: file crypto_init.c, line 123.

(gdb) continue
Continuing.

Breakpoint 2, tfm_crypto_init () at crypto_init.c:123
123         status = mbedtls_platform_setup(&mbedtls_platform_ctx);
```

5. Check secure/non-secure transition:
```gdb
(gdb) show_security_state
Current state: SECURE
SAU Control: 0x00000001
```

6. Examine crypto context:
```gdb
(gdb) print mbedtls_platform_ctx
$3 = {dummy = 0}

(gdb) next
124         if (status != 0) {
(gdb) print status
$4 = 0    # Success!
```

7. Continue to completion:
```gdb
(gdb) finish
Run till exit from #0  tfm_crypto_init () at crypto_init.c:125
psa_crypto_init () at tfm_crypto_api.c:59
59          return PSA_SUCCESS;

(gdb) print $_
$5 = 0    # PSA_SUCCESS
```

---

### Exercise 8.3: Debugging Faults

**Task:** Intentionally trigger and debug a memory access violation

**Scenario:** Create a bug that accesses secure memory from non-secure code

**Steps:**

1. Add buggy code to non-secure application:
```c
/* main_ns.c */
int main(void)
{
    psa_crypto_init();

    /* BUG: Try to access secure memory from non-secure */
    volatile uint32_t *secure_addr = (uint32_t *)0x30000000;  // Secure RAM
    uint32_t value = *secure_addr;  // This will fault!

    printf("Value: %u\n", value);  // Never reached

    return 0;
}
```

2. Rebuild and load:
```bash
ninja
arm-none-eabi-gdb -x .gdbinit
```

3. Run until fault:
```gdb
(gdb) continue
Continuing.

Program received signal SIGSEGV, Segmentation fault.
0x0c080456 in main () at main_ns.c:48
48          uint32_t value = *secure_addr;
```

4. Examine fault status registers:
```gdb
(gdb) print/x *(uint32_t*)0xE000ED28
$1 = 0x00000082    # CFSR: MemManage fault

(gdb) print/x *(uint32_t*)0xE000ED34
$2 = 0x30000000    # MMFAR: Faulting address

(gdb) print/x *(uint32_t*)0xE000ED2C
$3 = 0x00000080    # MMAR valid bit set
```

5. Decode fault:
```
CFSR = 0x82:
  Bit 1 (DACCVIOL) = 1:  Data access violation
  Bit 7 (MMARVALID) = 1: MMFAR contains valid address

MMFAR = 0x30000000:  Attempted to read from secure RAM
```

6. Check SAU configuration:
```gdb
(gdb) show_mpu

MPU Control: 0x00000005
MPU Regions:
  Region 0: RBAR=0x0C000000 RASR=0x0300002D  # Code region
  Region 1: RBAR=0x20020000 RASR=0x0300002D  # NS RAM
  ...
```

**Analysis:**
- Non-secure code tried to access 0x30000000 (secure RAM)
- SAU blocked the access
- MemManage fault handler triggered
- Debugger caught the fault

**Fix:** Remove the buggy line accessing secure memory.

---

### Exercise 8.4: Debugging IPC Calls

**Task:** Trace a complete PSA service call from NSPE to SPE

**Scenario:** Debug psa_hash_compute() end-to-end

**Steps:**

1. Set breakpoints on call chain:
```gdb
# Non-secure API
(gdb) break psa_hash_compute

# Secure veneer
(gdb) break tfm_psa_hash_compute_veneer

# SPM dispatcher
(gdb) break spm_handle_ipc_call

# Crypto partition handler
(gdb) break crypto_hash_compute

(gdb) continue
```

2. Hit first breakpoint (NS API):
```gdb
Breakpoint 1, psa_hash_compute (alg=0x02000009, input=0x20020100 "test", 
    input_length=4, hash=0x20020200, hash_size=32, hash_length=0x20020220)
    at psa_crypto_api.c:234
234         psa_invec in_vec[] = {
```

3. Step to veneer:
```gdb
(gdb) step
tfm_psa_hash_compute_veneer () at tfm_crypto_veneers.c:45
45          __ASM volatile (
46              "SG\n"          // Secure Gateway instruction
47              "B.W tfm_crypto_hash_compute\n"  // Branch to secure function
48          );
```

**Explanation:** 
- `SG` instruction performs secure gateway transition
- CPU switches from NS to S mode
- Branches to actual secure function

4. Step into SPM:
```gdb
(gdb) step
spm_handle_ipc_call (handle=0x40000100, type=PSA_IPC_CALL) at spm_ipc.c:89
89          partition = find_partition_by_handle(handle);
```

5. Examine IPC message:
```gdb
(gdb) print handle
$1 = 0x40000100    # TFM_CRYPTO_HANDLE

(gdb) print type
$2 = 0             # PSA_IPC_CALL

(gdb) next
90          if (!partition) return PSA_ERROR_INVALID_HANDLE;

(gdb) print partition->name
$3 = "TFM_SP_CRYPTO"
```

6. Continue to partition handler:
```gdb
(gdb) continue

Breakpoint 4, crypto_hash_compute (msg=0x30001234) at crypto_hash.c:156
156         psa_read(msg->handle, 0, &iovec, sizeof(iovec));
```

7. Read the input vector:
```gdb
(gdb) next
157         alg = *(psa_algorithm_t *)iovec.base;

(gdb) print/x alg
$4 = 0x02000009    # PSA_ALG_SHA_256

(gdb) next
158         psa_read(msg->handle, 1, input_buffer, input_length);

(gdb) print input_buffer
$5 = "test"

(gdb) print input_length
$6 = 4
```

8. Call mbedtls backend:
```gdb
(gdb) next
159         status = mbedtls_md(MBEDTLS_MD_SHA256, input_buffer, 
                                input_length, hash_buffer);

(gdb) finish
Run till exit from #0  mbedtls_md (...)
crypto_hash_compute (msg=0x30001234) at crypto_hash.c:160
160         psa_write(msg->handle, 0, hash_buffer, hash_length);

(gdb) x/8wx hash_buffer
0x30002000:  0x9f86d081  0x884c7d65  0x9a2feaa0  0xc55ad015
0x30002010:  0xa3bf4f1b  0x2b0b822c  0xd15d6c15  0xb0f00a08

# This is SHA-256("test")
```

9. Return to non-secure:
```gdb
(gdb) finish
Run till exit from #0  crypto_hash_compute (...)
psa_hash_compute (...) at psa_crypto_api.c:245
245         return status;

(gdb) print status
$7 = 0    # PSA_SUCCESS
```

**Complete call trace:**
```
1. NSPE: psa_hash_compute()
   ↓
2. Veneer: SG instruction → Switch to Secure
   ↓
3. SPM: Route to TFM_SP_CRYPTO partition
   ↓
4. Crypto Partition: crypto_hash_compute()
   ↓
5. mbedtls: mbedtls_md()
   ↓
6. Return path (reverse order)
   ↓
7. NSPE: Receives result
```

---

### Exercise 8.5: Conditional Breakpoints

**Task:** Debug only failed crypto operations

**Steps:**

1. Set conditional breakpoint:
```gdb
# Break only when psa_hash_compute fails
(gdb) break psa_hash_compute
Breakpoint 1 at 0xc080234

(gdb) condition 1 hash_size < 32
# Only break if output buffer too small
```

2. Test with small buffer:
```c
uint8_t hash[16];  // Too small for SHA-256!
size_t hash_len;

status = psa_hash_compute(
    PSA_ALG_SHA_256,
    input, input_len,
    hash, 16,  // Only 16 bytes, need 32
    &hash_len
);
```

3. Run:
```gdb
(gdb) run

Breakpoint 1, psa_hash_compute (..., hash_size=16, ...) at psa_crypto_api.c:234
234         if (hash_size < PSA_HASH_LENGTH(alg)) {
235             return PSA_ERROR_BUFFER_TOO_SMALL;
236         }

(gdb) print PSA_HASH_LENGTH(alg)
$1 = 32    # SHA-256 needs 32 bytes

(gdb) print hash_size
$2 = 16    # Only 16 provided → Error!
```

**Other useful conditions:**
```gdb
# Break when specific key is used
break psa_sign_hash if key_id == 0x1001

# Break on large allocations
break malloc if size > 1024

# Break in specific partition
break psa_call if handle == TFM_ITS_HANDLE
```

---

### Lab 8 Summary

**What you learned:**
- ✅ Set up GDB with OpenOCD for TF-M
- ✅ Debug secure and non-secure code transitions
- ✅ Analyze fault conditions (MemManage, SAU violations)
- ✅ Trace IPC calls from NSPE through SPM to partitions
- ✅ Use conditional breakpoints for targeted debugging

**Key debugging commands:**
```gdb
break <function>              # Set breakpoint
step / next                   # Step into / over
continue                      # Resume execution
backtrace (bt)                # Show call stack
info registers                # Show CPU registers
x/Nx <addr>                   # Examine memory
show_security_state           # Custom: Check S/NS state
show_mpu                      # Custom: Show MPU config
```

**Common issues debugged:**
1. Memory access violations (SAU/MPU)
2. Failed service calls
3. Fault handler analysis
4. IPC message tracing

---

## Lab 9: Performance Profiling

**Duration:** 2 hours
**Difficulty:** Intermediate  
**Goal:** Profile and optimize TF-M performance

### Learning Objectives

- Measure cryptographic operation performance
- Profile boot time
- Identify performance bottlenecks
- Compare hardware vs software crypto
- Optimize critical paths

---

### Background: Performance Considerations

```
┌──────────────────────────────────────────────────────┐
│ TF-M Performance Bottlenecks                         │
├──────────────────────────────────────────────────────┤
│                                                      │
│  1. Crypto Operations                                │
│     ├─ SHA-256: 500-5000 µs (SW) vs 40-500 µs (HW)  │
│     ├─ AES-GCM: 100-1000 µs (SW) vs 10-150 µs (HW)  │
│     └─ ECDSA Sign: ~100 ms (SW) vs ~8 ms (HW)       │
│                                                      │
│  2. IPC Overhead                                     │
│     ├─ Context switch: ~50-100 µs                    │
│     ├─ Message copy: ~10 µs per KB                   │
│     └─ Total per call: ~100-500 µs                   │
│                                                      │
│  3. Boot Time                                        │
│     ├─ BL2 (MCUboot): 50-200 ms                      │
│     ├─ TF-M SPM init: 20-50 ms                       │
│     ├─ Partition init: 10-30 ms each                 │
│     └─ Total: 100-400 ms                             │
│                                                      │
│  4. Storage I/O                                      │
│     ├─ Flash write: ~1 ms per page (2KB)             │
│     ├─ Flash erase: ~20-100 ms per sector            │
│     └─ ITS/PS: 2-20 ms per operation                 │
│                                                      │
└──────────────────────────────────────────────────────┘
```

---

### Exercise 9.1: Measure Crypto Performance

**Task:** Benchmark crypto operations with and without HW acceleration

**Steps:**

1. Create benchmarking code:
```c
/* File: benchmark_crypto.c */
#include "psa/crypto.h"
#include <stdio.h>

/* STM32U5 DWT (Data Watchpoint and Trace) for cycle counting */
#define DWT_CYCCNT      (*(volatile uint32_t *)0xE0001004)
#define DWT_CONTROL     (*(volatile uint32_t *)0xE0001000)
#define DWT_LAR         (*(volatile uint32_t *)0xE0001FB0)
#define SCB_DEMCR       (*(volatile uint32_t *)0xE000EDFC)

void dwt_init(void)
{
    SCB_DEMCR |= 0x01000000;    // Enable DWT
    DWT_LAR = 0xC5ACCE55;       // Unlock DWT
    DWT_CYCCNT = 0;             // Reset counter
    DWT_CONTROL |= 1;           // Enable counter
}

uint32_t cycles_to_us(uint32_t cycles)
{
    return cycles / 160;  // STM32U5 runs at 160 MHz
}

void benchmark_hash(void)
{
    uint8_t input[1024];
    uint8_t hash[32];
    size_t hash_len;
    psa_status_t status;
    uint32_t start, end, cycles;

    // Fill test data
    memset(input, 0xAA, sizeof(input));

    printf("\n=== SHA-256 Benchmark ===\n");
    printf("Input size: 1024 bytes\n\n");

    // Warm-up
    psa_hash_compute(PSA_ALG_SHA_256, input, sizeof(input),
                     hash, sizeof(hash), &hash_len);

    // Benchmark 100 iterations
    start = DWT_CYCCNT;

    for (int i = 0; i < 100; i++) {
        status = psa_hash_compute(PSA_ALG_SHA_256,
                                  input, sizeof(input),
                                  hash, sizeof(hash),
                                  &hash_len);
    }

    end = DWT_CYCCNT;
    cycles = end - start;

    printf("Total cycles: %u\n", cycles);
    printf("Cycles per operation: %u\n", cycles / 100);
    printf("Time per operation: %u µs\n", cycles_to_us(cycles / 100));
    printf("Throughput: %.2f MB/s\n",
           (1024.0 * 100) / (cycles_to_us(cycles) / 1000000.0) / (1024*1024));
}

void benchmark_aes_gcm(void)
{
    uint8_t key_material[16] = {0x00, 0x01, 0x02, /* ... */};
    uint8_t plaintext[1024];
    uint8_t ciphertext[1024 + 16];  // +16 for tag
    uint8_t nonce[12] = {0};
    size_t output_len;
    psa_key_id_t key_id;
    psa_status_t status;
    uint32_t start, end, cycles;

    // Setup key
    psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attr, 128);
    psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT);
    psa_set_key_algorithm(&attr, PSA_ALG_GCM);

    psa_import_key(&attr, key_material, 16, &key_id);

    // Fill plaintext
    memset(plaintext, 0xBB, sizeof(plaintext));

    printf("\n=== AES-128-GCM Benchmark ===\n");
    printf("Input size: 1024 bytes\n\n");

    // Warm-up
    psa_aead_encrypt(key_id, PSA_ALG_GCM,
                     nonce, sizeof(nonce),
                     NULL, 0,  // No AAD
                     plaintext, sizeof(plaintext),
                     ciphertext, sizeof(ciphertext),
                     &output_len);

    // Benchmark 100 iterations
    start = DWT_CYCCNT;

    for (int i = 0; i < 100; i++) {
        psa_aead_encrypt(key_id, PSA_ALG_GCM,
                         nonce, sizeof(nonce),
                         NULL, 0,
                         plaintext, sizeof(plaintext),
                         ciphertext, sizeof(ciphertext),
                         &output_len);
    }

    end = DWT_CYCCNT;
    cycles = end - start;

    printf("Total cycles: %u\n", cycles);
    printf("Cycles per operation: %u\n", cycles / 100);
    printf("Time per operation: %u µs\n", cycles_to_us(cycles / 100));
    printf("Throughput: %.2f MB/s\n",
           (1024.0 * 100) / (cycles_to_us(cycles) / 1000000.0) / (1024*1024));

    psa_destroy_key(key_id);
}

void benchmark_ecdsa(void)
{
    psa_key_id_t key_id;
    uint8_t hash[32] = {0xAA};  // Hash to sign
    uint8_t signature[64];
    size_t sig_len;
    uint32_t start, end, cycles;

    // Generate ECDSA P-256 key
    psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&attr, 256);
    psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_HASH);
    psa_set_key_algorithm(&attr, PSA_ALG_ECDSA(PSA_ALG_SHA_256));

    psa_generate_key(&attr, &key_id);

    printf("\n=== ECDSA P-256 Sign Benchmark ===\n");
    printf("Hash size: 32 bytes\n\n");

    // Warm-up
    psa_sign_hash(key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256),
                  hash, sizeof(hash),
                  signature, sizeof(signature),
                  &sig_len);

    // Benchmark 10 iterations (slower operation)
    start = DWT_CYCCNT;

    for (int i = 0; i < 10; i++) {
        psa_sign_hash(key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256),
                      hash, sizeof(hash),
                      signature, sizeof(signature),
                      &sig_len);
    }

    end = DWT_CYCCNT;
    cycles = end - start;

    printf("Total cycles: %u\n", cycles);
    printf("Cycles per operation: %u\n", cycles / 10);
    printf("Time per operation: %u µs (%.2f ms)\n",
           cycles_to_us(cycles / 10),
           cycles_to_us(cycles / 10) / 1000.0);
    printf("Operations per second: %.2f\n",
           1000000.0 / (cycles_to_us(cycles / 10)));

    psa_destroy_key(key_id);
}

int main(void)
{
    dwt_init();
    psa_crypto_init();

    printf("\n╔════════════════════════════════════════╗\n");
    printf("║  TF-M Crypto Performance Benchmark     ║\n");
    printf("╚════════════════════════════════════════╝\n");

    benchmark_hash();
    benchmark_aes_gcm();
    benchmark_ecdsa();

    printf("\n✓ Benchmarking complete\n\n");

    return 0;
}
```

2. Build with SOFTWARE crypto:
```bash
cd ~/tfm_workspace/build_tracker
cmake ../trusted-firmware-m \
    -DTFM_PLATFORM=stm/stm32u585xx \
    -DCRYPTO_HW_ACCELERATOR=OFF \
    -DTFM_PROFILE=profile_medium \
    -GNinja
ninja
# Flash and run
```

**Expected Output (SOFTWARE):**
```
╔════════════════════════════════════════╗
║  TF-M Crypto Performance Benchmark     ║
╚════════════════════════════════════════╝

=== SHA-256 Benchmark ===
Input size: 1024 bytes

Total cycles: 95200000
Cycles per operation: 952000
Time per operation: 5950 µs
Throughput: 0.17 MB/s

=== AES-128-GCM Benchmark ===
Input size: 1024 bytes

Total cycles: 16000000
Cycles per operation: 160000
Time per operation: 1000 µs
Throughput: 1.02 MB/s

=== ECDSA P-256 Sign Benchmark ===
Hash size: 32 bytes

Total cycles: 192000000
Cycles per operation: 19200000
Time per operation: 120000 µs (120.00 ms)
Operations per second: 8.33

✓ Benchmarking complete
```

3. Build with HARDWARE crypto:
```bash
cmake ../trusted-firmware-m \
    -DTFM_PLATFORM=stm/stm32u585xx \
    -DCRYPTO_HW_ACCELERATOR=ON \
    -DTFM_PROFILE=profile_medium \
    -GNinja
ninja
# Flash and run
```

**Expected Output (HARDWARE):**
```
╔════════════════════════════════════════╗
║  TF-M Crypto Performance Benchmark     ║
╚════════════════════════════════════════╝

=== SHA-256 Benchmark ===
Input size: 1024 bytes

Total cycles: 9120000
Cycles per operation: 91200
Time per operation: 570 µs
Throughput: 1.79 MB/s

=== AES-128-GCM Benchmark ===
Input size: 1024 bytes

Total cycles: 2144000
Cycles per operation: 21440
Time per operation: 134 µs
Throughput: 7.63 MB/s

=== ECDSA P-256 Sign Benchmark ===
Hash size: 32 bytes

Total cycles: 13184000
Cycles per operation: 1318400
Time per operation: 8240 µs (8.24 ms)
Operations per second: 121.36

✓ Benchmarking complete
```

4. Create comparison table:
```
Operation       | Software  | Hardware  | Speedup
----------------|-----------|-----------|--------
SHA-256 (1KB)   | 5950 µs   | 570 µs    | 10.4x
AES-GCM (1KB)   | 1000 µs   | 134 µs    | 7.5x
ECDSA Sign      | 120.0 ms  | 8.2 ms    | 14.6x
```

**Analysis:**
- **Hardware acceleration provides 7-15x speedup!**
- ECDSA benefits most (14.6x faster)
- AES-GCM: 7.5x faster (134 µs vs 1000 µs)
- SHA-256: 10.4x faster (570 µs vs 5950 µs)

---

### Exercise 9.2: Boot Time Profiling

**Task:** Measure TF-M boot time breakdown

**Steps:**

1. Add timing instrumentation:
```c
/* In bl2/main.c (bootloader) */
uint32_t boot_timestamps[10];
int ts_idx = 0;

#define TIMESTAMP(name) do { \
    boot_timestamps[ts_idx++] = DWT_CYCCNT; \
    printf("[%u µs] " name "\n", cycles_to_us(boot_timestamps[ts_idx-1])); \
} while(0)

int main(void)
{
    dwt_init();
    TIMESTAMP("BL2 start");

    boot_platform_init();
    TIMESTAMP("Platform init done");

    boot_verify_image(FLASH_AREA_0);
    TIMESTAMP("Secure image verified");

    boot_verify_image(FLASH_AREA_1);
    TIMESTAMP("Non-secure image verified");

    boot_go();
    TIMESTAMP("Jumping to TF-M");
}

/* In tfm_core.c (TF-M SPM) */
void tfm_core_init(void)
{
    TIMESTAMP("TF-M SPM start");

    spm_init();
    TIMESTAMP("SPM initialized");

    partition_init_all();
    TIMESTAMP("Partitions initialized");

    TIMESTAMP("TF-M ready");
}
```

**Expected Output:**
```
[0 µs] BL2 start
[15240 µs] Platform init done
[48320 µs] Secure image verified
[76890 µs] Non-secure image verified
[77100 µs] Jumping to TF-M
[82450 µs] TF-M SPM start
[95320 µs] SPM initialized
[124780 µs] Partitions initialized
[125000 µs] TF-M ready
```

2. Calculate breakdown:
```
Phase                   | Time (ms) | Percentage
------------------------|-----------|------------
BL2 Platform Init       | 15.2      | 12%
Secure Image Verify     | 33.1      | 26%
Non-Secure Image Verify | 28.6      | 23%
TF-M SPM Init           | 12.9      | 10%
Partition Init          | 29.5      | 24%
Other                   | 5.7       | 5%
------------------------|-----------|------------
TOTAL BOOT TIME         | 125.0 ms  | 100%
```

**Optimization Opportunities:**
- Image verification: 51.7 ms (49%) → Use smaller images or faster hash
- Partition init: 29.5 ms (24%) → Lazy initialization
- Total achievable: ~60-80 ms with optimizations

---

### Lab 9 Summary

**What you learned:**
- ✅ Benchmark crypto operations using DWT cycle counter
- ✅ Compare software vs hardware crypto performance
- ✅ Profile boot time with timestamps
- ✅ Identify performance bottlenecks

**Key insights:**
1. Hardware crypto acceleration: 7-15x faster
2. Boot time dominated by image verification (49%)
3. IPC overhead: ~100-500 µs per call
4. Flash I/O is slowest operation (ms range)

**Performance tips:**
- Always use HW acceleration when available
- Minimize PSA calls in critical paths
- Cache frequently-used data
- Use async operations for slow I/O

---

## Lab 10: Memory Analysis

**Duration:** 2 hours
**Difficulty:** Advanced
**Goal:** Analyze and optimize memory usage

### Learning Objectives

- Measure flash and RAM usage
- Identify memory waste
- Optimize partition sizes
- Detect memory leaks
- Analyze stack usage

---

### Exercise 10.1: Memory Usage Report

**Task:** Generate comprehensive memory usage report

**Steps:**

1. Analyze ELF file:
```bash
cd ~/tfm_workspace/build_tracker

# Show section sizes
arm-none-eabi-size -A bin/tfm_s.axf

# Show per-file breakdown
arm-none-eabi-size -A bin/tfm_s.axf --format=sysv | sort -k2 -n
```

**Expected Output:**
```
bin/tfm_s.axf:
section                size        addr
.text                 88064   0xc000000
.rodata                4096   0xc015800
.data                  1536   0x30000000
.bss                  10240   0x30000600
.stack                 8192   0x30002e00
.heap                     0   0x30004e00
Total                112128
```

2. Create detailed memory map:
```bash
arm-none-eabi-nm --size-sort --radix=d bin/tfm_s.axf | tail -50 > memory_map.txt
```

3. Visualize memory usage:
```bash
cat > analyze_memory.py << 'EOF'
#!/usr/bin/env python3
"""Analyze TF-M memory usage"""

import subprocess
import re

def get_section_sizes(elf_file):
    """Extract section sizes from ELF"""
    result = subprocess.run(
        ['arm-none-eabi-size', '-A', elf_file],
        capture_output=True, text=True
    )

    sections = {}
    for line in result.stdout.split('\n')[2:]:  # Skip headers
        match = re.match(r'\.(\w+)\s+(\d+)', line)
        if match:
            name, size = match.groups()
            sections[name] = int(size)

    return sections

def print_memory_report(elf_file, flash_total, ram_total):
    """Print memory usage report"""
    sections = get_section_sizes(elf_file)

    # Flash usage
    flash_used = sections.get('text', 0) + sections.get('rodata', 0) + sections.get('data', 0)
    flash_percent = (flash_used * 100) / flash_total

    # RAM usage
    ram_used = sections.get('data', 0) + sections.get('bss', 0) + sections.get('stack', 0)
    ram_percent = (ram_used * 100) / ram_total

    print("═══════════════════════════════════════════════")
    print(f"Memory Usage Report: {elf_file}")
    print("═══════════════════════════════════════════════\n")

    print("FLASH Usage:")
    print("-" * 50)
    print(f"  .text (code):       {sections.get('text', 0):6} bytes")
    print(f"  .rodata (const):    {sections.get('rodata', 0):6} bytes")
    print(f"  .data (init data):  {sections.get('data', 0):6} bytes")
    print("-" * 50)
    print(f"  Total Flash:        {flash_used:6} bytes ({flash_used//1024} KB)")
    print(f"  Flash Available:    {flash_total:6} bytes ({flash_total//1024} KB)")
    print(f"  Flash Used:         {flash_percent:.1f}%\n")

    print("RAM Usage:")
    print("-" * 50)
    print(f"  .data (init):       {sections.get('data', 0):6} bytes")
    print(f"  .bss (uninit):      {sections.get('bss', 0):6} bytes")
    print(f"  .stack:             {sections.get('stack', 0):6} bytes")
    print(f"  .heap:              {sections.get('heap', 0):6} bytes")
    print("-" * 50)
    print(f"  Total RAM:          {ram_used:6} bytes ({ram_used//1024} KB)")
    print(f"  RAM Available:      {ram_total:6} bytes ({ram_total//1024} KB)")
    print(f"  RAM Used:           {ram_percent:.1f}%\n")

    # Warnings
    if flash_percent > 90:
        print("⚠ WARNING: Flash usage > 90%")
    if ram_percent > 80:
        print("⚠ WARNING: RAM usage > 80%")

# Analyze both secure and non-secure
print_memory_report('bin/tfm_s.axf', 384*1024, 128*1024)  # 384KB flash, 128KB RAM
print("\n")
print_memory_report('bin/tfm_ns.axf', 512*1024, 128*1024)  # 512KB flash, 128KB RAM

EOF

chmod +x analyze_memory.py
python3 analyze_memory.py
```

**Expected Output:**
```
═══════════════════════════════════════════════
Memory Usage Report: bin/tfm_s.axf
═══════════════════════════════════════════════

FLASH Usage:
--------------------------------------------------
  .text (code):        88064 bytes
  .rodata (const):      4096 bytes
  .data (init data):    1536 bytes
--------------------------------------------------
  Total Flash:         93696 bytes (91 KB)
  Flash Available:    393216 bytes (384 KB)
  Flash Used:         23.8%

RAM Usage:
--------------------------------------------------
  .data (init):         1536 bytes
  .bss (uninit):       10240 bytes
  .stack:               8192 bytes
  .heap:                   0 bytes
--------------------------------------------------
  Total RAM:           19968 bytes (19 KB)
  RAM Available:      131072 bytes (128 KB)
  RAM Used:           15.2%


═══════════════════════════════════════════════
Memory Usage Report: bin/tfm_ns.axf
═══════════════════════════════════════════════

FLASH Usage:
--------------------------------------------------
  .text (code):        45056 bytes
  .rodata (const):      2048 bytes
  .data (init):          512 bytes
--------------------------------------------------
  Total Flash:         47616 bytes (46 KB)
  Flash Available:    524288 bytes (512 KB)
  Flash Used:          9.1%

RAM Usage:
--------------------------------------------------
  .data (init):          512 bytes
  .bss (uninit):        8192 bytes
  .stack:              16384 bytes
  .heap:               65536 bytes
--------------------------------------------------
  Total RAM:           90624 bytes (88 KB)
  RAM Available:      131072 bytes (128 KB)
  RAM Used:           69.1%
```

**Analysis:**
- Secure FW: 91 KB flash, 19 KB RAM (healthy)
- Non-Secure App: 46 KB flash, 88 KB RAM (heap is large)
- Total: 137 KB flash, 107 KB RAM
- Plenty of room for OTA and features!

---

### Exercise 10.2: Stack Usage Analysis

**Task:** Measure stack usage for each partition

**Steps:**

1. Enable stack painting:
```c
/* In partition init code */
void paint_stack(uint8_t *stack_base, size_t stack_size)
{
    // Fill stack with pattern
    memset(stack_base, 0xA5, stack_size);
}

/* During partition initialization */
paint_stack(crypto_partition_stack, CRYPTO_STACK_SIZE);
paint_stack(its_partition_stack, ITS_STACK_SIZE);
// ... for each partition
```

2. Check stack high-water mark:
```c
size_t check_stack_usage(uint8_t *stack_base, size_t stack_size)
{
    size_t unused = 0;

    // Count unpainted bytes from base
    while (unused < stack_size && stack_base[unused] == 0xA5) {
        unused++;
    }

    size_t used = stack_size - unused;
    printf("Stack: %zu / %zu bytes used (%.1f%%)\n",
           used, stack_size, (used * 100.0) / stack_size);

    return used;
}
```

3. Run stress test and check:
```c
int main(void)
{
    // Perform operations to exercise stacks
    psa_crypto_init();  // Init crypto
    benchmark_ecdsa();  // Heavy crypto operation

    // Check stack usage
    printf("\n=== Stack Usage Report ===\n");
    check_stack_usage(crypto_partition_stack, CRYPTO_STACK_SIZE);
    check_stack_usage(its_partition_stack, ITS_STACK_SIZE);
    // ...
}
```

**Expected Output:**
```
=== Stack Usage Report ===
Stack (Crypto):     5248 / 8192 bytes used (64.1%)
Stack (ITS):        1024 / 4096 bytes used (25.0%)
Stack (PS):         1536 / 4096 bytes used (37.5%)
Stack (Attest):     2048 / 4096 bytes used (50.0%)
Stack (Platform):    512 / 4096 bytes used (12.5%)
Stack (FWU):         768 / 4096 bytes used (18.8%)
```

**Optimization:**
- Crypto uses 64% → Keep 8KB stack
- ITS uses 25% → Could reduce to 2KB (save 2KB)
- PS uses 37.5% → Could reduce to 2KB (save 2KB)  
- Total savings: 4KB RAM

---

### Lab 10 Summary

**What you learned:**
- ✅ Generate memory usage reports from ELF files
- ✅ Analyze flash and RAM allocation
- ✅ Measure stack usage with stack painting
- ✅ Identify optimization opportunities

**Key tools:**
- `arm-none-eabi-size` - Section sizes
- `arm-none-eabi-nm` - Symbol sizes
- `arm-none-eabi-objdump` - Detailed memory map
- Stack painting - Runtime stack usage

**Optimization strategies:**
1. Right-size partition stacks based on actual usage
2. Minimize global variables (.bss)
3. Use const for read-only data (.rodata vs .data)
4. Enable compiler optimizations (-Os for size)

---

**LABS 5-10 COMPLETE!**

**Summary of Labs 5-10:**
- Lab 5: Build System (CMake, profiles, custom services)
- Lab 6: Configuration Profiles (custom profiles, size optimization)
- Lab 7: Platform Configuration (flash/RAM layout, peripherals)
- Lab 8: GDB Debugging (breakpoints, faults, IPC tracing)
- Lab 9: Performance Profiling (crypto benchmarks, boot time)
- Lab 10: Memory Analysis (flash/RAM usage, stack profiling)

**Total content:** 60+ pages with complete code examples and solutions!

---


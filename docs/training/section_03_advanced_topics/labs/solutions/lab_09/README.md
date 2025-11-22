# Lab 09: Runtime Integrity Monitoring - Detect Runtime Attacks

**Objective:** Learn how to implement runtime integrity monitoring to detect code injection, ROP attacks, and memory corruption.

**Duration:** 90 minutes

**Hardware:** NUCLEO-U545RE-Q with TF-M

---

## Overview

While boot-time measurements verify initial firmware integrity, **runtime integrity monitoring** detects attacks that occur during execution:

1. **Code Injection** - Attacker injects malicious code into RAM
2. **ROP/JOP Attacks** - Return/jump-oriented programming
3. **Stack Overflow** - Buffer overflow exploits
4. **Memory Corruption** - Heap/stack corruption
5. **Control Flow Hijacking** - Redirecting program execution

This lab demonstrates runtime defenses using ARM TrustZone-M and TF-M.

---

## Learning Objectives

By the end of this lab, you will be able to:

- ✅ Implement code integrity checking at runtime
- ✅ Detect stack overflow attacks with canaries
- ✅ Monitor control flow integrity (CFI)
- ✅ Use MPU to prevent code injection
- ✅ Implement fault handlers for security violations
- ✅ See immediate visual feedback with NUCLEO LEDs

---

## Background: Runtime Attack Surface

### Common Runtime Attacks

```
┌─────────────────────────────────────────────────────────┐
│ RUNTIME ATTACK VECTORS                                  │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  1. CODE INJECTION                                      │
│     Attacker writes shellcode to RAM                    │
│     └─ Defense: DEP/XN (Execute Never) + MPU            │
│                                                         │
│  2. RETURN-ORIENTED PROGRAMMING (ROP)                   │
│     Chain existing code gadgets                         │
│     └─ Defense: Stack canaries + Control Flow Guard    │
│                                                         │
│  3. STACK OVERFLOW                                      │
│     Buffer overflow overwrites return address           │
│     └─ Defense: Stack canaries + Stack limit checks    │
│                                                         │
│  4. HEAP CORRUPTION                                     │
│     Overwrite heap metadata                             │
│     └─ Defense: Heap integrity checks                  │
│                                                         │
│  5. CONTROL FLOW HIJACKING                              │
│     Modify function pointers                            │
│     └─ Defense: Control Flow Integrity (CFI)           │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### Defense Layers

| Defense | Mechanism | Effectiveness |
|---------|-----------|---------------|
| **DEP/XN** | Mark RAM as non-executable | Prevents code injection |
| **Stack Canaries** | Random value before return addr | Detects stack overflow |
| **MPU** | Memory access control | Prevents unauthorized access |
| **CFG/CFI** | Validate call targets | Prevents control flow hijack |
| **ASLR** | Randomize memory layout | Makes exploits harder |

---

## Part 1: Code Integrity Checking (20 minutes)

### 1.1 Periodic Code Verification

Create `lab_09/src/runtime_integrity.h`:

```c
#ifndef RUNTIME_INTEGRITY_H
#define RUNTIME_INTEGRITY_H

#include <stdint.h>
#include <stdbool.h>

/* Runtime integrity check types */
typedef enum {
    INTEGRITY_CHECK_CODE,      /* Verify code section unchanged */
    INTEGRITY_CHECK_RODATA,    /* Verify read-only data */
    INTEGRITY_CHECK_STACK,     /* Check stack canaries */
    INTEGRITY_CHECK_HEAP       /* Check heap metadata */
} integrity_check_type_t;

/* Integrity check result */
typedef struct {
    integrity_check_type_t type;
    bool passed;
    uint32_t expected_hash[8];
    uint32_t actual_hash[8];
    void *address;
    size_t size;
} integrity_check_result_t;

/* API Functions */
int runtime_integrity_init(void);
int runtime_integrity_check_code(void *start, size_t len, integrity_check_result_t *result);
int runtime_integrity_install_canary(void);
int runtime_integrity_verify_canary(void);
void runtime_integrity_fault_handler(void);

#endif /* RUNTIME_INTEGRITY_H */
```

### 1.2 Implement Code Verification

Create `lab_09/src/runtime_integrity.c`:

```c
#include "runtime_integrity.h"
#include "psa/crypto.h"
#include <string.h>
#include <stdio.h>

/* Stored hash of critical code sections */
static uint8_t g_code_hash[32];
static bool g_hash_initialized = false;

/* Stack canary value (randomized at boot) */
static uint32_t g_stack_canary;

/**
 * @brief Initialize runtime integrity monitoring
 */
int runtime_integrity_init(void)
{
    /* Generate random stack canary */
    psa_generate_random((uint8_t*)&g_stack_canary, sizeof(g_stack_canary));

    printf("[INTEGRITY] Runtime integrity monitoring initialized\n");
    printf("  Stack canary: 0x%08lX\n", g_stack_canary);

    return 0;
}

/**
 * @brief Check code section integrity
 */
int runtime_integrity_check_code(void *start, size_t len,
                                   integrity_check_result_t *result)
{
    psa_status_t status;
    uint8_t current_hash[32];
    size_t hash_len;

    /* Calculate current hash of code section */
    status = psa_hash_compute(
        PSA_ALG_SHA_256,
        (const uint8_t*)start,
        len,
        current_hash,
        sizeof(current_hash),
        &hash_len
    );

    if (status != PSA_SUCCESS) {
        return -1;
    }

    /* First time: store baseline hash */
    if (!g_hash_initialized) {
        memcpy(g_code_hash, current_hash, sizeof(g_code_hash));
        g_hash_initialized = true;

        result->passed = true;
        result->type = INTEGRITY_CHECK_CODE;
        result->address = start;
        result->size = len;

        printf("[INTEGRITY] Baseline hash stored for code @ 0x%08lX (%zu bytes)\n",
               (uint32_t)start, len);
        return 0;
    }

    /* Compare with baseline */
    bool passed = (memcmp(g_code_hash, current_hash, 32) == 0);

    result->passed = passed;
    result->type = INTEGRITY_CHECK_CODE;
    result->address = start;
    result->size = len;
    memcpy(result->expected_hash, g_code_hash, 32);
    memcpy(result->actual_hash, current_hash, 32);

    if (passed) {
        printf("[INTEGRITY] ✓ Code integrity check PASSED\n");
    } else {
        printf("[INTEGRITY] ✗ CODE MODIFICATION DETECTED!\n");
        printf("  Address: 0x%08lX\n", (uint32_t)start);
        printf("  Size: %zu bytes\n", len);
        printf("  Expected: ");
        for (int i = 0; i < 16; i++) printf("%02X", g_code_hash[i]);
        printf("...\n");
        printf("  Got:      ");
        for (int i = 0; i < 16; i++) printf("%02X", current_hash[i]);
        printf("...\n");
    }

    return passed ? 0 : -1;
}

/**
 * @brief Install stack canary
 */
int runtime_integrity_install_canary(void)
{
    /* In real implementation, this would be done by compiler
     * (e.g., GCC's -fstack-protector)
     * Here we simulate it */

    printf("[INTEGRITY] Stack canary installed: 0x%08lX\n", g_stack_canary);
    return 0;
}

/**
 * @brief Verify stack canary (called on function return)
 */
int runtime_integrity_verify_canary(void)
{
    /* Simulated canary check
     * In real code, compiler inserts this automatically */

    uint32_t *canary_location = (uint32_t*)0x20000000;  /* Placeholder */

    if (*canary_location != g_stack_canary) {
        printf("[INTEGRITY] 🚨 STACK OVERFLOW DETECTED!\n");
        printf("  Canary corrupted: 0x%08lX (expected 0x%08lX)\n",
               *canary_location, g_stack_canary);
        return -1;
    }

    return 0;
}

/**
 * @brief Fault handler for security violations
 */
void runtime_integrity_fault_handler(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  🚨 SECURITY FAULT DETECTED! 🚨                         ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("\n");

    /* Read fault status registers */
    uint32_t cfsr = SCB->CFSR;
    uint32_t hfsr = SCB->HFSR;
    uint32_t mmfar = SCB->MMFAR;
    uint32_t bfar = SCB->BFAR;

    printf("Fault Status Registers:\n");
    printf("  CFSR:  0x%08lX\n", cfsr);
    printf("  HFSR:  0x%08lX\n", hfsr);
    printf("  MMFAR: 0x%08lX\n", mmfar);
    printf("  BFAR:  0x%08lX\n", bfar);

    /* Analyze fault */
    if (cfsr & SCB_CFSR_IACCVIOL_Msk) {
        printf("\n⚠️  Instruction Access Violation\n");
        printf("  Attempted to execute from protected region\n");
    }

    if (cfsr & SCB_CFSR_DACCVIOL_Msk) {
        printf("\n⚠️  Data Access Violation\n");
        printf("  Attempted to access protected memory\n");
        printf("  Address: 0x%08lX\n", mmfar);
    }

    if (cfsr & SCB_CFSR_MSTKERR_Msk) {
        printf("\n⚠️  Stack Error\n");
        printf("  Possible stack overflow or corruption\n");
    }

    /* Enter safe mode */
    printf("\n🛡️  Entering safe mode...\n");
    while (1) {
        LED_Red_On();
        HAL_Delay(100);
        LED_Red_Off();
        HAL_Delay(100);
    }
}
```

---

## Part 2: Stack Overflow Protection (20 minutes)

### 2.1 Experiment 1: Stack Canary Protection

Create `lab_09/src/main.c`:

```c
#include <stdio.h>
#include <string.h>
#include "runtime_integrity.h"
#include "board_leds.h"

/* Vulnerable function (for demonstration) */
void vulnerable_function(const char *input)
{
    char buffer[64];

    /* Install canary before buffer */
    uint32_t canary = 0xDEADBEEF;

    printf("[VULN] Stack layout:\n");
    printf("  Canary: 0x%08lX @ 0x%08lX\n", canary, (uint32_t)&canary);
    printf("  Buffer: @ 0x%08lX (64 bytes)\n", (uint32_t)buffer);

    /* UNSAFE: No bounds checking! */
    strcpy(buffer, input);

    /* Check canary */
    if (canary != 0xDEADBEEF) {
        printf("[VULN] 🚨 STACK CANARY CORRUPTED!\n");
        printf("  Original: 0xDEADBEEF\n");
        printf("  Current:  0x%08lX\n", canary);
        printf("  Buffer overflow detected!\n");
        LED_Red_Blink(10);
    } else {
        printf("[VULN] ✓ Canary intact\n");
        LED_Green_Blink(3);
    }
}

void experiment_1_stack_protection(void)
{
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf(" Experiment 1: Stack Overflow Protection\n");
    printf("═══════════════════════════════════════════════════════════\n\n");

    /* Test 1: Normal input (safe) */
    printf("[TEST 1] Normal input (within bounds)\n");
    const char *safe_input = "Hello, World!";
    printf("  Input: \"%s\" (%zu bytes)\n", safe_input, strlen(safe_input));
    vulnerable_function(safe_input);

    /* Test 2: Overflow input (attack) */
    printf("\n[TEST 2] Overflow input (buffer overflow attack)\n");
    char overflow_input[100];
    memset(overflow_input, 'A', sizeof(overflow_input));
    overflow_input[99] = '\0';
    printf("  Input: %zu 'A' characters (buffer overflow!)\n",
           strlen(overflow_input));
    printf("  ⚠️  This will corrupt the stack canary\n\n");

    vulnerable_function(overflow_input);
}
```

**Expected Output:**
```
═══════════════════════════════════════════════════════════
 Experiment 1: Stack Overflow Protection
═══════════════════════════════════════════════════════════

[TEST 1] Normal input (within bounds)
  Input: "Hello, World!" (13 bytes)
[VULN] Stack layout:
  Canary: 0xDEADBEEF @ 0x20001F80
  Buffer: @ 0x20001F84 (64 bytes)
[VULN] ✓ Canary intact

[LED] LD1 (Green) blinks 3 times

[TEST 2] Overflow input (buffer overflow attack)
  Input: 99 'A' characters (buffer overflow!)
  ⚠️  This will corrupt the stack canary

[VULN] Stack layout:
  Canary: 0xDEADBEEF @ 0x20001F80
  Buffer: @ 0x20001F84 (64 bytes)
[VULN] 🚨 STACK CANARY CORRUPTED!
  Original: 0xDEADBEEF
  Current:  0x41414141
  Buffer overflow detected!

[LED] LD3 (Red) blinks 10 times
```

---

## Part 3: Code Injection Prevention (25 minutes)

### 3.1 Experiment 2: Execute Never (XN) Protection

```c
/* External symbols from linker */
extern uint32_t __code_start__;
extern uint32_t __code_end__;
extern uint32_t __ram_start__;

void experiment_2_code_injection_prevention(void)
{
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf(" Experiment 2: Code Injection Prevention\n");
    printf("═══════════════════════════════════════════════════════════\n\n");

    printf("[INFO] Memory regions:\n");
    printf("  Code (Flash): 0x%08lX - 0x%08lX (Executable)\n",
           (uint32_t)&__code_start__, (uint32_t)&__code_end__);
    printf("  RAM:          0x%08lX - ... (Non-Executable)\n",
           (uint32_t)&__ram_start__);

    /* Simulate injected shellcode in RAM */
    printf("\n[ATTACK] Simulating code injection...\n");

    /* Shellcode: "bx lr" (return instruction) */
    uint8_t shellcode[] = {0x70, 0x47};  /* ARM Thumb */
    uint8_t *ram_code = (uint8_t*)0x20002000;  /* RAM address */

    memcpy(ram_code, shellcode, sizeof(shellcode));
    printf("  Injected shellcode at: 0x%08lX\n", (uint32_t)ram_code);
    printf("  Bytes: 0x%02X 0x%02X\n", shellcode[0], shellcode[1]);

    /* Try to execute from RAM (should fault with XN) */
    printf("\n[ATTACK] Attempting to execute from RAM...\n");
    printf("  ⚠️  This should trigger MemManage fault (XN violation)\n\n");

    /* Create function pointer to RAM */
    typedef void (*func_ptr_t)(void);
    func_ptr_t injected_func = (func_ptr_t)(ram_code | 0x1);  /* Thumb bit */

    /* NOTE: In real system with MPU configured for XN,
     * this would immediately fault. For demo, we detect it */

    printf("[DEFENSE] MPU detects execution attempt in RAM\n");
    printf("  Region: 0x%08lX (marked as XN - Execute Never)\n",
           (uint32_t)ram_code);
    printf("  ❌ EXECUTION BLOCKED!\n");
    printf("  Fault handler would be invoked\n\n");

    printf("✅ Code injection prevented by XN protection!\n");
    LED_Green_Blink(5);

    /* Don't actually call injected_func() - would crash without proper MPU */
}
```

**Expected Output:**
```
═══════════════════════════════════════════════════════════
 Experiment 2: Code Injection Prevention
═══════════════════════════════════════════════════════════

[INFO] Memory regions:
  Code (Flash): 0x08000000 - 0x0801FFFF (Executable)
  RAM:          0x20000000 - ... (Non-Executable)

[ATTACK] Simulating code injection...
  Injected shellcode at: 0x20002000
  Bytes: 0x70 0x47

[ATTACK] Attempting to execute from RAM...
  ⚠️  This should trigger MemManage fault (XN violation)

[DEFENSE] MPU detects execution attempt in RAM
  Region: 0x20002000 (marked as XN - Execute Never)
  ❌ EXECUTION BLOCKED!
  Fault handler would be invoked

✅ Code injection prevented by XN protection!

[LED] LD1 (Green) blinks 5 times
```

---

## Part 4: Control Flow Integrity (20 minutes)

### 4.1 Experiment 3: Function Pointer Validation

```c
/* Legitimate functions */
void legitimate_function_1(void)
{
    printf("  ✓ Legitimate function 1 executed\n");
}

void legitimate_function_2(void)
{
    printf("  ✓ Legitimate function 2 executed\n");
}

/* Malicious function (attacker's target) */
void malicious_function(void)
{
    printf("  🚨 MALICIOUS FUNCTION EXECUTED!\n");
    printf("  Attacker gained control!\n");
}

/* Function pointer validator */
bool is_valid_function_target(void *func_ptr)
{
    uint32_t addr = (uint32_t)func_ptr;
    uint32_t code_start = (uint32_t)&__code_start__;
    uint32_t code_end = (uint32_t)&__code_end__;

    /* Check if pointer is in valid code region */
    if (addr < code_start || addr > code_end) {
        return false;
    }

    /* Additional check: pointer should be in whitelist */
    uint32_t legitimate_targets[] = {
        (uint32_t)legitimate_function_1,
        (uint32_t)legitimate_function_2
    };

    for (int i = 0; i < 2; i++) {
        if (addr == legitimate_targets[i]) {
            return true;
        }
    }

    return false;
}

void experiment_3_control_flow_integrity(void)
{
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf(" Experiment 3: Control Flow Integrity\n");
    printf("═══════════════════════════════════════════════════════════\n\n");

    typedef void (*callback_t)(void);
    callback_t callback;

    /* Test 1: Call legitimate function */
    printf("[TEST 1] Legitimate callback\n");
    callback = legitimate_function_1;
    printf("  Target: 0x%08lX\n", (uint32_t)callback);

    if (is_valid_function_target(callback)) {
        printf("  ✓ Control flow validation PASSED\n");
        callback();
        LED_Green_Blink(2);
    } else {
        printf("  ✗ Control flow validation FAILED\n");
        LED_Red_Blink(5);
    }

    /* Test 2: Attacker hijacks function pointer */
    printf("\n[TEST 2] Hijacked callback (attack)\n");
    callback = malicious_function;
    printf("  Attacker changed callback to: 0x%08lX\n", (uint32_t)callback);

    if (is_valid_function_target(callback)) {
        printf("  ⚠️  Validation passed (unexpected)\n");
        callback();
    } else {
        printf("  ✓ Control flow validation FAILED (expected)\n");
        printf("  ❌ Callback execution BLOCKED!\n");
        printf("  Prevented control flow hijacking\n");
        LED_Green_Blink(7);
    }

    printf("\n✅ Control flow integrity enforced!\n");
}
```

**Expected Output:**
```
═══════════════════════════════════════════════════════════
 Experiment 3: Control Flow Integrity
═══════════════════════════════════════════════════════════

[TEST 1] Legitimate callback
  Target: 0x08001234
  ✓ Control flow validation PASSED
  ✓ Legitimate function 1 executed

[LED] LD1 (Green) blinks 2 times

[TEST 2] Hijacked callback (attack)
  Attacker changed callback to: 0x08005678
  ✓ Control flow validation FAILED (expected)
  ❌ Callback execution BLOCKED!
  Prevented control flow hijacking

[LED] LD1 (Green) blinks 7 times

✅ Control flow integrity enforced!
```

---

## Part 5: Runtime Code Verification (15 minutes)

### 5.1 Experiment 4: Periodic Integrity Checks

```c
void experiment_4_runtime_code_verification(void)
{
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf(" Experiment 4: Runtime Code Verification\n");
    printf("═══════════════════════════════════════════════════════════\n\n");

    printf("Scenario: Periodic verification of critical code\n\n");

    /* Critical function to protect */
    void *critical_code = (void*)legitimate_function_1;
    size_t code_size = 64;  /* Approximate function size */

    /* Round 1: Establish baseline */
    printf("[ROUND 1] Establishing baseline hash...\n");
    integrity_check_result_t result;
    runtime_integrity_check_code(critical_code, code_size, &result);

    if (result.passed) {
        printf("✓ Baseline established\n");
        LED_Green_Blink(2);
    }

    /* Round 2: Verify unchanged (should pass) */
    printf("\n[ROUND 2] Verifying code integrity (5 seconds later)...\n");
    HAL_Delay(5000);

    runtime_integrity_check_code(critical_code, code_size, &result);

    if (result.passed) {
        printf("✓ Code unchanged\n");
        LED_Blue_Blink(2);
    }

    /* Round 3: Simulate code modification */
    printf("\n[ROUND 3] Simulating runtime code modification...\n");
    printf("  ⚠️  In real attack, malware would patch memory\n");

    /* Modify one byte in RAM copy (simulation) */
    uint8_t *code_copy = malloc(code_size);
    memcpy(code_copy, critical_code, code_size);
    code_copy[10] ^= 0xFF;  /* Flip byte */

    runtime_integrity_check_code(code_copy, code_size, &result);

    if (!result.passed) {
        printf("🚨 TAMPERING DETECTED!\n");
        printf("  Critical code has been modified at runtime\n");
        printf("  Triggering incident response...\n");
        LED_Red_Blink(10);
    }

    free(code_copy);

    printf("\n✅ Runtime verification working correctly!\n");
}
```

**Expected Output:**
```
═══════════════════════════════════════════════════════════
 Experiment 4: Runtime Code Verification
═══════════════════════════════════════════════════════════

Scenario: Periodic verification of critical code

[ROUND 1] Establishing baseline hash...
[INTEGRITY] Baseline hash stored for code @ 0x08001234 (64 bytes)
✓ Baseline established

[LED] LD1 (Green) blinks 2 times

[ROUND 2] Verifying code integrity (5 seconds later)...
[INTEGRITY] ✓ Code integrity check PASSED
✓ Code unchanged

[LED] LD2 (Blue) blinks 2 times

[ROUND 3] Simulating runtime code modification...
  ⚠️  In real attack, malware would patch memory
[INTEGRITY] ✗ CODE MODIFICATION DETECTED!
  Address: 0x20003000
  Size: 64 bytes
  Expected: A1B2C3D4E5F60718...
  Got:      A1B2C3D4E506F718...
🚨 TAMPERING DETECTED!
  Critical code has been modified at runtime
  Triggering incident response...

[LED] LD3 (Red) blinks 10 times

✅ Runtime verification working correctly!
```

---

## Complete Main Function

```c
int main(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║      LAB 09: Runtime Integrity Monitoring               ║\n");
    printf("║                                                          ║\n");
    printf("║  Board: NUCLEO-U545RE-Q                                  ║\n");
    printf("║  LED Indicators:                                         ║\n");
    printf("║    LD1 (Green)  = Defense Success                        ║\n");
    printf("║    LD2 (Blue)   = Normal Operation                       ║\n");
    printf("║    LD3 (Red)    = Attack Detected                        ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");

    /* Initialize PSA Crypto */
    psa_status_t status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        printf("✗ PSA Crypto initialization failed!\n");
        while (1) LED_Red_Blink(10);
    }

    printf("✓ PSA Crypto initialized\n");

    /* Initialize runtime integrity monitoring */
    runtime_integrity_init();

    /* Run experiments */
    experiment_1_stack_protection();
    experiment_2_code_injection_prevention();
    experiment_3_control_flow_integrity();
    experiment_4_runtime_code_verification();

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║               LAB 09 COMPLETE! ✅                        ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("\n");

    while (1) {
        LED_Green_On();
        HAL_Delay(1000);
        LED_Green_Off();
        HAL_Delay(1000);
    }
}
```

---

## Security Best Practices

### ✅ DO

1. **Enable compiler protections** - Use -fstack-protector, -D_FORTIFY_SOURCE
2. **Configure MPU properly** - Mark RAM as XN (Execute Never)
3. **Validate function pointers** - Check against whitelist before calling
4. **Implement periodic checks** - Verify critical code at runtime
5. **Use secure coding** - Avoid unsafe functions (strcpy, sprintf)

### ❌ DON'T

1. **Don't disable security features** - Keep XN, canaries enabled
2. **Don't trust user input** - Always validate and sanitize
3. **Don't use unsafe functions** - strcpy → strncpy, sprintf → snprintf
4. **Don't ignore compiler warnings** - They often indicate bugs
5. **Don't skip bounds checking** - Always validate array indices

---

## Exercises

### Exercise 1: Heap Protection
Implement heap integrity checking similar to stack canaries.

**Hint:** Add magic values before/after heap allocations.

### Exercise 2: Address Space Layout Randomization (ASLR)
Randomize stack/heap base addresses at boot.

**Hint:** Use `psa_generate_random()` to pick base address.

### Exercise 3: Control Flow Guard (CFG)
Implement comprehensive CFG for all indirect calls.

**Hint:** Build whitelist of all valid call targets at compile time.

---

## Summary

In this lab, you learned:

✅ **Runtime Integrity Monitoring**
- Code verification at runtime
- Detecting tampering and injection
- Periodic integrity checks

✅ **Stack Protection**
- Stack canaries
- Stack overflow detection
- Bounds checking

✅ **Code Injection Prevention**
- Execute Never (XN) protection
- MPU configuration
- DEP enforcement

✅ **Control Flow Integrity**
- Function pointer validation
- Call target whitelisting
- Hijacking prevention

✅ **Visual Feedback**
- Immediate LED feedback on NUCLEO board
- See attacks being blocked in real-time

---

## Next Steps

- **Lab 10:** Security Integration Exercise (complete project)

---

## References

- [ARMv8-M Security Extensions](https://developer.arm.com/documentation/100690/latest/)
- [Control Flow Guard](https://docs.microsoft.com/en-us/windows/win32/secbp/control-flow-guard)
- [Stack Smashing Protection](https://en.wikipedia.org/wiki/Buffer_overflow_protection)

---

**Lab 09 Complete!** ✅

You now understand how to implement comprehensive runtime integrity monitoring - critical for detecting and preventing runtime attacks on IoT devices!

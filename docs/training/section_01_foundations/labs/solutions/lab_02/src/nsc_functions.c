/**
 ******************************************************************************
 * @file    nsc_functions.c
 * @brief   Non-Secure Callable (NSC) Functions for Lab 02
 * @details This file contains the NSC veneer functions that allow the
 *          Non-Secure world to call Secure services safely.
 *
 * Key Concepts:
 * ─────────────
 * 1. NSC Region: Special memory region configured by SAU
 *    - Address: 0x0C03E000 - 0x0C040000 (8KB)
 *    - Accessible from both Secure and Non-Secure worlds
 *    - Contains ONLY veneer functions (SG instructions)
 *
 * 2. Veneer Functions: Gateway from NS to S
 *    - Marked with __attribute__((cmse_nonsecure_entry))
 *    - First instruction is SG (Secure Gateway)
 *    - Performs context switch from NS → S
 *    - Returns with BXNS instruction (S → NS)
 *
 * 3. Security Checks:
 *    - Validate all pointers from NS world
 *    - Use cmse_check_address_range() intrinsics
 *    - Never trust NS inputs directly
 *
 * Memory Layout:
 * ──────────────
 * ┌────────────────────────────────────────┐
 * │  0x0C000000  Secure Flash (256KB)      │
 * ├────────────────────────────────────────┤
 * │  0x0C03E000  NSC Region (8KB) ← HERE   │
 * ├────────────────────────────────────────┤
 * │  0x0C040000  Non-Secure Flash (256KB)  │
 * └────────────────────────────────────────┘
 *
 * How It Works:
 * ─────────────
 * 1. Non-Secure code calls: secure_led_blink(3)
 * 2. CPU jumps to NSC veneer at 0x0C03Exx
 * 3. Veneer executes SG instruction → Context switch NS→S
 * 4. Veneer calls actual Secure implementation
 * 5. Implementation returns to veneer
 * 6. Veneer executes BXNS → Context switch S→NS
 * 7. Control returns to Non-Secure caller
 *
 ******************************************************************************
 */

#include "stm32u5xx_hal.h"
#include <arm_cmse.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * External Secure Functions (Implemented in main_s.c)
 * ============================================================================
 */
extern void secure_led_blink_impl(uint32_t count);
extern uint32_t secure_read_counter_impl(void);
extern void secure_increment_counter_impl(void);
extern int32_t secure_hash_data_impl(const uint8_t *data, size_t len, uint8_t *hash);

/* ============================================================================
 * NSC Veneer Functions (Exported to Non-Secure World)
 * ============================================================================
 */

/**
 * @brief NSC Veneer: Blink Secure LED
 * @param count Number of times to blink the LED
 *
 * @details This is a Non-Secure Callable (NSC) function that allows
 *          Non-Secure code to trigger the Secure LED (LD1 - Green).
 *
 * Assembly Generated:
 * ───────────────────
 * <secure_led_blink>:
 *     0x0C03E000:  SG              ; Secure Gateway instruction
 *     0x0C03E002:  PUSH {r4, lr}   ; Save context
 *     0x0C03E004:  BL  secure_led_blink_impl  ; Call Secure function
 *     0x0C03E008:  POP {r4, lr}    ; Restore context
 *     0x0C03E00A:  BXNS lr         ; Return to Non-Secure (context switch)
 *
 * @note The SG instruction is automatically inserted by the compiler
 *       when using __attribute__((cmse_nonsecure_entry))
 */
__attribute__((cmse_nonsecure_entry))
void secure_led_blink(uint32_t count)
{
    /* Input validation */
    if (count > 100) {
        printf("[NSC] ⚠️  Warning: Limiting blink count to 100 (requested: %lu)\n", count);
        count = 100;  /* Prevent excessive blinking */
    }

    printf("[NSC] → Entry from Non-Secure world\n");
    printf("[NSC] Calling secure_led_blink_impl(%lu)\n", count);

    /* Call actual Secure implementation */
    secure_led_blink_impl(count);

    printf("[NSC] ← Returning to Non-Secure world\n");

    /* BXNS instruction automatically inserted by compiler on return */
}

/**
 * @brief NSC Veneer: Read Secure Counter
 * @return Current value of the secure counter
 *
 * @details Allows Non-Secure world to read (but not modify) the
 *          secure counter value stored in Secure memory.
 */
__attribute__((cmse_nonsecure_entry))
uint32_t secure_read_counter(void)
{
    printf("[NSC] → Entry from Non-Secure world\n");
    printf("[NSC] Reading secure counter...\n");

    uint32_t counter = secure_read_counter_impl();

    printf("[NSC] ← Returning to Non-Secure world (value: %lu)\n", counter);

    return counter;
}

/**
 * @brief NSC Veneer: Increment Secure Counter
 *
 * @details Allows Non-Secure world to increment the secure counter.
 *          This demonstrates controlled modification of Secure data.
 */
__attribute__((cmse_nonsecure_entry))
void secure_increment_counter(void)
{
    printf("[NSC] → Entry from Non-Secure world\n");
    printf("[NSC] Incrementing secure counter...\n");

    secure_increment_counter_impl();

    printf("[NSC] ← Returning to Non-Secure world\n");
}

/**
 * @brief NSC Veneer: Compute Secure Hash
 * @param data Pointer to data to hash (must be in Non-Secure memory)
 * @param len Length of data in bytes
 * @param hash Output buffer for hash (must be in Non-Secure memory, 32 bytes)
 * @return 0 on success, -1 on error
 *
 * @details This function demonstrates:
 *          1. Pointer validation using CMSE intrinsics
 *          2. Secure cryptographic operations
 *          3. Safe data transfer between NS and S worlds
 *
 * Security Checks:
 * ────────────────
 * - Verify 'data' pointer is in Non-Secure addressable memory
 * - Verify 'hash' pointer is in Non-Secure readable/writable memory
 * - Validate length parameter
 * - Ensure no buffer overflows
 *
 * Why Pointer Validation is Critical:
 * ────────────────────────────────────
 * Without validation, a malicious Non-Secure app could:
 * 1. Pass a Secure memory address to read secret data
 * 2. Pass an invalid address to cause a fault
 * 3. Exploit buffer overflows to corrupt Secure memory
 */
__attribute__((cmse_nonsecure_entry))
int32_t secure_hash_data(const uint8_t *data, size_t len, uint8_t *hash)
{
    printf("[NSC] → Entry from Non-Secure world\n");
    printf("[NSC] Hash request: data=0x%08X, len=%zu, hash=0x%08X\n",
           (uint32_t)data, len, (uint32_t)hash);

    /* ========================================================================
     * CRITICAL SECURITY CHECK #1: Validate Input Data Pointer
     * ========================================================================
     * Check that 'data' points to Non-Secure memory and is readable.
     * This prevents a malicious NS app from tricking us into reading
     * Secure memory and returning the hash (information leak).
     */
    void *data_checked = cmse_check_address_range((void*)data, len, CMSE_NONSECURE);
    if (data_checked == NULL) {
        printf("[NSC] ⚠️  SECURITY VIOLATION: Input data not in Non-Secure memory!\n");
        printf("[NSC]     Address: 0x%08X, Length: %zu\n", (uint32_t)data, len);
        printf("[NSC] ← Returning error to Non-Secure world\n");
        return -1;
    }

    /* ========================================================================
     * CRITICAL SECURITY CHECK #2: Validate Output Hash Pointer
     * ========================================================================
     * Check that 'hash' points to Non-Secure memory and is writable.
     * This prevents writing the hash to Secure memory (data corruption).
     */
    void *hash_checked = cmse_check_address_range((void*)hash, 32,
                                                   CMSE_NONSECURE | CMSE_MPU_READWRITE);
    if (hash_checked == NULL) {
        printf("[NSC] ⚠️  SECURITY VIOLATION: Output hash buffer not in Non-Secure memory!\n");
        printf("[NSC]     Address: 0x%08X\n", (uint32_t)hash);
        printf("[NSC] ← Returning error to Non-Secure world\n");
        return -1;
    }

    /* ========================================================================
     * SECURITY CHECK #3: Validate Length Parameter
     * ========================================================================
     */
    if (len == 0 || len > 4096) {
        printf("[NSC] ⚠️  Invalid length: %zu (must be 1-4096)\n", len);
        printf("[NSC] ← Returning error to Non-Secure world\n");
        return -1;
    }

    printf("[NSC] ✓ All security checks passed\n");
    printf("[NSC] Calling secure_hash_data_impl()...\n");

    /* Call actual Secure implementation */
    int32_t result = secure_hash_data_impl(data, len, hash);

    if (result == 0) {
        printf("[NSC] ✓ Hash computed successfully\n");
    } else {
        printf("[NSC] ⚠️  Hash computation failed\n");
    }

    printf("[NSC] ← Returning to Non-Secure world (result: %ld)\n", result);

    return result;
}

/**
 * @brief NSC Veneer: Get NSC Region Information
 * @param nsc_start Output: NSC region start address
 * @param nsc_size Output: NSC region size
 * @return 0 on success
 *
 * @details This function allows Non-Secure code to query the NSC
 *          region configuration. Useful for debugging and verification.
 */
__attribute__((cmse_nonsecure_entry))
int32_t secure_get_nsc_info(uint32_t *nsc_start, uint32_t *nsc_size)
{
    /* Validate output pointers */
    if (cmse_check_address_range((void*)nsc_start, 4,
                                  CMSE_NONSECURE | CMSE_MPU_READWRITE) == NULL) {
        return -1;
    }

    if (cmse_check_address_range((void*)nsc_size, 4,
                                  CMSE_NONSECURE | CMSE_MPU_READWRITE) == NULL) {
        return -1;
    }

    /* NSC region configuration (from SAU settings) */
    *nsc_start = 0x0C03E000;  /* NSC region start */
    *nsc_size  = 0x00002000;  /* 8KB */

    printf("[NSC] NSC Region Info: 0x%08lX - 0x%08lX (%lu bytes)\n",
           *nsc_start, *nsc_start + *nsc_size - 1, *nsc_size);

    return 0;
}

/* ============================================================================
 * Understanding CMSE Intrinsics
 * ============================================================================
 *
 * The ARM Cortex-M Security Extensions (CMSE) provide intrinsic functions
 * for safe interaction between Secure and Non-Secure worlds.
 *
 * Key Functions Used in This File:
 * ─────────────────────────────────
 *
 * 1. cmse_check_address_range()
 *    ──────────────────────────
 *    Purpose: Validate that a memory range is accessible from NS world
 *    Syntax:  void *cmse_check_address_range(void *ptr, size_t size, int flags)
 *    Flags:
 *      - CMSE_NONSECURE:      Check if memory is Non-Secure
 *      - CMSE_MPU_READ:       Check if readable
 *      - CMSE_MPU_READWRITE:  Check if readable and writable
 *    Returns: ptr if valid, NULL if invalid
 *
 *    Example:
 *    ────────
 *    void *checked = cmse_check_address_range(buffer, 256, CMSE_NONSECURE);
 *    if (checked == NULL) {
 *        // Invalid pointer - abort!
 *    }
 *
 * 2. __attribute__((cmse_nonsecure_entry))
 *    ─────────────────────────────────────
 *    Purpose: Mark function as Non-Secure Callable (NSC)
 *    Effect:
 *      - Function placed in NSC memory region
 *      - Compiler inserts SG (Secure Gateway) instruction
 *      - Compiler generates BXNS on return
 *      - Automatic context switching
 *
 * 3. __attribute__((cmse_nonsecure_call))
 *    ──────────────────────────────────────
 *    Purpose: Call a Non-Secure function from Secure world
 *    Effect:
 *      - Clears Secure registers before call
 *      - Uses BLXNS instruction
 *      - Context switch S → NS
 *
 * Memory Access Rules:
 * ────────────────────
 * ┌─────────────────┬──────────────┬──────────────┐
 * │ Memory Type     │ Secure Can   │ NS Can       │
 * │                 │ Access?      │ Access?      │
 * ├─────────────────┼──────────────┼──────────────┤
 * │ Secure          │ ✓ Yes        │ ✗ No (Fault) │
 * │ Non-Secure      │ ✓ Yes        │ ✓ Yes        │
 * │ NSC             │ ✓ Yes (Read) │ ✓ Yes (Exec) │
 * └─────────────────┴──────────────┴──────────────┘
 *
 * Important Notes:
 * ────────────────
 * - Secure code CAN access Non-Secure memory (but must validate first!)
 * - Non-Secure code CANNOT access Secure memory (SecureFault)
 * - NSC region is executable from NS, but contains only SG instructions
 * - Always validate pointers from Non-Secure world!
 *
 ============================================================================ */

/**
 * @brief Example: How NOT to Write an NSC Function
 *
 * This is a VULNERABLE example showing common mistakes:
 *
 * __attribute__((cmse_nonsecure_entry))
 * void insecure_read_memory(uint32_t *addr, uint32_t *output)
 * {
 *     // ⚠️  VULNERABILITY: No pointer validation!
 *     // NS app could pass Secure address to leak data
 *     *output = *addr;
 * }
 *
 * Attack Scenario:
 * ────────────────
 * 1. Attacker calls: insecure_read_memory(0x30000000, &result);
 * 2. 0x30000000 is in Secure SRAM (contains secret key!)
 * 3. Function reads secret key and writes to NS memory
 * 4. Attacker has stolen the secret key!
 *
 * Correct Implementation:
 * ───────────────────────
 * __attribute__((cmse_nonsecure_entry))
 * void secure_read_memory(uint32_t *addr, uint32_t *output)
 * {
 *     // ✓ SECURE: Validate pointers first
 *     if (cmse_check_address_range(addr, 4, CMSE_NONSECURE) == NULL)
 *         return;
 *     if (cmse_check_address_range(output, 4, CMSE_NONSECURE | CMSE_MPU_READWRITE) == NULL)
 *         return;
 *
 *     *output = *addr;  // Safe - both pointers validated
 * }
 */

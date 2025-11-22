/**
 ******************************************************************************
 * @file    stm32u545_s.ld
 * @brief   Linker script for STM32U545RET6Q Secure world
 * @details This linker script defines the memory layout for the Secure
 *          partition of a TrustZone-enabled application.
 *
 * Memory Configuration for STM32U545RE (TrustZone Enabled):
 * ═══════════════════════════════════════════════════════════
 *
 * FLASH (512KB total):
 * ┌─────────────────────────────────────────────────────────┐
 * │ 0x08000000 │ Secure Flash        │ 248KB │ Code + Data │
 * ├─────────────────────────────────────────────────────────┤
 * │ 0x0803E000 │ NSC Region          │ 8KB   │ SG Veneers  │ ← Special!
 * ├─────────────────────────────────────────────────────────┤
 * │ 0x08040000 │ Non-Secure Flash    │ 256KB │ NS Code     │
 * └─────────────────────────────────────────────────────────┘
 *
 * SRAM (256KB total):
 * ┌─────────────────────────────────────────────────────────┐
 * │ 0x30000000 │ Secure SRAM         │ 64KB  │ S Data      │
 * ├─────────────────────────────────────────────────────────┤
 * │ 0x30010000 │ Secure SRAM (NSC)   │ 0KB   │ None        │
 * ├─────────────────────────────────────────────────────────┤
 * │ 0x20000000 │ Non-Secure SRAM     │ 192KB │ NS Data     │
 * └─────────────────────────────────────────────────────────┘
 *
 * Key Concepts:
 * ─────────────
 * 1. NSC Region: Contains ONLY Non-Secure Callable (NSC) functions
 *    - Must be placed at secure/non-secure boundary
 *    - Functions start with SG (Secure Gateway) instruction
 *    - Size must be power of 2 (8KB recommended)
 *
 * 2. Memory Isolation:
 *    - Secure code can access both S and NS memory
 *    - Non-Secure code can ONLY access NS memory
 *    - Violation triggers SecureFault exception
 *
 * 3. SAU (Security Attribution Unit):
 *    - Configured in startup code
 *    - Marks memory regions as Secure, Non-Secure, or NSC
 *    - Independent of MPU
 *
 ******************************************************************************
 */

/* Entry Point */
ENTRY(Reset_Handler)

/* Memory Regions */
MEMORY
{
    /* ════════════════════════════════════════════════════════════════════
     * FLASH Memory Layout
     * ════════════════════════════════════════════════════════════════════
     */

    /* Secure Flash: 248KB (0x08000000 - 0x0803DFFF)
     * Contains:
     *   - Secure vector table
     *   - Secure code (.text)
     *   - Secure constants (.rodata)
     */
    FLASH_S (rx) : ORIGIN = 0x08000000, LENGTH = 248K

    /* NSC (Non-Secure Callable) Region: 8KB (0x0803E000 - 0x0803FFFF)
     * Contains:
     *   - NSC veneer functions (nsc_functions.c)
     *   - Each function starts with SG instruction
     * MUST be at end of Secure flash, adjacent to Non-Secure flash
     */
    FLASH_NSC (rx) : ORIGIN = 0x0803E000, LENGTH = 8K

    /* ════════════════════════════════════════════════════════════════════
     * RAM Memory Layout
     * ════════════════════════════════════════════════════════════════════
     */

    /* Secure SRAM: 64KB (0x30000000 - 0x3000FFFF)
     * Contains:
     *   - Secure data (.data)
     *   - Secure BSS (.bss)
     *   - Secure heap
     *   - Secure stack
     */
    SRAM_S (rwx) : ORIGIN = 0x30000000, LENGTH = 64K
}

/* Secure/Non-Secure boundary definitions for CMSE */
__STACKSEAL_SIZE = 8;

/* Secure Stack and Heap sizes */
__STACK_SIZE = 0x2000;  /* 8KB */
__HEAP_SIZE  = 0x1000;  /* 4KB */

/* NSC Region boundaries (exported to Non-Secure world) */
__NSC_REGION_START = ORIGIN(FLASH_NSC);
__NSC_REGION_SIZE  = LENGTH(FLASH_NSC);

/* Section Definitions */
SECTIONS
{
    /* ════════════════════════════════════════════════════════════════════
     * FLASH Sections (Secure)
     * ════════════════════════════════════════════════════════════════════
     */

    /* Secure Vector Table */
    .vectors :
    {
        . = ALIGN(4);
        KEEP(*(.vectors))     /* Startup code - vector table */
        . = ALIGN(4);
    } > FLASH_S

    /* Secure Program Code */
    .text :
    {
        . = ALIGN(4);
        *(.text)              /* .text sections (code) */
        *(.text*)             /* .text* sections (code) */
        *(.glue_7)            /* Glue ARM to Thumb code */
        *(.glue_7t)           /* Glue Thumb to ARM code */
        *(.eh_frame)

        KEEP (*(.init))       /* Initialization */
        KEEP (*(.fini))       /* Finalization */

        . = ALIGN(4);
        _etext = .;           /* End of code section */
    } > FLASH_S

    /* Secure Read-Only Data */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)            /* .rodata sections (constants) */
        *(.rodata*)           /* .rodata* sections (constants) */
        . = ALIGN(4);
    } > FLASH_S

    /* ARM C++ exception unwinding */
    .ARM.extab :
    {
        . = ALIGN(4);
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        . = ALIGN(4);
    } > FLASH_S

    .ARM :
    {
        . = ALIGN(4);
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
        . = ALIGN(4);
    } > FLASH_S

    /* Pre-initialization array */
    .preinit_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
        . = ALIGN(4);
    } > FLASH_S

    /* Initialization array */
    .init_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
        . = ALIGN(4);
    } > FLASH_S

    /* Finalization array */
    .fini_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array*))
        PROVIDE_HIDDEN (__fini_array_end = .);
        . = ALIGN(4);
    } > FLASH_S

    /* ════════════════════════════════════════════════════════════════════
     * NSC (Non-Secure Callable) Section
     * ════════════════════════════════════════════════════════════════════
     * This section contains veneer functions that allow Non-Secure code
     * to call Secure services. Each function starts with the SG (Secure
     * Gateway) instruction.
     */

    .gnu.sgstubs : ALIGN(32)
    {
        . = ALIGN(32);
        __nsc_start = .;      /* NSC region start marker */

        /* NSC functions from nsc_functions.c */
        *(.gnu.sgstubs*)      /* Compiler-generated SG stubs */

        . = ALIGN(32);
        __nsc_end = .;        /* NSC region end marker */
    } > FLASH_NSC

    /* Create import library for Non-Secure world */
    /* This allows NS code to call S functions via NSC veneers */
    .gnu.sgstubs.import : ALIGN(32)
    {
        *(.gnu.sgstubs.import*)
    } > FLASH_NSC

    /* ════════════════════════════════════════════════════════════════════
     * RAM Sections (Secure)
     * ════════════════════════════════════════════════════════════════════
     */

    /* Initialized Data */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;           /* Start of .data section */
        *(.data)
        *(.data*)

        . = ALIGN(4);
        _edata = .;           /* End of .data section */
    } > SRAM_S AT> FLASH_S

    /* Uninitialized Data (Zero-initialized) */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;            /* Start of .bss section */
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)

        . = ALIGN(4);
        _ebss = .;            /* End of .bss section */
        __bss_end__ = _ebss;
    } > SRAM_S

    /* Secure Data (explicitly marked) */
    .secure_data :
    {
        . = ALIGN(4);
        *(.secure_data)       /* Data marked with __attribute__((section(".secure_data"))) */
        . = ALIGN(4);
    } > SRAM_S

    /* Heap */
    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        __heap_start = .;
        . = . + __HEAP_SIZE;
        . = ALIGN(8);
        __heap_end = .;
    } > SRAM_S

    /* Stack (grows downward) */
    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        __stack_limit = .;
        . = . + __STACK_SIZE;
        . = ALIGN(8);
        __stack_top = .;
    } > SRAM_S

    PROVIDE(__stack = __stack_top);

    /* Stack seal (for stack overflow detection) */
    .stackseal (NOLOAD) :
    {
        . = ALIGN(8);
        __StackSeal = .;
        . = . + __STACKSEAL_SIZE;
        . = ALIGN(8);
    } > SRAM_S
}

/* ════════════════════════════════════════════════════════════════════════
 * Symbol Exports for Code
 * ════════════════════════════════════════════════════════════════════════
 * These symbols are used by startup code and the application
 */

/* Flash regions */
PROVIDE(__FLASH_S_START   = ORIGIN(FLASH_S));
PROVIDE(__FLASH_S_SIZE    = LENGTH(FLASH_S));
PROVIDE(__FLASH_NSC_START = ORIGIN(FLASH_NSC));
PROVIDE(__FLASH_NSC_SIZE  = LENGTH(FLASH_NSC));

/* SRAM regions */
PROVIDE(__SRAM_S_START    = ORIGIN(SRAM_S));
PROVIDE(__SRAM_S_SIZE     = LENGTH(SRAM_S));

/* Non-Secure entry point (from Non-Secure linker script) */
/* This will be resolved when linking with NS world */
PROVIDE(__NS_RESET_HANDLER = 0x08040000);  /* NS vector table */

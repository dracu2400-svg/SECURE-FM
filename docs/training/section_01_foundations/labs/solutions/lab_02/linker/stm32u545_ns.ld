/**
 ******************************************************************************
 * @file    stm32u545_ns.ld
 * @brief   Linker script for STM32U545RET6Q Non-Secure world
 * @details This linker script defines the memory layout for the Non-Secure
 *          partition of a TrustZone-enabled application.
 *
 * Memory Configuration for STM32U545RE (TrustZone Enabled):
 * ═══════════════════════════════════════════════════════════
 *
 * FLASH (512KB total):
 * ┌─────────────────────────────────────────────────────────┐
 * │ 0x08000000 │ Secure Flash        │ 248KB │ S Code      │ ← NOT ACCESSIBLE
 * ├─────────────────────────────────────────────────────────┤
 * │ 0x0803E000 │ NSC Region          │ 8KB   │ SG Veneers  │ ← Execute Only
 * ├─────────────────────────────────────────────────────────┤
 * │ 0x08040000 │ Non-Secure Flash    │ 256KB │ NS Code     │ ← OUR SPACE
 * └─────────────────────────────────────────────────────────┘
 *
 * SRAM (256KB total):
 * ┌─────────────────────────────────────────────────────────┐
 * │ 0x30000000 │ Secure SRAM         │ 64KB  │ S Data      │ ← NOT ACCESSIBLE
 * ├─────────────────────────────────────────────────────────┤
 * │ 0x20000000 │ Non-Secure SRAM     │ 192KB │ NS Data     │ ← OUR SPACE
 * └─────────────────────────────────────────────────────────┘
 *
 * Access Rules from Non-Secure World:
 * ────────────────────────────────────
 * ✓ CAN ACCESS:
 *   - Non-Secure Flash (0x08040000 - 0x0807FFFF) - Read/Execute
 *   - NSC Region (0x0803E000 - 0x0803FFFF) - Execute Only (SG instructions)
 *   - Non-Secure SRAM (0x20000000 - 0x2002FFFF) - Read/Write/Execute
 *
 * ✗ CANNOT ACCESS (SecureFault):
 *   - Secure Flash (0x08000000 - 0x0803DFFF)
 *   - Secure SRAM (0x30000000 - 0x3000FFFF)
 *   - Secure peripherals
 *
 * Calling Secure Functions:
 * ──────────────────────────
 * 1. Non-Secure code calls function pointer to NSC region
 * 2. CPU jumps to NSC veneer (SG instruction)
 * 3. Context switches to Secure world
 * 4. Secure function executes
 * 5. Returns with BXNS (context switch back to NS)
 *
 ******************************************************************************
 */

/* Entry Point */
ENTRY(Reset_Handler)

/* Memory Regions */
MEMORY
{
    /* ════════════════════════════════════════════════════════════════════
     * FLASH Memory Layout (Non-Secure portion)
     * ════════════════════════════════════════════════════════════════════
     */

    /* Non-Secure Flash: 256KB (0x08040000 - 0x0807FFFF)
     * Contains:
     *   - Non-Secure vector table
     *   - Non-Secure code (.text)
     *   - Non-Secure constants (.rodata)
     */
    FLASH_NS (rx) : ORIGIN = 0x08040000, LENGTH = 256K

    /* ════════════════════════════════════════════════════════════════════
     * RAM Memory Layout (Non-Secure portion)
     * ════════════════════════════════════════════════════════════════════
     */

    /* Non-Secure SRAM: 192KB (0x20000000 - 0x2002FFFF)
     * Contains:
     *   - Non-Secure data (.data)
     *   - Non-Secure BSS (.bss)
     *   - Non-Secure heap
     *   - Non-Secure stack
     */
    SRAM_NS (rwx) : ORIGIN = 0x20000000, LENGTH = 192K
}

/* Non-Secure Stack and Heap sizes */
__STACK_SIZE = 0x2000;  /* 8KB */
__HEAP_SIZE  = 0x2000;  /* 8KB */

/* NSC Region information (from Secure world) */
__NSC_REGION_START = 0x0803E000;
__NSC_REGION_SIZE  = 0x00002000;  /* 8KB */

/* Section Definitions */
SECTIONS
{
    /* ════════════════════════════════════════════════════════════════════
     * FLASH Sections (Non-Secure)
     * ════════════════════════════════════════════════════════════════════
     */

    /* Non-Secure Vector Table */
    .vectors :
    {
        . = ALIGN(4);
        KEEP(*(.vectors))     /* Startup code - vector table */
        . = ALIGN(4);
    } > FLASH_NS

    /* Non-Secure Program Code */
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
    } > FLASH_NS

    /* Non-Secure Read-Only Data */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)            /* .rodata sections (constants) */
        *(.rodata*)           /* .rodata* sections (constants) */
        . = ALIGN(4);
    } > FLASH_NS

    /* ARM C++ exception unwinding */
    .ARM.extab :
    {
        . = ALIGN(4);
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        . = ALIGN(4);
    } > FLASH_NS

    .ARM :
    {
        . = ALIGN(4);
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
        . = ALIGN(4);
    } > FLASH_NS

    /* Pre-initialization array */
    .preinit_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
        . = ALIGN(4);
    } > FLASH_NS

    /* Initialization array */
    .init_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
        . = ALIGN(4);
    } > FLASH_NS

    /* Finalization array */
    .fini_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array*))
        PROVIDE_HIDDEN (__fini_array_end = .);
        . = ALIGN(4);
    } > FLASH_NS

    /* ════════════════════════════════════════════════════════════════════
     * RAM Sections (Non-Secure)
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
    } > SRAM_NS AT> FLASH_NS

    /* Copy .data from Flash to RAM */
    _sidata = LOADADDR(.data);

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
    } > SRAM_NS

    /* Non-Secure Data (explicitly marked) */
    .ns_data :
    {
        . = ALIGN(4);
        *(.ns_data)           /* Data marked with __attribute__((section(".ns_data"))) */
        . = ALIGN(4);
    } > SRAM_NS

    /* Heap */
    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        __heap_start = .;
        __end__ = .;
        PROVIDE(end = .);
        . = . + __HEAP_SIZE;
        . = ALIGN(8);
        __heap_end = .;
        __HeapLimit = .;
    } > SRAM_NS

    /* Stack (grows downward) */
    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        __stack_limit = .;
        __StackLimit = .;
        . = . + __STACK_SIZE;
        . = ALIGN(8);
        __stack_top = .;
        __StackTop = .;
    } > SRAM_NS

    PROVIDE(__stack = __stack_top);
    PROVIDE(_estack = __stack_top);

    /* Check for stack/heap collision */
    ASSERT(__heap_end <= __stack_limit, "Heap and stack collision!")
}

/* ════════════════════════════════════════════════════════════════════════
 * Symbol Exports for Code
 * ════════════════════════════════════════════════════════════════════════
 * These symbols are used by startup code and the application
 */

/* Flash regions */
PROVIDE(__FLASH_NS_START = ORIGIN(FLASH_NS));
PROVIDE(__FLASH_NS_SIZE  = LENGTH(FLASH_NS));

/* SRAM regions */
PROVIDE(__SRAM_NS_START  = ORIGIN(SRAM_NS));
PROVIDE(__SRAM_NS_SIZE   = LENGTH(SRAM_NS));

/* Secure regions (read-only reference, cannot access!) */
PROVIDE(__FLASH_S_START  = 0x08000000);
PROVIDE(__FLASH_S_SIZE   = 0x0003E000);  /* 248KB */
PROVIDE(__SRAM_S_START   = 0x30000000);
PROVIDE(__SRAM_S_SIZE    = 0x00010000);  /* 64KB */

/**
 * Memory Layout Summary
 * ═════════════════════════════════════════════════════════════════════════
 *
 * What Non-Secure Code Can See:
 * ──────────────────────────────
 * ┌──────────────┬──────────────┬──────────────────────────────────────┐
 * │ Region       │ Address      │ Description                          │
 * ├──────────────┼──────────────┼──────────────────────────────────────┤
 * │ NS Flash     │ 0x08040000   │ Our code and constants (256KB)       │
 * │ NS SRAM      │ 0x20000000   │ Our data, heap, stack (192KB)        │
 * │ NSC Veneers  │ 0x0803E000   │ Call Secure functions (Execute only) │
 * │ Peripherals  │ 0x40000000   │ Non-Secure peripherals (GPIO, etc.)  │
 * └──────────────┴──────────────┴──────────────────────────────────────┘
 *
 * What Happens If We Try to Access Secure Memory:
 * ────────────────────────────────────────────────
 * uint32_t *secure_ptr = (uint32_t *)0x30000000;  // Secure SRAM
 * uint32_t value = *secure_ptr;                   // ← SecureFault!
 *
 * SecureFault Handler will trigger:
 * - LD3 (Red LED) turns on
 * - "SECURITY VIOLATION" printed to serial
 * - System resets (in this lab)
 *
 * How to Call Secure Functions:
 * ──────────────────────────────
 * 1. Declare NSC function:
 *    extern void secure_led_blink(uint32_t count);
 *
 * 2. Call normally:
 *    secure_led_blink(5);  // CPU automatically switches to Secure world
 *
 * 3. Function executes in Secure world, then returns to Non-Secure
 *
 * The Magic of NSC Veneers:
 * ─────────────────────────
 * secure_led_blink() is NOT in our Flash (0x08040000+)
 * It's a veneer at 0x0803Exxx that contains:
 *   - SG (Secure Gateway) instruction
 *   - Branch to actual Secure implementation
 *
 * This is how the CPU knows it's a legitimate Secure entry point!
 */

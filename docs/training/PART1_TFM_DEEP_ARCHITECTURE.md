# TF-M Deep Architecture Guide
## Complete Technical Deep Dive with Register-Level Details

---

## Table of Contents

1. [ARM TrustZone Architecture Deep Dive](#1-arm-trustzone-architecture-deep-dive)
2. [TF-M Secure Partition Manager Internals](#2-tfm-secure-partition-manager-internals)
3. [Memory Management and MPU Configuration](#3-memory-management-and-mpu-configuration)
4. [Secure-to-NonSecure Transition Flow](#4-secure-to-nonsecure-transition-flow)
5. [PSA Service Call Data Path](#5-psa-service-call-data-path)
6. [Interrupt Handling in TF-M](#6-interrupt-handling-in-tfm)
7. [Context Switching Mechanism](#7-context-switching-mechanism)
8. [Cryptographic Hardware Integration](#8-cryptographic-hardware-integration)

---

## 1. ARM TrustZone Architecture Deep Dive

### 1.1 TrustZone Hardware Components

TrustZone for Armv8-M provides hardware-enforced isolation through several key components:

```
ARM Cortex-M33/M35P Processor
├── Security Attribution Unit (SAU)
│   ├── 8 configurable regions
│   └── Memory security assignment
├── Implementation Defined Attribution Unit (IDAU)
│   ├── Vendor-specific security
│   └── Cannot be overridden
├── Memory Protection Unit (MPU)
│   ├── Secure MPU (MPU_S)
│   └── Non-Secure MPU (MPU_NS)
├── Nested Vectored Interrupt Controller (NVIC)
│   ├── Secure NVIC
│   └── Non-Secure NVIC
└── Processor Registers
    ├── Secure Stack Pointers (MSP_S, PSP_S)
    ├── Non-Secure Stack Pointers (MSP_NS, PSP_NS)
    └── Security State Control Registers
```

### 1.2 Security Attribution Unit (SAU) - Register Level

The SAU divides the memory map into Secure and Non-Secure regions.

**SAU Control Register (SAU->CTRL):**

```c
/* SAU Control Register at 0xE000EDD0 */
typedef struct {
    uint32_t ENABLE:1;    /* Bit 0: Enable SAU */
    uint32_t ALLNS:1;     /* Bit 1: All memory Non-Secure when SAU disabled */
    uint32_t :30;         /* Bits 2-31: Reserved */
} SAU_CTRL_Type;

/* Example: Enable SAU */
SAU->CTRL = 0x1;  /* ENABLE=1, ALLNS=0 */
```

**SAU Region Number Register (SAU->RNR):**

```c
/* SAU Region Number Register at 0xE000EDD8 */
typedef struct {
    uint32_t REGION:8;    /* Bits 0-7: Region number (0-7) */
    uint32_t :24;         /* Bits 8-31: Reserved */
} SAU_RNR_Type;

/* Select region 0 */
SAU->RNR = 0;
```

**SAU Region Base Address Register (SAU->RBAR):**

```c
/* SAU Region Base Address Register at 0xE000EDDC */
typedef struct {
    uint32_t :5;          /* Bits 0-4: Reserved */
    uint32_t BADDR:27;    /* Bits 5-31: Base address (32-byte aligned) */
} SAU_RBAR_Type;

#define SAU_RBAR_BADDR_Msk    (0xFFFFFFE0UL)

/* Set base address to 0x10000000 */
SAU->RBAR = 0x10000000 & SAU_RBAR_BADDR_Msk;
```

**SAU Region Limit Address Register (SAU->RLAR):**

```c
/* SAU Region Limit Address Register at 0xE000EDE0 */
typedef struct {
    uint32_t ENABLE:1;    /* Bit 0: Region enable */
    uint32_t NSC:1;       /* Bit 1: Non-Secure Callable */
    uint32_t :3;          /* Bits 2-4: Reserved */
    uint32_t LADDR:27;    /* Bits 5-31: Limit address (32-byte aligned) */
} SAU_RLAR_Type;

#define SAU_RLAR_ENABLE_Msk   (1UL << 0)
#define SAU_RLAR_NSC_Msk      (1UL << 1)
#define SAU_RLAR_LADDR_Msk    (0xFFFFFFE0UL)

/* Set limit to 0x1FFFFFFF, enable, Non-Secure */
SAU->RLAR = (0x1FFFFFFF & SAU_RLAR_LADDR_Msk) | SAU_RLAR_ENABLE_Msk;
```

**Complete SAU Configuration Example:**

```c
/**
 * Configure SAU for TF-M on STM32U585
 */
void TZ_SAU_Setup(void)
{
    /* Disable SAU during configuration */
    SAU->CTRL = 0;

    /* Region 0: Non-Secure Flash (0x08100000 - 0x081FFFFF) */
    SAU->RNR  = 0;
    SAU->RBAR = 0x08100000 & SAU_RBAR_BADDR_Msk;
    SAU->RLAR = (0x081FFFFF & SAU_RLAR_LADDR_Msk) | SAU_RLAR_ENABLE_Msk;

    /* Region 1: Non-Secure RAM (0x20040000 - 0x2007FFFF) */
    SAU->RNR  = 1;
    SAU->RBAR = 0x20040000 & SAU_RBAR_BADDR_Msk;
    SAU->RLAR = (0x2007FFFF & SAU_RLAR_LADDR_Msk) | SAU_RLAR_ENABLE_Msk;

    /* Region 2: Non-Secure Callable (NSC) - Veneers (0x080FE000 - 0x080FFFFF) */
    SAU->RNR  = 2;
    SAU->RBAR = 0x080FE000 & SAU_RBAR_BADDR_Msk;
    SAU->RLAR = (0x080FFFFF & SAU_RLAR_LADDR_Msk) |
                SAU_RLAR_ENABLE_Msk | SAU_RLAR_NSC_Msk;

    /* Region 3: Non-Secure Peripherals (0x40000000 - 0x4FFFFFFF) */
    SAU->RNR  = 3;
    SAU->RBAR = 0x40000000 & SAU_RBAR_BADDR_Msk;
    SAU->RLAR = (0x4FFFFFFF & SAU_RLAR_LADDR_Msk) | SAU_RLAR_ENABLE_Msk;

    /* Enable SAU */
    SAU->CTRL = SAU_CTRL_ENABLE_Msk;

    /* Memory barriers to ensure SAU is configured before continuing */
    __DSB();
    __ISB();
}
```

### 1.3 Memory Protection Unit (MPU) - Detailed Configuration

The MPU provides fine-grained memory protection. Both Secure and Non-Secure worlds have separate MPUs.

**MPU Type Register (MPU->TYPE):**

```c
/* Read-only register describing MPU capabilities */
typedef struct {
    uint32_t SEPARATE:1;  /* Bit 0: 1=Separate I/D MPUs */
    uint32_t :7;          /* Bits 1-7: Reserved */
    uint32_t DREGION:8;   /* Bits 8-15: Number of data regions */
    uint32_t IREGION:8;   /* Bits 16-23: Number of instruction regions */
    uint32_t :8;          /* Bits 24-31: Reserved */
} MPU_TYPE_Type;

/* Check MPU capabilities */
uint32_t mpu_type = MPU->TYPE;
uint32_t num_regions = (mpu_type >> 8) & 0xFF;  /* Typically 8 or 16 */
```

**MPU Control Register (MPU->CTRL):**

```c
/* MPU Control Register */
typedef struct {
    uint32_t ENABLE:1;     /* Bit 0: Enable MPU */
    uint32_t HFNMIENA:1;   /* Bit 1: Enable MPU during HardFault/NMI */
    uint32_t PRIVDEFENA:1; /* Bit 2: Enable default memory map for privileged */
    uint32_t :29;          /* Bits 3-31: Reserved */
} MPU_CTRL_Type;

#define MPU_CTRL_ENABLE_Msk     (1UL << 0)
#define MPU_CTRL_HFNMIENA_Msk   (1UL << 1)
#define MPU_CTRL_PRIVDEFENA_Msk (1UL << 2)

/* Enable MPU with default memory map for privileged access */
MPU->CTRL = MPU_CTRL_ENABLE_Msk | MPU_CTRL_PRIVDEFENA_Msk;
```

**MPU Region Number Register (MPU->RNR):**

```c
/* Select MPU region to configure */
MPU->RNR = 0;  /* Select region 0 */
```

**MPU Region Base Address Register (MPU->RBAR):**

```c
/* MPU Region Base Address Register */
typedef struct {
    uint32_t REGION:4;    /* Bits 0-3: Region number (optional) */
    uint32_t VALID:1;     /* Bit 4: MPU_RNR valid */
    uint32_t ADDR:27;     /* Bits 5-31: Base address (aligned to size) */
} MPU_RBAR_Type;

/* Set base address to 0x20000000 */
MPU->RBAR = 0x20000000;  /* Assuming region already selected via RNR */
```

**MPU Region Attribute and Size Register (MPU->RASR):**

```c
/* MPU Region Attribute and Size Register */
typedef struct {
    uint32_t ENABLE:1;    /* Bit 0: Region enable */
    uint32_t SIZE:5;      /* Bits 1-5: Region size (2^(SIZE+1) bytes) */
    uint32_t :2;          /* Bits 6-7: Reserved */
    uint32_t SRD:8;       /* Bits 8-15: Subregion disable */
    uint32_t B:1;         /* Bit 16: Bufferable */
    uint32_t C:1;         /* Bit 17: Cacheable */
    uint32_t S:1;         /* Bit 18: Shareable */
    uint32_t TEX:3;       /* Bits 19-21: Type extension */
    uint32_t :2;          /* Bits 22-23: Reserved */
    uint32_t AP:3;        /* Bits 24-26: Access permission */
    uint32_t :1;          /* Bit 27: Reserved */
    uint32_t XN:1;        /* Bit 28: Execute never */
    uint32_t :3;          /* Bits 29-31: Reserved */
} MPU_RASR_Type;

/* Access Permission (AP) field values */
#define MPU_AP_PRIV_RW_USER_NA  0  /* Privileged RW, User No Access */
#define MPU_AP_PRIV_RW_USER_RW  1  /* Privileged RW, User RW */
#define MPU_AP_PRIV_RW_USER_RO  2  /* Privileged RW, User RO */
#define MPU_AP_PRIV_RO_USER_RO  3  /* Privileged RO, User RO */
#define MPU_AP_PRIV_RO_USER_NA  5  /* Privileged RO, User No Access */

/* Region size encoding: SIZE = log2(region_size) - 1 */
/* Examples:
 * 32 bytes   = 2^5  = SIZE 4
 * 256 bytes  = 2^8  = SIZE 7
 * 4 KB       = 2^12 = SIZE 11
 * 128 KB     = 2^17 = SIZE 16
 * 512 KB     = 2^19 = SIZE 18
 */

#define MPU_RASR_SIZE_32B   4
#define MPU_RASR_SIZE_256B  7
#define MPU_RASR_SIZE_4KB   11
#define MPU_RASR_SIZE_128KB 16
#define MPU_RASR_SIZE_512KB 18

/* Memory attributes */
#define MPU_RASR_ATTR_FLASH  (0 << 16) | (1 << 17)  /* Non-bufferable, Cacheable */
#define MPU_RASR_ATTR_SRAM   (1 << 16) | (1 << 17)  /* Bufferable, Cacheable */
#define MPU_RASR_ATTR_DEVICE (1 << 16) | (0 << 17)  /* Bufferable, Non-cacheable */
```

**Complete MPU Configuration Example:**

```c
/**
 * Configure MPU for TF-M Secure partition
 */
void MPU_Setup_Secure(void)
{
    /* Disable MPU during configuration */
    MPU->CTRL = 0;

    /* Region 0: Secure Flash (512 KB at 0x08000000) */
    /* Read-only, Execute allowed, Privileged access */
    MPU->RNR = 0;
    MPU->RBAR = 0x08000000;
    MPU->RASR = (1 << 0) |                          /* ENABLE */
                (18 << 1) |                         /* SIZE = 18 (512 KB) */
                (0 << 8) |                          /* SRD = 0 (all enabled) */
                (MPU_RASR_ATTR_FLASH) |             /* Flash attributes */
                (MPU_AP_PRIV_RO_USER_NA << 24) |    /* Privileged RO */
                (0 << 28);                          /* XN = 0 (execute allowed) */

    /* Region 1: Secure RAM (128 KB at 0x20000000) */
    /* Read-write, Execute never, Privileged access */
    MPU->RNR = 1;
    MPU->RBAR = 0x20000000;
    MPU->RASR = (1 << 0) |                          /* ENABLE */
                (16 << 1) |                         /* SIZE = 16 (128 KB) */
                (0 << 8) |                          /* SRD = 0 */
                (MPU_RASR_ATTR_SRAM) |              /* SRAM attributes */
                (MPU_AP_PRIV_RW_USER_NA << 24) |    /* Privileged RW */
                (1 << 28);                          /* XN = 1 (no execute) */

    /* Region 2: Secure Peripherals (16 MB at 0x50000000) */
    /* Read-write, Execute never, Device memory */
    MPU->RNR = 2;
    MPU->RBAR = 0x50000000;
    MPU->RASR = (1 << 0) |                          /* ENABLE */
                (23 << 1) |                         /* SIZE = 23 (16 MB) */
                (0 << 8) |                          /* SRD = 0 */
                (MPU_RASR_ATTR_DEVICE) |            /* Device attributes */
                (MPU_AP_PRIV_RW_USER_NA << 24) |    /* Privileged RW */
                (1 << 28);                          /* XN = 1 */

    /* Region 3: Crypto Accelerator (4 KB at 0x520C0000) */
    /* Read-write, Execute never, Device memory, Shareable */
    MPU->RNR = 3;
    MPU->RBAR = 0x520C0000;
    MPU->RASR = (1 << 0) |                          /* ENABLE */
                (11 << 1) |                         /* SIZE = 11 (4 KB) */
                (0 << 8) |                          /* SRD = 0 */
                (1 << 16) |                         /* Bufferable */
                (0 << 17) |                         /* Non-cacheable */
                (1 << 18) |                         /* Shareable */
                (MPU_AP_PRIV_RW_USER_NA << 24) |    /* Privileged RW */
                (1 << 28);                          /* XN = 1 */

    /* Enable MPU with default memory map for privileged access */
    MPU->CTRL = MPU_CTRL_ENABLE_Msk | MPU_CTRL_PRIVDEFENA_Msk;

    __DSB();
    __ISB();
}
```

### 1.4 Stack Pointer Selection

ARM Cortex-M has separate stack pointers for Secure and Non-Secure worlds:

```c
/* Stack Pointer Registers */
/*
 * MSP_S: Main Stack Pointer (Secure)
 * PSP_S: Process Stack Pointer (Secure)
 * MSP_NS: Main Stack Pointer (Non-Secure)
 * PSP_NS: Process Stack Pointer (Non-Secure)
 */

/* Control Register (CONTROL) */
typedef struct {
    uint32_t nPRIV:1;     /* Bit 0: 0=Privileged, 1=Unprivileged */
    uint32_t SPSEL:1;     /* Bit 1: 0=MSP, 1=PSP */
    uint32_t FPCA:1;      /* Bit 2: FP context active */
    uint32_t SFPA:1;      /* Bit 3: Secure FP active */
    uint32_t :28;         /* Bits 4-31: Reserved */
} CONTROL_Type;

/* Read/Write stack pointers */
static inline void __set_MSP_S(uint32_t topOfMainStack)
{
    __ASM volatile ("MSR msp_s, %0" : : "r" (topOfMainStack) : );
}

static inline uint32_t __get_MSP_S(void)
{
    uint32_t result;
    __ASM volatile ("MRS %0, msp_s" : "=r" (result) );
    return result;
}

static inline void __set_PSP_S(uint32_t topOfProcStack)
{
    __ASM volatile ("MSR psp_s, %0" : : "r" (topOfProcStack) : );
}

static inline uint32_t __get_PSP_S(void)
{
    uint32_t result;
    __ASM volatile ("MRS %0, psp_s" : "=r" (result) );
    return result;
}

/* Set MSP_NS from Secure code */
static inline void __TZ_set_MSP_NS(uint32_t topOfMainStack)
{
    __ASM volatile ("MSR msp_ns, %0" : : "r" (topOfMainStack) : );
}

/* Example: Initialize stack pointers */
void init_stack_pointers(void)
{
    /* Set Secure stacks */
    __set_MSP_S(0x30020000);  /* Secure main stack @ end of Secure RAM */
    __set_PSP_S(0x30018000);  /* Secure process stack */

    /* Set Non-Secure stacks from Secure code */
    __TZ_set_MSP_NS(0x20040000);  /* Non-Secure main stack */
}
```

### 1.5 Secure Gateway (SG) Instruction

The SG instruction is the only way to transition from Non-Secure to Secure state:

```c
/**
 * Secure Gateway Veneer
 *
 * This function is placed in the NSC (Non-Secure Callable) region
 * Compiler generates SG instruction at entry point
 */

/* Attribute to mark function as Non-Secure Callable */
#define __attribute__((cmse_nonsecure_entry))

__attribute__((cmse_nonsecure_entry))
int secure_function(int param1, int param2)
{
    /*
     * Generated assembly:
     *
     * secure_function:
     *     SG                    ; Secure Gateway instruction
     *     PUSH {r4-r7, lr}      ; Save registers
     *     ...                   ; Function body
     *     POP  {r4-r7, pc}      ; Return (BXNS via pc)
     */

    /* Validate Non-Secure parameters */
    if (cmse_check_address_range((void*)param1, sizeof(int),
                                  CMSE_NONSECURE | CMSE_MPU_READ) == NULL) {
        return -1;  /* Invalid NS address */
    }

    /* Perform secure operation */
    return perform_secure_operation(param1, param2);
}
```

**SG Instruction Behavior:**

```
Before SG:
- State: Non-Secure
- PC: Non-Secure Callable region
- Registers: May contain NS data

SG Instruction Execution:
1. Check PC is in NSC region (else HardFault)
2. Check security state transition valid
3. Switch to Secure state
4. Continue execution in Secure world

After SG:
- State: Secure
- PC: Secure code
- LR: Special value (0xFEFFFFxx) for secure return
- Registers: Still contain NS data (must validate!)
```

### 1.6 Non-Secure Return (BXNS/BLXNS)

Return from Secure to Non-Secure uses special instructions:

```c
/**
 * Return to Non-Secure
 */
static inline void return_to_nonsecure(uint32_t ns_entry_point)
{
    /* Set up for Non-Secure return */
    /* LR must have FNC_RETURN value (0xFEFFFFxx) */

    __ASM volatile (
        "MOV R0, %0        \n"  /* Load NS entry point */
        "BXNS R0           \n"  /* Branch to NS (if R0 bit 0 = 0) */
        /* or */
        "BLXNS R0          \n"  /* Call NS (with return) */
        : : "r" (ns_entry_point) : "r0"
    );
}

/*
 * BXNS/BLXNS Behavior:
 *
 * 1. Clear Secure registers (r0-r3, r12, APSR)
 * 2. Switch to Non-Secure state
 * 3. Branch to target address
 * 4. If target address bit 0 = 1, HardFault
 */
```

### 1.7 Complete TrustZone Initialization Example

```c
/**
 * Complete TrustZone initialization for TF-M
 */

/* Secure stack (defined in linker script) */
extern uint32_t __INITIAL_SP_S;
extern uint32_t __STACK_LIMIT_S;

/* Non-Secure entry point */
extern uint32_t __NS_ENTRY;

void TZ_Init_Complete(void)
{
    /*
     * Step 1: Configure SAU
     */
    TZ_SAU_Setup();

    /*
     * Step 2: Configure Secure MPU
     */
    MPU_Setup_Secure();

    /*
     * Step 3: Configure Secure NVIC
     */
    /* All interrupts Secure by default, configure as needed */
    NVIC_SetPriority(SecureInterrupt_IRQn, 0);
    NVIC_EnableIRQ(SecureInterrupt_IRQn);

    /*
     * Step 4: Set up stacks
     */
    __set_MSP_S((uint32_t)&__INITIAL_SP_S);
    __TZ_set_MSP_NS(0x20040000);  /* NS stack */

    /*
     * Step 5: Configure Non-Secure Vector Table
     */
    SCB_NS->VTOR = 0x08100000;  /* NS vector table address */

    /*
     * Step 6: Configure AIRCR (for system control)
     */
    SCB->AIRCR = (0x05FA << SCB_AIRCR_VECTKEY_Pos) |  /* Key */
                 (1 << SCB_AIRCR_PRIS_Pos) |           /* Prioritize Secure */
                 (1 << SCB_AIRCR_BFHFNMINS_Pos);       /* BusFault, HardFault, NMI Secure */

    /*
     * Step 7: Jump to Non-Secure
     */
    /* Get NS Reset Handler address from vector table */
    uint32_t *ns_vtor = (uint32_t *)0x08100000;
    uint32_t ns_reset_handler = ns_vtor[1];

    /* Create function pointer for NS entry */
    typedef void (*ns_func_ptr)(void) __attribute__((cmse_nonsecure_call));
    ns_func_ptr ns_entry = (ns_func_ptr)(ns_reset_handler);

    /* Call Non-Secure code */
    ns_entry();

    /* Should never reach here */
    while(1);
}
```

---

## 2. TF-M Secure Partition Manager Internals

### 2.1 SPM Data Structures

**Partition Information Structure:**

```c
/* spm/include/spm_partition.h */

/**
 * Partition runtime context
 */
struct partition_t {
    uint32_t partition_id;          /* Unique partition ID */
    uint32_t flags;                 /* Partition flags */
    uint32_t priority;              /* Thread priority (IPC only) */
    uintptr_t boundary;             /* Partition boundary marker */

    /* Stack information */
    uintptr_t stack_base;           /* Stack base address */
    size_t stack_size;              /* Stack size in bytes */

    /* Context */
    struct context_ctrl_t ctx_ctrl; /* Context control */

    /* IPC-specific */
    #if CONFIG_TFM_SPM_BACKEND_IPC
    struct thread_t *p_thread;      /* Thread control block */
    uint8_t signals_asserted;       /* Asserted signals */
    uint8_t signals_waiting;        /* Signals being waited */
    #endif

    /* Isolation */
    struct partition_load_info_t *p_ldinf;  /* Load information */
    struct platform_data_t *platform_data;   /* Platform-specific data */
};

/**
 * Partition load information (from manifest)
 */
struct partition_load_info_t {
    uint32_t psa_ff_ver;            /* PSA FF version */
    uint32_t pid;                   /* Partition ID */
    uint32_t flags;                 /* Partition flags */
    uint32_t priority;              /* Priority */
    uint32_t entry;                 /* Entry point function */
    uint32_t stack_size;            /* Stack size */
    uint32_t heap_size;             /* Heap size */
    uint32_t ndeps;                 /* Number of dependencies */
    uint32_t nservices;             /* Number of services */
    uint32_t nirqs;                 /* Number of IRQs */
    uint32_t nmmio;                 /* Number of MMIO regions */
};
```

**Service Information Structure:**

```c
/**
 * Service runtime context
 */
struct service_t {
    struct partition_t *partition;  /* Owning partition */
    uint32_t service_db;            /* Service database entry */
    uint32_t sid;                   /* Service ID */
    uint32_t version;               /* Service version */
    psa_signal_t signal;            /* Service signal */

    #if CONFIG_TFM_SPM_BACKEND_IPC
    uint32_t msg_queue;             /* Message queue */
    #endif
};

/**
 * Service load information (from manifest)
 */
struct service_load_info_t {
    uint32_t sid;                   /* Service ID (RoT Service ID) */
    uint32_t flags;                 /* Service flags */
    uint32_t version;               /* Version */
    psa_signal_t signal;            /* Signal value */
};
```

### 2.2 IPC Message Handling Data Path

When a Non-Secure application calls a PSA API, here's the complete data flow:

```
Step 1: NS Application calls psa_call()
├── Location: interface/src/tfm_psa_api_veneers.c
└── Function: tfm_psa_call_veneer()

Step 2: Veneer transitions to Secure
├── Attribute: __attribute__((cmse_nonsecure_entry))
├── Assembly: SG instruction executed
└── State: Now in Secure world

Step 3: Veneer validates parameters
├── Check: NS pointer validity
├── Check: Handle validity
└── Function: cmse_check_address_range()

Step 4: Veneer calls SPM
├── Location: secure_fw/spm/core/tfm_spm_api.c
└── Function: tfm_spm_client_psa_call()

Step 5: SPM allocates message
├── Structure: struct client_call_params_t
├── Location: In caller's partition memory
└── Contents: invec[], outvec[], type, etc.

Step 6: SPM finds target service
├── Lookup: Service ID → service_t structure
├── Check: Service version compatibility
└── Get: Target partition

Step 7: SPM enqueues message
├── Queue: Target partition's message queue
├── Signal: Set service signal bit
└── Action: Wake target partition thread

Step 8: SPM scheduler runs
├── Select: Highest priority ready partition
├── Context switch: To target partition
└── Execute: Partition's psa_wait()

Step 9: Target partition processes
├── Function: psa_wait() returns with signal
├── Function: psa_get() retrieves message
├── Process: Execute service logic
├── Function: psa_write() for outputs
└── Function: psa_reply() sends result

Step 10: SPM returns to caller
├── Context switch: Back to caller partition
├── Copy: Output data to NS memory
└── Return: Status code

Step 11: Veneer returns to NS
├── Instruction: BXNS
├── Clear: Secure registers
└── State: Back to Non-Secure
```

**Detailed Code Example:**

```c
/* Step 1-3: Veneer function */
/* interface/src/tfm_psa_api_veneers.c */

__attribute__((cmse_nonsecure_entry))
psa_status_t tfm_psa_call_veneer(psa_handle_t handle,
                                  int32_t type,
                                  const psa_invec *in_vec,
                                  size_t in_len,
                                  psa_outvec *out_vec,
                                  size_t out_len)
{
    /* SG instruction here (compiler-generated) */

    /* Validate all NS pointers */
    if (in_vec != NULL) {
        if (cmse_check_address_range((void *)in_vec,
                                      in_len * sizeof(psa_invec),
                                      CMSE_NONSECURE | CMSE_MPU_READ) == NULL) {
            return PSA_ERROR_INVALID_ARGUMENT;
        }

        /* Validate each invec buffer */
        for (size_t i = 0; i < in_len; i++) {
            if (in_vec[i].base != NULL) {
                if (cmse_check_address_range((void *)in_vec[i].base,
                                              in_vec[i].len,
                                              CMSE_NONSECURE | CMSE_MPU_READ) == NULL) {
                    return PSA_ERROR_INVALID_ARGUMENT;
                }
            }
        }
    }

    if (out_vec != NULL) {
        if (cmse_check_address_range((void *)out_vec,
                                      out_len * sizeof(psa_outvec),
                                      CMSE_NONSECURE | CMSE_MPU_READWRITE) == NULL) {
            return PSA_ERROR_INVALID_ARGUMENT;
        }

        /* Validate each outvec buffer */
        for (size_t i = 0; i < out_len; i++) {
            if (out_vec[i].base != NULL) {
                if (cmse_check_address_range((void *)out_vec[i].base,
                                              out_vec[i].len,
                                              CMSE_NONSECURE | CMSE_MPU_READWRITE) == NULL) {
                    return PSA_ERROR_INVALID_ARGUMENT;
                }
            }
        }
    }

    /* Call SPM implementation */
    return tfm_spm_client_psa_call(handle, type, in_vec, in_len, out_vec, out_len);
}

/* Step 4-7: SPM implementation */
/* secure_fw/spm/core/tfm_spm_api.c */

psa_status_t tfm_spm_client_psa_call(psa_handle_t handle,
                                      int32_t type,
                                      const psa_invec *in_vec,
                                      size_t in_len,
                                      psa_outvec *out_vec,
                                      size_t out_len)
{
    struct connection_t *p_connection;
    struct service_t *service;
    struct partition_t *curr_partition, *target_partition;
    struct client_call_params_t params;
    psa_status_t status;

    /* Get current partition */
    curr_partition = GET_CURRENT_PARTITION();

    /* Validate handle */
    p_connection = spm_get_connection_by_handle(handle);
    if (!p_connection) {
        return PSA_ERROR_INVALID_HANDLE;
    }

    service = p_connection->service;
    target_partition = service->partition;

    /* Prepare call parameters */
    params.handle = handle;
    params.type = type;
    params.in_len = in_len;
    params.out_len = out_len;

    /* Copy invec data to secure memory */
    for (size_t i = 0; i < in_len; i++) {
        params.in_vec[i].base = in_vec[i].base;
        params.in_vec[i].len = in_vec[i].len;
    }

    /* Copy outvec pointers */
    for (size_t i = 0; i < out_len; i++) {
        params.out_vec[i].base = out_vec[i].base;
        params.out_vec[i].len = out_vec[i].len;
    }

    /* Allocate message */
    struct tfm_msg_body_t *p_msg = spm_get_empty_msg_buffer();
    if (!p_msg) {
        return PSA_ERROR_INSUFFICIENT_MEMORY;
    }

    /* Fill message */
    p_msg->msg.type = type;
    p_msg->msg.client_id = curr_partition->partition_id;
    p_msg->msg.handle = handle;
    p_msg->msg.in_size[0] = in_len > 0 ? in_vec[0].len : 0;
    p_msg->msg.in_size[1] = in_len > 1 ? in_vec[1].len : 0;
    p_msg->msg.in_size[2] = in_len > 2 ? in_vec[2].len : 0;
    p_msg->msg.in_size[3] = in_len > 3 ? in_vec[3].len : 0;
    p_msg->msg.out_size[0] = out_len > 0 ? out_vec[0].len : 0;
    p_msg->msg.out_size[1] = out_len > 1 ? out_vec[1].len : 0;
    p_msg->msg.out_size[2] = out_len > 2 ? out_vec[2].len : 0;
    p_msg->msg.out_size[3] = out_len > 3 ? out_vec[3].len : 0;
    p_msg->caller_data = &params;

    /* Enqueue message to target partition */
    tfm_msg_enqueue(&target_partition->msg_queue, p_msg);

    /* Assert service signal */
    tfm_event_signal(target_partition, service->signal);

    /* Block caller until reply */
    tfm_event_wait(curr_partition, TFM_EVENT_CALL_REPLY);

    /* Reply received, copy output data */
    status = p_msg->msg.status;

    /* Free message */
    spm_free_msg_buffer(p_msg);

    return status;
}
```

This is just the beginning of the deep dive. Would you like me to continue with:
1. More SPM internals?
2. Interrupt handling details?
3. Context switching mechanism?
4. Complete lab exercises with solutions?

Let me know and I'll continue creating the comprehensive materials!
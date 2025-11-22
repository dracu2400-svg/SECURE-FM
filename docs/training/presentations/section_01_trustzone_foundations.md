---
marp: true
theme: default
paginate: true
backgroundColor: #ffffff
header: 'TF-M Training Package - Section 1'
footer: 'TrustZone-M Foundations | © 2025'
style: |
  section {
    font-family: 'Arial', sans-serif;
  }
  h1 {
    color: #00AA00;
    font-size: 44pt;
  }
  h2 {
    color: #0066CC;
    font-size: 32pt;
  }
  code {
    background: #1E1E1E;
    color: #D4D4D4;
    font-family: 'Consolas', monospace;
    font-size: 18pt;
  }
  .secure {
    color: #00AA00;
    font-weight: bold;
  }
  .nonsecure {
    color: #0066CC;
    font-weight: bold;
  }
  .danger {
    color: #CC0000;
    font-weight: bold;
  }
---

<!-- _class: lead -->
<!-- _paginate: false -->

# TrustZone-M Foundations

**Hardware-Based Security for Embedded Systems**

![bg right:40%](https://via.placeholder.com/400x300/00AA00/ffffff?text=ARM+Cortex-M33)

*TF-M Training Package - Section 1*

---

## Why Security Matters

### Embedded Device Threats

- **70%** of IoT devices have vulnerabilities
- Smart locks, vehicles, medical devices at risk
- Remote exploitation possible
- Financial and safety impacts

![bg right:40%](https://via.placeholder.com/400x300/CC0000/ffffff?text=HACKED)

---

## Cost of Security Breaches

### Case Study: Mirai Botnet

- **2016:** 600,000+ IoT devices compromised
- Massive DDoS attacks
- **$Millions** in damages
- Simple default passwords were the entry point

**Lesson:** Basic security is not enough

---

## Traditional Security Limitations

### Software-Only Protection

❌ Can be bypassed by malware
❌ No isolation between components
❌ Debuggers can extract secrets
❌ Firmware can be tampered with

**We need hardware-enforced boundaries**

---

## Enter TrustZone-M

### Hardware-Enforced Security Boundary

- ✅ CPU-level isolation
- ✅ Memory protection
- ✅ Peripheral access control
- ✅ Cannot be bypassed by software

**Secure World** | **Non-Secure World**

---

## ARM Cortex-M33 Block Diagram

```
┌────────────────────────────────────────┐
│         ARM Cortex-M33 Core            │
├────────────────────────────────────────┤
│  ┌──────────┐  ┌──────────────────┐   │
│  │   CPU    │  │  TrustZone-M     │   │
│  │          │  │  - SAU           │   │
│  │          │  │  - IDAU          │   │
│  └──────────┘  │  - MPU (S/NS)    │   │
│                └──────────────────┘   │
└────────────────────────────────────────┘
```

**Key Components:** SAU, IDAU, MPU, NVIC

---

## Secure vs Non-Secure Worlds

<div style="display: flex;">
<div style="flex: 1; background: #00AA00; color: white; padding: 20px; margin: 10px;">

### Secure World
- Cryptographic keys
- Secure boot
- Attestation
- Protected storage
- Critical peripherals

</div>
<div style="flex: 1; background: #0066CC; color: white; padding: 20px; margin: 10px;">

### Non-Secure World
- Application code
- User interface
- Network stack
- File system
- General peripherals

</div>
</div>

---

## Memory Partitioning

### STM32U545RE-Q Example

```
FLASH (512 KB):
├── 0x08000000 - 0x08040000: Secure (256 KB)
├── 0x08040000 - 0x08042000: NSC (8 KB)
└── 0x08042000 - 0x08080000: Non-Secure (248 KB)

SRAM (256 KB):
├── 0x30000000 - 0x30010000: Secure (64 KB)
└── 0x30010000 - 0x30040000: Non-Secure (192 KB)
```

**NSC = Non-Secure Callable (Gateway Region)**

---

## SAU (Security Attribution Unit)

### Configuring Memory Regions

```c
/* Enable SAU */
SAU->CTRL = SAU_CTRL_ENABLE_Msk;

/* Region 0: NSC region (gateway) */
SAU->RNR = 0;
SAU->RBAR = 0x0C03E000;  /* Start address */
SAU->RLAR = (0x0C040000 - 1) | SAU_RLAR_ENABLE_Msk |
            SAU_RLAR_NSC_Msk;

/* Region 1: Non-Secure Flash */
SAU->RNR = 1;
SAU->RBAR = 0x08040000;
SAU->RLAR = (0x08080000 - 1) | SAU_RLAR_ENABLE_Msk;
```

---

## IDAU (Implementation Defined Attribution Unit)

### Chip-Specific Security

- **Set by silicon vendor** (ST, NXP, Nordic)
- Defines default secure/non-secure regions
- **Cannot be changed by software**
- Works in conjunction with SAU

**IDAU + SAU = Complete Memory Security**

---

## Context Switching (NS → S)

```
1. Non-Secure calls NSC function
   ├── CPU saves NS registers
   ├── Clears r0-r3, r12, LR, PSR
   └── Jumps to Secure Gateway (SG instruction)

2. Secure function executes
   ├── Protected memory access
   ├── Secure peripherals
   └── Cryptographic operations

3. Return to Non-Secure
   ├── Clears secure registers
   └── Restores NS context
```

**Security:** Prevents data leakage between worlds

---

## NSC (Non-Secure Callable) Regions

### Special Memory for Secure Entry Points

```c
/* NSC function in Secure code */
__attribute__((cmse_nonsecure_entry))
int32_t secure_add(int32_t a, int32_t b)
{
    /* SG instruction auto-inserted here */
    return a + b;
}
```

**Only functions in NSC region can be called from NS**

---

## Interrupt Handling

### Secure and Non-Secure Interrupts

```c
/* Configure UART1 as Secure interrupt */
NVIC_SetPriority(USART1_IRQn, 2);
NVIC_EnableIRQ(USART1_IRQn);

/* Configure UART2 as Non-Secure interrupt */
NVIC_SetTargetState(USART2_IRQn, 1);  /* NS */
NVIC_SetPriority(USART2_IRQn, 3);
NVIC_EnableIRQ(USART2_IRQn);
```

**Lower priority number = Higher priority**
**Secure interrupts can preempt Non-Secure**

---

## Exception Handling

### SecureFault Exception

```c
void SecureFault_Handler(void)
{
    uint32_t sfsr = SAU->SFSR;

    if (sfsr & SAU_SFSR_INVTRAN_Msk) {
        printf("Invalid transition!\n");
    }
    if (sfsr & SAU_SFSR_AUVIOL_Msk) {
        printf("Attribution unit violation!\n");
    }

    /* Log and halt */
    while(1);
}
```

---

## Lab 02 Demo

### See TrustZone in Action!

**LED Indicators:**
- 🟢 **Green:** Secure world operation
- 🔵 **Blue:** Non-Secure world operation
- 🔴 **Red:** Security violation detected

**Button Actions:**
- Press USER button to call secure function
- Watch LED toggle between secure/non-secure

---

## CMSE (Cortex-M Security Extensions)

### Compiler Intrinsics

```c
/* Mark function as NSC entry point */
__attribute__((cmse_nonsecure_entry))
void secure_function(void);

/* Check if address is in Non-Secure memory */
void *cmse_check_address_range(void *p, size_t s, int flags);

/* Check if pointer is secure */
void *cmse_check_pointed_object(void *p, int flags);

/* Secure Gateway instruction */
__asm("SG");
```

---

## NSC Function Example

```c
__attribute__((cmse_nonsecure_entry))
void secure_led_blink(uint32_t count)
{
    /* Security check: Validate count */
    if (count > 100) {
        return;  /* Invalid parameter */
    }

    /* Secure implementation */
    for (uint32_t i = 0; i < count; i++) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
        HAL_Delay(100);
    }
}
```

**Accessible from Non-Secure code**

---

## Pointer Validation

### CRITICAL Security Check

```c
__attribute__((cmse_nonsecure_entry))
int secure_process_data(uint8_t *buffer, size_t size)
{
    /* Validate NS pointer before use! */
    uint8_t *checked = cmse_check_address_range(
        buffer, size, CMSE_NONSECURE);

    if (checked == NULL) {
        return -1;  /* Invalid pointer - ATTACK! */
    }

    /* Safe to use now */
    process_data(checked, size);
    return 0;
}
```

---

## Common Pitfalls ⚠️

### Security Mistakes to Avoid

❌ **Not validating NS pointers**
```c
void bad_function(uint8_t *ns_ptr) {
    uint32_t secret = *ns_ptr;  /* DANGER! */
}
```

❌ **Leaking secure data via return values**
```c
uint32_t leak_key(void) {
    return crypto_key;  /* LEAKED! */
}
```

❌ **Using NS pointers in secure memory access**

---

## Best Practices ✅

### Secure Coding Guidelines

✅ **Always validate pointers from NS**
✅ **Minimize NSC function count** (smaller attack surface)
✅ **Clear registers on return** (prevent leakage)
✅ **Check parameter ranges**
✅ **Never trust NS input**
✅ **Log security violations**

**Golden Rule: Treat NS code as potentially malicious**

---

## Code Walkthrough: main_s.c

```c
/* Secure main function */
int main(void)
{
    /* 1. Initialize Secure peripherals */
    HAL_Init();
    SystemClock_Config();

    /* 2. Configure SAU/MPU */
    TZ_SAU_Setup();

    /* 3. Initialize Secure services */
    SecureLED_Init();

    /* 4. Jump to Non-Secure code */
    NonSecure_Init();

    while(1);  /* Should never reach here */
}
```

---

## Code Walkthrough: main_ns.c

```c
/* Non-Secure main function */
int main(void)
{
    /* Initialize NS peripherals */
    HAL_Init();

    while(1) {
        if (button_pressed()) {
            /* Call secure function via NSC */
            secure_led_blink(5);
        }
        HAL_Delay(100);
    }
}
```

**NS code calls S functions through NSC gateway**

---

## Code Walkthrough: nsc_functions.c

```c
/* NSC functions live in special region */
__attribute__((section(".gnu.sgstubs")))
__attribute__((cmse_nonsecure_entry))
void secure_led_blink(uint32_t count)
{
    /* Validate input */
    if (count == 0 || count > MAX_BLINK) {
        return;
    }

    /* Blink secure LED */
    for (uint32_t i = 0; i < count; i++) {
        SecureLED_Toggle();
        HAL_Delay(200);
    }
}
```

---

## What is TF-M?

### Trusted Firmware-M

- **ARM's reference implementation** of PSA
- Open source (BSD license)
- PSA Certified
- Portable across Cortex-M devices
- Production-ready

**Official Project:** github.com/TrustedFirmwareM/trusted-firmware-m

---

## TF-M Architecture

```
┌────────────────────────────────────────┐
│        Non-Secure Application          │
├────────────────────────────────────────┤
│          PSA Client APIs               │
│  (Crypto, Storage, Attestation, FWU)   │
├────────────────────────────────────────┤
│    Secure Partition Manager (SPM)      │
├────────────────────────────────────────┤
│       Secure Partitions (RoT)          │
│  ┌──────┐ ┌─────┐ ┌──────┐ ┌──────┐   │
│  │Crypto│ │ ITS │ │  PS  │ │Attest│   │
│  └──────┘ └─────┘ └──────┘ └──────┘   │
└────────────────────────────────────────┘
```

---

## Secure Partition Manager (SPM)

### Core TF-M Component

**Responsibilities:**
- Manages secure partitions (isolated services)
- Context switching between partitions
- IPC (Inter-Partition Communication)
- Memory protection enforcement
- Interrupt routing

**Think of it as a "mini-OS" for secure world**

---

## PSA Firmware Framework

### Isolation Levels

| Level | Description | Use Case |
|-------|-------------|----------|
| **1** | Basic isolation | Simple devices |
| **2** | Partition isolation | Most IoT |
| **3** | Full isolation | High security |

**Higher level = More security + More overhead**

---

## Partition Manifest

```yaml
{
  "name": "TFM_SP_CRYPTO",
  "type": "APPLICATION-ROT",
  "priority": "NORMAL",
  "model": "IPC",
  "entry_point": "tfm_crypto_init",
  "stack_size": "0x2000",
  "services": [
    {
      "name": "TFM_CRYPTO",
      "sid": "0x00000080",
      "version": 1,
      "non_secure_clients": true
    }
  ]
}
```

---

## Core Secure Services

### Built-in TF-M Services

1. **Crypto** - PSA Crypto API (AES, ECDSA, SHA, etc.)
2. **ITS** - Internal Trusted Storage
3. **PS** - Protected Storage
4. **Attestation** - Device identity proof
5. **FWU** - Firmware Update service

**All accessible via standardized PSA APIs**

---

## PSA APIs

### Standardized Interface

```c
/* Same API across all TF-M devices */
psa_status_t psa_crypto_init(void);

psa_status_t psa_generate_random(
    uint8_t *output,
    size_t output_size);

psa_status_t psa_its_set(
    psa_storage_uid_t uid,
    size_t data_length,
    const void *p_data,
    psa_storage_create_flags_t create_flags);
```

**Vendor-agnostic, future-proof**

---

## MCUboot Integration

### Secure Boot Process

```
1. ROM Bootloader
   └──> Verifies MCUboot signature
         └──> MCUboot
               └──> Verifies TF-M signature
                     └──> TF-M
                           └──> Verifies App signature
                                 └──> Application
```

**Chain of Trust:** Each stage verifies the next

---

## PSA Certification

### Security Levels

| Level | Requirements | Target |
|-------|--------------|--------|
| **1** | Basic protection | Consumer IoT |
| **2** | Substantial protection | Smart home |
| **3** | High protection | Critical infrastructure |

**Certification validates security claims**

---

## TF-M vs Custom Solutions

| Feature | TF-M | Custom |
|---------|------|--------|
| Development time | ⚡ Weeks | 🐌 Months |
| Testing | ✅ Extensive | ❓ Varies |
| Certification | ✅ PSA Certified | ❌ Self-certify |
| Updates | ✅ Community | ❌ Your team |
| Cost | 💰 Free | 💰💰💰 Expensive |

**Use TF-M unless you have specific requirements**

---

## Ecosystem Support

### Industry Adoption

**Silicon Vendors:**
- ST Microelectronics
- NXP Semiconductors
- Nordic Semiconductor
- Cypress/Infineon

**IDEs:**
- ARM Keil MDK
- IAR Embedded Workbench
- ARM GCC (free!)

---

## Toolchain Setup

### Required Tools

```bash
# ARM GCC Compiler
sudo apt install gcc-arm-none-eabi

# CMake build system
sudo apt install cmake

# OpenOCD for debugging
sudo apt install openocd

# Python for build scripts
sudo apt install python3 python3-pip
```

---

## STM32CubeMX Configuration

### TrustZone Setup

![width:900px](https://via.placeholder.com/900x500/ffffff/000000?text=STM32CubeMX+TrustZone+Config)

**Steps:**
1. Enable TrustZone
2. Configure SAU regions
3. Assign peripherals to Secure/NS
4. Generate project

---

## Memory Configuration

### Linker Script Example

```ld
MEMORY
{
  FLASH_S  (rx)  : ORIGIN = 0x08000000, LENGTH = 256K
  FLASH_NSC (rx) : ORIGIN = 0x08040000, LENGTH = 8K
  FLASH_NS (rx)  : ORIGIN = 0x08042000, LENGTH = 248K

  RAM_S (rwx)    : ORIGIN = 0x30000000, LENGTH = 64K
  RAM_NS (rwx)   : ORIGIN = 0x30010000, LENGTH = 192K
}

SECTIONS
{
  .gnu.sgstubs : { *(.gnu.sgstubs*) } > FLASH_NSC
}
```

---

## Build Process

```
┌──────────────┐     ┌──────────────┐
│ Compile      │────>│ secure.elf   │
│ Secure Code  │     │              │
└──────────────┘     └──────┬───────┘
                            │
┌──────────────┐     ┌──────▼───────┐
│ Compile      │────>│ nonsecure.elf│
│ NS Code      │     │              │
└──────────────┘     └──────┬───────┘
                            │
                     ┌──────▼───────┐
                     │  merged.hex  │
                     │  (Final)     │
                     └──────────────┘
```

---

## Debugging TrustZone

### Separate Debug Sessions

```bash
# Terminal 1: Start OpenOCD
openocd -f board/stm32u5x.cfg

# Terminal 2: GDB for Secure
arm-none-eabi-gdb secure.elf
(gdb) target remote :3333
(gdb) monitor halt
(gdb) load
(gdb) continue

# Terminal 3: GDB for Non-Secure (optional)
arm-none-eabi-gdb nonsecure.elf
```

---

## Lab 01: Hello TrustZone

### First TrustZone Program

**Goal:** Create minimal S + NS application

**Steps:**
1. Configure TrustZone with STM32CubeMX
2. Write secure LED blink function
3. Call from non-secure main
4. Flash and run

**Expected Output:** LED blinks

---

## Lab 02: Interactive Demo

### Hands-On Workshop

**Features:**
- Button triggers secure function
- LED indicates Secure vs NS execution
- UART debug messages
- Security violation demo

**Duration:** 60 minutes

---

## Troubleshooting: Build Errors

### Common Issues

**Error:** `undefined reference to 'secure_function'`
**Fix:** Add function to NSC exports in linker script

**Error:** `HardFault on function call`
**Fix:** Check SAU configuration, verify NSC region

**Error:** `SecureFault exception`
**Fix:** NS code trying to access secure memory directly

---

## Troubleshooting: Flash Errors

### OpenOCD Issues

**Error:** `Can't connect to target`
**Fix:** Check ST-LINK connection, update firmware

**Error:** `Flash write failed`
**Fix:** Unlock flash, disable read protection

**Error:** `TrustZone not enabled`
**Fix:** Use STM32CubeProgrammer to enable TZEN option byte

---

## Key Concepts Review

### What We Learned

✅ TrustZone-M provides **hardware isolation**
✅ SAU/IDAU enforce **security boundaries**
✅ NSC regions provide **secure entry points**
✅ CMSE intrinsics enable **safe S/NS interaction**
✅ TF-M provides **production-ready framework**

---

## Section Summary

### Skills Acquired

✓ Understand TrustZone-M architecture
✓ Configure SAU and memory partitioning
✓ Implement NSC functions
✓ Use CMSE intrinsics
✓ Validate pointers from NS code
✓ Build and debug TrustZone applications

**You can now create secure embedded systems!**

---

## Next Section Preview

### Section 2: PSA Core Services

**Topics:**
- PSA Crypto API (encryption, hashing, signing)
- Secure Storage (ITS and PS)
- Initial Attestation
- Integration examples

**Labs:**
- Lab 03: Cryptographic operations
- Lab 04: Secure credential storage
- Lab 05: Device attestation

---

## Additional Resources

### Documentation

- **ARM TrustZone-M:** developer.arm.com/trustzone-m
- **TF-M GitHub:** github.com/TrustedFirmwareM/trusted-firmware-m
- **PSA Certified:** psacertified.org
- **STM32 Security:** st.com/stm32trust

### Community
- TF-M mailing list
- ARM Community forums

---

<!-- _class: lead -->
<!-- _paginate: false -->

# Questions?

**Contact:** training@example.com
**Course Website:** secure-fm-training.com

**Next:** Section 2 - PSA Core Services

---

**End of Section 1**
*Total Slides: 50*
*Estimated Duration: 2.5 hours*

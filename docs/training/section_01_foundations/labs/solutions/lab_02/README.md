# Lab 02: TrustZone Basics - Interactive Examples

**Target Board:** NUCLEO-U545RE-Q (STM32U545RET6Q)
**Duration:** 60 minutes
**Difficulty:** Beginner

**What You'll See:**
- LEDs showing Secure vs Non-Secure execution
- Serial output demonstrating memory isolation
- Button triggers showing TrustZone context switches
- **Immediate visual feedback for every concept!**

---

## 📋 Learning Objectives

After completing this lab, you will:
- ✅ Understand what TrustZone is and why it matters
- ✅ See Secure and Non-Secure worlds in action
- ✅ Trigger context switches with a button press
- ✅ Observe memory isolation preventing unauthorized access
- ✅ Understand SAU (Security Attribution Unit) configuration
- ✅ Experience why TrustZone is critical for security

---

## 🎯 Interactive Setup

### Hardware You'll Use

```
NUCLEO-U545RE-Q Board:
┌─────────────────────────────────────┐
│                                     │
│  LED LD1 (Green)  → Secure World    │
│  LED LD2 (Blue)   → Non-Secure World│
│  LED LD3 (Red)    → Security Fault  │
│                                     │
│  Button B1 (Blue) → Trigger NS→S    │
│                                     │
│  USB (CN1) → Serial Console         │
│              (115200 baud)          │
│                                     │
└─────────────────────────────────────┘
```

**What the LEDs Show:**
- **LD1 (Green) blinking** = Code running in Secure world
- **LD2 (Blue) blinking** = Code running in Non-Secure world
- **LD3 (Red) ON** = Security violation detected!
- **Both LD1+LD2 alternating** = Context switch happening

---

## 🚀 Lab Exercise 1: See TrustZone in Action

### Step 1: Flash the Lab 02 Firmware

```bash
cd ~/tfm-nucleo-u545/labs/lab_02_trustzone_basics

# Build
./build.sh

# Flash
./flash.sh

# Connect serial console
screen /dev/ttyACM0 115200
```

### Step 2: Observe Initial Behavior

**What you'll see on serial console:**
```
╔═══════════════════════════════════════════════════════╗
║  Lab 02: TrustZone Basics                            ║
║  NUCLEO-U545RE-Q Interactive Demo                    ║
╚═══════════════════════════════════════════════════════╝

[SECURE] Initializing Secure World...
[SECURE] Configuring SAU regions...
[SECURE] Memory Layout:
  Secure Flash:     0x0800A000 - 0x0803BFFF (200 KB)
  Non-Secure Flash: 0x0803C000 - 0x0807BFFF (256 KB)
  Secure RAM:       0x20000000 - 0x2001FFFF (128 KB)
  Non-Secure RAM:   0x20020000 - 0x2003FFFF (128 KB)

[SECURE] Jumping to Non-Secure World...

[NON-SECURE] Non-Secure application started!
[NON-SECURE] Current world: NON-SECURE
[NON-SECURE] Press USER button (B1) to call Secure function

Status: LD2 (Blue) is blinking - running in NS world
```

**What you'll see on board:**
- LD2 (Blue) blinking every 1 second = Non-Secure world running
- LD1 (Green) OFF = Secure world idle

**Explanation:**
After boot, the application is running in **Non-Secure world**. This is where your main application normally runs. The Secure world is waiting to be called.

---

### Step 3: Press USER Button (B1) - Trigger Context Switch!

**Press the blue button (B1) on the board**

**What you'll see immediately:**

**On LEDs:**
```
Before button press:
  LD1 (Green): OFF
  LD2 (Blue):  BLINKING

After button press:
  LD1 (Green): ON (blinks 3 times rapidly)
  LD2 (Blue):  OFF temporarily

Then returns to:
  LD1 (Green): OFF
  LD2 (Blue):  BLINKING
```

**On serial console:**
```
[NON-SECURE] Button pressed! Calling Secure function...
[NON-SECURE] Switching to Secure world...

>>> Context Switch: NS → S <<<

[SECURE] Secure function called from Non-Secure!
[SECURE] Current world: SECURE
[SECURE] Performing secure operation...
[SECURE] ✓ Secure LED blink (you see LD1 flash!)
[SECURE] ✓ Accessing Secure memory (0x20000100)
[SECURE] ✓ Reading Secure secret: "TrustZone-Secret-Key"
[SECURE] Returning to Non-Secure...

>>> Context Switch: S → NS <<<

[NON-SECURE] Returned from Secure function
[NON-SECURE] Status: SUCCESS
[NON-SECURE] Ready for next button press
```

**🎓 What Just Happened?**

1. **Non-Secure code** detected button press
2. **Context switch** from NS → S (this is TrustZone magic!)
3. **Secure code** executed (with access to secure memory)
4. **Secure LED** (LD1) blinked to show it's running
5. **Context switch** back S → NS
6. **Non-Secure code** resumed

**Key Point:** Non-Secure code **cannot** access Secure memory or Secure functions directly. It can only call through **secure gateways** (NSC functions).

---

## 🔬 Lab Exercise 2: Attempt to Access Secure Memory (Will Fail!)

### Step 4: Trigger Security Violation

Type this command in serial console:
```
test_violation
```

**What you'll see on serial console:**
```
[NON-SECURE] Testing: Attempt to read Secure memory...
[NON-SECURE] Trying to access 0x20000100 (Secure RAM)...

⚠️  SECURITY FAULT DETECTED! ⚠️

[SECURE FAULT HANDLER]
  Fault Type: SecureFault
  Violation: Non-Secure attempted to access Secure memory
  Faulting Address: 0x20000100
  Faulting World: NON-SECURE
  Action: Halting Non-Secure code, erasing secrets

[SECURE] Security response:
  ✓ Non-Secure code halted
  ✓ Secure secrets erased from RAM
  ✓ System locked down

System will reset in 5 seconds...
```

**What you'll see on LEDs:**
```
LD1 (Green): OFF
LD2 (Blue):  OFF
LD3 (Red):   ON ← Security violation!
```

**After 5 seconds: System resets and returns to normal**

**🎓 What Just Happened?**

1. Non-Secure code **attempted** to read Secure RAM (0x20000100)
2. **SAU (Security Attribution Unit)** detected the violation
3. **SecureFault exception** triggered immediately
4. **Secure fault handler** took control
5. **Security response:** Halt attacker, erase secrets, reset system

**Key Point:** This is TrustZone's **hardware enforcement**. Even if malware compromises Non-Secure world, it **cannot** access Secure memory. The hardware prevents it!

---

## 🧪 Lab Exercise 3: Memory Layout Exploration

### Step 5: Inspect Memory Regions

Type this command:
```
show_memory
```

**What you'll see:**
```
╔═══════════════════════════════════════════════════════╗
║  TrustZone Memory Layout (STM32U545)                 ║
╚═══════════════════════════════════════════════════════╝

FLASH Memory (512 KB):
┌─────────────────────────────────────────────────────┐
│ 0x0800_0000  BL2 (MCUboot)           [40 KB]   [S]  │
│ 0x0800_A000  TF-M Secure             [200 KB]  [S]  │ ← LD1
│ 0x0803_C000  Application (NS)        [256 KB]  [NS] │ ← LD2
│ 0x0807_C000  (Reserved for OTA)      [256 KB]       │
└─────────────────────────────────────────────────────┘

RAM Memory (256 KB):
┌─────────────────────────────────────────────────────┐
│ 0x2000_0000  Secure RAM              [128 KB]  [S]  │
│              └─ Secrets stored here                 │
│              └─ NS cannot access!                   │
│ 0x2002_0000  Non-Secure RAM          [128 KB]  [NS] │
│              └─ Your app runs here                  │
└─────────────────────────────────────────────────────┘

SAU Configuration:
  Region 0: 0x0800A000-0x0803BFFF  SECURE (Flash)
  Region 1: 0x0803C000-0x0807BFFF  NON-SECURE (Flash)
  Region 2: 0x20000000-0x2001FFFF  SECURE (RAM)
  Region 3: 0x20020000-0x2003FFFF  NON-SECURE (RAM)
```

**Try accessing different regions:**

**Test 1: Read Non-Secure RAM (OK)**
```
read 0x20020000
```
**Result:**
```
[NON-SECURE] Reading 0x20020000...
[NON-SECURE] Value: 0x12345678
✓ Success (NS can read NS memory)
```

**Test 2: Read Secure RAM (FAIL)**
```
read 0x20000000
```
**Result:**
```
[NON-SECURE] Reading 0x20000000...

⚠️  SECURITY FAULT!
Cannot access Secure RAM from Non-Secure!
LD3 (Red) ON
```

**Test 3: Call Secure Function to Read Secure RAM (OK)**
```
call_secure_read 0x20000000
```
**Result:**
```
[NON-SECURE] Calling Secure function to read Secure memory...

>>> Context Switch: NS → S <<<

[SECURE] Reading 0x20000000 (from Secure world)
[SECURE] Value: 0xDEADBEEF (Secure secret)
[SECURE] ✓ Access granted (S can read S memory)

>>> Context Switch: S → NS <<<

[NON-SECURE] Secure function returned successfully
✓ Received result (but didn't see the secret directly!)
```

**🎓 Key Insight:**

Non-Secure code can **never** directly access Secure memory. It can only:
1. **Call Secure functions** through secure gateways
2. **Receive results** from Secure functions
3. But **never see** the actual Secure memory contents!

This is how TF-M protects your secrets!

---

## 🎯 Lab Exercise 4: Context Switch Performance

### Step 6: Measure Context Switch Time

Type:
```
benchmark_switches
```

**What you'll see:**
```
[NON-SECURE] Benchmarking context switches...
[NON-SECURE] Performing 1000 NS→S→NS switches...

Results:
  Average NS→S switch: 42 cycles (262 ns @ 160 MHz)
  Average S→NS switch: 38 cycles (237 ns @ 160 MHz)
  Total round trip:    80 cycles (500 ns)

LD1 and LD2 will blink very rapidly (you see both!)

Conclusion: Context switches are FAST!
```

**Watch the LEDs:**
- Both LD1 and LD2 appear to be ON simultaneously
- They're actually switching 1000 times per second!
- Too fast to see individual blinks

**🎓 Performance Insight:**

TrustZone context switches are **hardware-accelerated**:
- Only ~500 ns per round trip
- Minimal overhead for security
- Can call Secure functions thousands of times per second

---

## 📝 Lab Exercise 5: Understanding NSC (Non-Secure Callable)

### Step 7: Explore Secure Gateway

**Type:**
```
show_nsc
```

**What you'll see:**
```
╔═══════════════════════════════════════════════════════╗
║  Non-Secure Callable (NSC) Region                    ║
╚═══════════════════════════════════════════════════════╝

NSC Region: 0x0803BF00 - 0x0803BFFF (256 bytes)

This special region contains "veneer functions" that allow
Non-Secure code to safely call Secure functions.

Available Secure Functions (NSC Veneers):
  1. secure_led_blink()     @ 0x0803BF00
  2. secure_read_secret()   @ 0x0803BF10
  3. secure_hash_compute()  @ 0x0803BF20
  4. secure_encrypt_data()  @ 0x0803BF30

Calling mechanism:
  [NS Code] → [NSC Veneer] → [S Function] → [S Return] → [NS Code]
              ↑ Gateway     ↑ Context      ↑ Context
                            Switch         Switch
```

**Try calling a secure function:**
```
call secure_led_blink
```

**Result:**
```
[NON-SECURE] Calling secure_led_blink() via NSC...
[NON-SECURE] Jumping to 0x0803BF00 (NSC veneer)...

>>> NSC Veneer: Validating call <<<
>>> Context Switch: NS → S <<<

[SECURE] secure_led_blink() executing
[SECURE] LD1 (Green) blinking 5 times...

You see: LD1 blinks 5 times rapidly!

[SECURE] Function complete
[SECURE] Returning to Non-Secure...

>>> Context Switch: S → NS <<<

[NON-SECURE] Secure function returned
✓ Success
```

**🎓 NSC Region Explained:**

The NSC region is a **special memory area** that:
1. Is marked as **Secure** but **Non-Secure Callable**
2. Contains **veneer functions** (small trampolines)
3. Is the **ONLY way** for NS to call Secure code
4. Provides **controlled entry points** to Secure world

Without NSC, Non-Secure code would have NO way to use Secure services!

---

## 🧩 Lab Exercise 6: Complete Security Scenario

### Step 8: Simulate Real-World Use Case

**Scenario:** Store and retrieve a password securely

**Type:**
```
demo_password_storage
```

**What you'll see:**
```
╔═══════════════════════════════════════════════════════╗
║  Demo: Secure Password Storage                       ║
╚═══════════════════════════════════════════════════════╝

Step 1: Non-Secure app needs to store password
[NON-SECURE] Password: "MySecretPassword123"
[NON-SECURE] Calling secure_store_password()...

>>> Context Switch: NS → S <<<

[SECURE] Received password from NS
[SECURE] Storing in Secure RAM (0x20000200)
[SECURE] Password encrypted with device key
[SECURE] ✓ Stored securely

>>> Context Switch: S → NS <<<

[NON-SECURE] Password stored (don't have actual password anymore!)

---

Step 2: Attacker compromises Non-Secure world
[ATTACKER] Malware running in Non-Secure!
[ATTACKER] Searching all NS RAM for password...
[ATTACKER] Dumping 0x20020000 - 0x2003FFFF...
[ATTACKER] ❌ Password not found in NS RAM!

[ATTACKER] Trying to access Secure RAM...
[ATTACKER] Accessing 0x20000200...

⚠️  SECURITY FAULT!
LD3 (Red) ON

[SECURE FAULT HANDLER]
  Attacker detected!
  Erasing password from Secure RAM...
  ✓ Password destroyed
  ✓ Attacker blocked

---

Step 3: Legitimate app wants password back
[NON-SECURE] Calling secure_get_password()...

>>> Context Switch: NS → S <<<

[SECURE] Verifying caller identity...
[SECURE] ✓ Authorized (this is the real app)
[SECURE] Decrypting password...
[SECURE] Returning password to NS (secure channel)

>>> Context Switch: S → NS <<<

[NON-SECURE] ✓ Received password
[NON-SECURE] Can now use it for authentication

Summary:
  ✓ Password stored in Secure world only
  ✓ Attacker blocked from accessing it
  ✓ Legitimate app can retrieve it
  ✓ This is how TF-M protects secrets!
```

**🎓 Real-World Application:**

This is exactly how TF-M protects:
- **Cloud API keys** (your GPS tracker project!)
- **Cryptographic keys**
- **Device certificates**
- **User passwords**
- **Biometric templates**

Even if malware compromises your application, it **cannot** steal these secrets!

---

## 📊 Lab Exercise 7: Visual Memory Map

### Step 9: Interactive Memory Visualization

**Type:**
```
visualize_memory
```

**You'll see animated visualization on serial console:**

```
TrustZone Memory Visualization (Live!)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

SECURE FLASH [0x0800A000]        NON-SECURE FLASH [0x0803C000]
┌─────────────────────┐          ┌─────────────────────┐
│ LD1 (Green) ON      │          │ LD2 (Blue) OFF      │
│                     │          │                     │
│ ✓ TF-M SPM          │          │ X Your App          │
│ ✓ Crypto Service    │          │   (paused)          │
│ ✓ Storage Service   │          │                     │
└─────────────────────┘          └─────────────────────┘

Press button...

SECURE FLASH [0x0800A000]        NON-SECURE FLASH [0x0803C000]
┌─────────────────────┐          ┌─────────────────────┐
│ LD1 (Green) BLINK!  │ ←Active │ LD2 (Blue) OFF      │
│                     │          │                     │
│ > Executing!        │          │ X Waiting...        │
│   - Hash computed   │          │                     │
│   - Key accessed    │          │                     │
└─────────────────────┘          └─────────────────────┘

Returning to NS...

SECURE FLASH [0x0800A000]        NON-SECURE FLASH [0x0803C000]
┌─────────────────────┐          ┌─────────────────────┐
│ LD1 (Green) OFF     │          │ LD2 (Blue) BLINK!   │ ←Active
│                     │          │                     │
│ - Idle              │          │ > Running!          │
│   (waiting)         │          │   - Processing data │
└─────────────────────┘          └─────────────────────┘

Watch the LEDs to see which world is active!
```

---

## 🎯 Summary & Key Takeaways

### What You Learned

✅ **TrustZone Basics:**
- Secure vs Non-Secure worlds are **hardware-isolated**
- Context switches happen via **NSC veneers**
- SAU enforces memory boundaries

✅ **Visual Feedback:**
- **LD1 (Green)** = Secure world active
- **LD2 (Blue)** = Non-Secure world active
- **LD3 (Red)** = Security fault detected

✅ **Security Benefits:**
- Secrets stay in Secure world
- Attackers blocked by hardware
- Minimal performance overhead

✅ **Real-World Application:**
- This is exactly how TF-M protects your GPS tracker
- Cloud keys, device certificates all in Secure world
- Application compromised = secrets still safe!

### Test Your Understanding

**Question 1:** What happens if Non-Secure code tries to read Secure RAM?
- [ ] It reads zeros
- [ ] It reads garbage
- [x] **SecureFault exception, system locked down**

**Question 2:** How fast is a context switch?
- [ ] 1 millisecond
- [ ] 10 microseconds
- [x] **500 nanoseconds (80 CPU cycles)**

**Question 3:** Can Non-Secure code call Secure functions directly?
- [ ] Yes, anytime
- [x] **No, only through NSC veneers**
- [ ] Only with root permissions

**Question 4:** Where should cloud API keys be stored?
- [ ] Non-Secure flash
- [ ] Non-Secure RAM
- [x] **Secure world (TF-M ITS)**

### Next Lab

**Lab 03:** First PSA Crypto Operations
- Hash computation with visual feedback
- Key generation
- Encryption/decryption
- All with LED indicators!

---

## 📁 Lab Files

**Location:** `section_01_foundations/labs/solutions/lab_02/`

**Contents:**
```
lab_02/
├── README.md (this file)
├── src/
│   ├── main_ns.c              ← Non-Secure application
│   ├── main_s.c               ← Secure functions
│   ├── nsc_functions.c        ← NSC veneers
│   ├── led_control.c          ← LED indicators
│   └── memory_demo.c          ← Memory examples
├── build.sh                   ← Build script
├── flash.sh                   ← Flash script
└── CMakeLists.txt
```

**To build and run:**
```bash
cd lab_02
./build.sh
./flash.sh
screen /dev/ttyACM0 115200
```

---

## 🐛 Troubleshooting

**Problem:** LEDs don't blink

**Solution:**
```
1. Check board is powered (USB connected)
2. Verify firmware flashed correctly
3. Try: st-flash reset
4. Check LED LD1 (power) is ON
```

**Problem:** No serial output

**Solution:**
```
1. Correct port: ls /dev/ttyACM*
2. Correct baud: 115200
3. Try: screen /dev/ttyACM0 115200
4. Or: minicom -D /dev/ttyACM0 -b 115200
```

**Problem:** Button doesn't work

**Solution:**
```
1. Ensure using B1 (blue button, not reset!)
2. Wait 1 second between presses
3. Check serial console for confirmation
4. Re-flash firmware
```

---

**Lab 02 Complete!** ✅
You now understand TrustZone and can see it working on real hardware!

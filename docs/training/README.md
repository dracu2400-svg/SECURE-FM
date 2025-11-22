# Trusted Firmware-M (TF-M) Complete Training Package
## From Zero to Secure IoT Product

---

## 📚 Welcome to TF-M Training!

This comprehensive training package takes you from TF-M fundamentals to building production-ready secure IoT products. The training is designed to be:

- ✅ **Progressive**: Start simple, build to complex
- ✅ **Hands-on**: Learn by doing with practical labs
- ✅ **Real-world**: Build actual products, not just demos
- ✅ **Complete**: Everything from theory to deployment

**Training Duration:** 5-7 days (40-50 hours)
**Difficulty Levels:** Beginner → Intermediate → Advanced → Expert

---

## 📖 Training Materials

### 1. Main Training Guide
**File:** [`TFM_COMPLETE_TRAINING_GUIDE.md`](./TFM_COMPLETE_TRAINING_GUIDE.md)

The core training curriculum covering all TF-M concepts:

**Part 1: Foundations (Day 1)**
- Module 1: Introduction to Secure IoT
- Module 2: ARM TrustZone Technology
- Module 3: PSA (Platform Security Architecture)
- Module 4: TF-M Architecture Overview
- Module 5: Development Environment Setup

**Part 2: Core Concepts (Day 2)**
- Module 6: Secure Partition Manager (SPM)
- Module 7: Isolation Levels and Security Models
- Module 8: PSA APIs - Client and Service
- Module 9: Build System and Configuration
- Module 10: First TF-M Application

**Part 3: Secure Services (Day 3)**
- Module 11: Cryptographic Services
- Module 12: Secure Storage - ITS and PS
- Module 13: Initial Attestation
- Module 14: Platform Services
- Module 15: Firmware Update Service

**Part 4: Secure Boot and MCUboot (Day 4)**
- Module 16: Secure Boot Architecture
- Module 17: MCUboot Deep Dive
- Module 18: Image Signing and Verification
- Module 19: Rollback Protection
- Module 20: Encrypted Firmware Images

**Part 5: Advanced Topics (Day 5)**
- Module 21: Porting TF-M to New Hardware
- Module 22: Creating Custom Secure Partitions
- Module 23: Multi-Core Configurations
- Module 24: Physical Attack Mitigation
- Module 25: Performance Optimization

**Part 6: Real-World Projects (Days 6-7)**
- Module 26: Project 1 - STM32U5 Secure Tracker
- Module 27: Project 2 - NRF52840 Secure Tracker

---

### 2. Laboratory Exercises
**File:** [`TFM_TRAINING_LABS.md`](./TFM_TRAINING_LABS.md)

Hands-on practical exercises for each module:

**Lab 1:** Environment Setup and First Build
- Install toolchain
- Build TF-M for AN521 platform
- Run on Fixed Virtual Platform (FVP)

**Lab 2:** Understanding TrustZone Memory Layout
- Examine platform memory configuration
- Analyze SAU/MPU settings
- Create memory map visualizations

**Lab 3:** PSA Crypto API Basics
- Random number generation
- Hash computation (SHA-256)
- Symmetric encryption (AES)
- HMAC implementation

**Lab 4:** Secure Storage (ITS and PS)
- Basic ITS operations
- Write-once flag testing
- Device configuration management
- Protected Storage with encryption

**... (25 total labs covering all modules)**

Each lab includes:
- Clear objectives
- Step-by-step instructions
- Expected results
- Troubleshooting tips
- Challenge exercises

---

### 3. MCUboot Complete Guide
**File:** [`MCUBOOT_COMPLETE_GUIDE.md`](./MCUBOOT_COMPLETE_GUIDE.md)

Deep dive into MCUboot secure bootloader:

**Contents:**
1. Introduction to MCUboot
2. MCUboot Architecture
3. Image Format and Signing
4. Swap Mechanisms (Overwrite, Swap, Move)
5. Rollback Protection
6. Encrypted Images
7. Multi-Image Boot
8. Integration with TF-M
9. Hands-On Labs
10. Advanced Topics

**What You'll Learn:**
- How secure boot works
- Image signing with ECDSA/RSA/ED25519
- Implementing OTA updates
- Rollback attack prevention
- Encrypted firmware distribution
- Managing multiple images (Secure + Non-Secure)

---

### 4. Project 1: STM32U5 Secure Tracker
**File:** [`PROJECT_STM32U5_SECURE_TRACKER.md`](./PROJECT_STM32U5_SECURE_TRACKER.md)

Build a production-ready GPS tracker with cellular connectivity:

**Hardware:**
- STM32U585ZI-Q Nucleo-64 board (~$25)
- SimCom A7672SA 4G LTE module (~$20)
- LSM6DSO IMU sensor (~$5)

**Features:**
- ✅ Secure boot with MCUboot
- ✅ TLS 1.3 communication to cloud
- ✅ Encrypted credential storage
- ✅ GPS tracking and geofencing
- ✅ Motion detection
- ✅ OTA firmware updates
- ✅ Device attestation
- ✅ Low power modes

**What You'll Build:**
1. Complete hardware setup
2. A7672SA 4G driver (UART, AT commands)
3. LSM6DSO IMU driver (I2C)
4. GPS data parser (NMEA)
5. Cloud MQTT client with TLS
6. Secure credential management
7. OTA update mechanism
8. Power management system

**Skills Gained:**
- Real hardware integration
- Cellular modem programming
- Sensor interfacing
- Cloud connectivity
- Production security practices

---

### 5. Project 2: NRF52840 Secure Tracker
**File:** [`PROJECT_NRF52840_SECURE_TRACKER.md`](./PROJECT_NRF52840_SECURE_TRACKER.md)

Build an advanced tracker with user recognition and Bluetooth:

**Hardware:**
- nRF52840-DK board (~$40)
- SimCom A7672SA 4G LTE module (~$20)
- LSM6DSO IMU sensor (~$5)

**Unique Features:**
- ✅ **User Recognition**: IMU-based activity classification
- ✅ **TinyML**: On-device machine learning
- ✅ **Bluetooth LE 5.2**: Local device pairing
- ✅ **Ultra Low Power**: 10+ days battery life
- ✅ **Activity Tracking**: Walking, running, sitting detection
- ✅ **Multi-Protocol**: BLE + LTE concurrent operation

**What You'll Build:**
1. TinyML activity classifier
2. Motion pattern recognition system
3. Secure user profile storage
4. BLE GATT services
5. Secure BLE pairing
6. Companion mobile app
7. Cloud data synchronization
8. Battery optimization

**Advanced Skills:**
- Embedded machine learning (TensorFlow Lite Micro)
- Bluetooth Low Energy programming
- Activity recognition algorithms
- Nordic nRF SDK integration
- Mobile app development basics

---

## 🎯 Learning Paths

### Path 1: Security Engineer
**Focus:** Security architecture, threat modeling, certification

```
Day 1: Modules 1-5 (Foundations)
Day 2: Modules 6-8 (Core security concepts)
Day 3: Modules 11-13 (Crypto, storage, attestation)
Day 4: Modules 16-20 (Secure boot, MCUboot)
Day 5: Module 24 (Physical attack mitigation)
```

**Labs:** 1, 2, 3, 4, security-focused challenges

---

### Path 2: Embedded Systems Developer
**Focus:** Practical implementation, drivers, integration

```
Day 1: Modules 1-5 (Foundations)
Day 2: Modules 6-10 (Core concepts + first app)
Day 3: Modules 11-15 (All secure services)
Day 4: Modules 16-17 (Boot basics)
Day 5: Modules 21-22 (Porting + custom partitions)
Days 6-7: Project 1 or 2 (Full implementation)
```

**Labs:** All labs, focus on hands-on implementation

---

### Path 3: IoT Product Manager
**Focus:** Architecture, capabilities, trade-offs

```
Day 1: Modules 1-4 (Concepts + architecture)
Day 2: Modules 6-7, 11-15 (Understand services)
Day 3: Modules 16-20 (Update mechanisms)
Day 4: Project 1 overview (Real product example)
Day 5: Module 25, certification path
```

**Labs:** Labs 1, 3, 4 (understand capabilities)

---

### Path 4: Full Stack IoT Developer
**Focus:** Complete product development

```
Day 1: Modules 1-5
Day 2: Modules 6-10
Day 3: Modules 11-15
Day 4: Modules 16-20
Day 5: Modules 21-25
Days 6-7: Complete Project 1
Days 8-9: Complete Project 2
```

**Labs:** All labs + all challenges

---

## 🛠️ Prerequisites

### Required Knowledge
- ✅ C programming (intermediate level)
- ✅ Basic embedded systems concepts
- ✅ Familiarity with command line (Linux/Mac)
- ⚠️ Helpful but not required:
  - ARM Cortex-M architecture
  - Cryptography basics
  - CMake build system

### Required Software
- **OS:** Linux (Ubuntu 20.04+), macOS, or WSL2 on Windows
- **Toolchain:** ARM GCC Embedded Toolchain
- **Build:** CMake 3.21+, Python 3.8+
- **Debug:** OpenOCD or J-Link
- **Optional:** VS Code with CMake extension

### Required Hardware (for projects)

**Minimum (for one project):**
- STM32U585 Nucleo-64 **OR** nRF52840-DK
- SimCom A7672SA module
- LSM6DSO sensor (or IKS01A3 shield)
- SIM card with data plan
- Antennas, jumper wires, breadboard

**Full Setup (for both projects):**
- Both development boards
- Shared modules (A7672SA, LSM6DSO)
- Total cost: ~$100-120

---

## 📅 Suggested Training Schedule

### Week 1: Intensive Training (Corporate/Bootcamp)

**Monday:**
- 09:00-10:00: Module 1-2 (IoT Security, TrustZone)
- 10:00-11:00: Lab 1 (Environment Setup)
- 11:00-12:00: Module 3-4 (PSA, TF-M Architecture)
- 13:00-14:00: Module 5 (Development Setup)
- 14:00-16:00: Lab 2 (Memory Layout)
- 16:00-17:00: Q&A, Review

**Tuesday:**
- 09:00-10:30: Module 6-7 (SPM, Isolation)
- 10:30-12:00: Module 8 (PSA APIs)
- 13:00-14:00: Module 9-10 (Build System, First App)
- 14:00-17:00: Labs 3-4 (Crypto, Storage)

**Wednesday:**
- 09:00-12:00: Modules 11-15 (All Secure Services)
- 13:00-17:00: Labs 5-10 (Service implementation)

**Thursday:**
- 09:00-12:00: Modules 16-20 (MCUboot Deep Dive)
- 13:00-17:00: Labs 11-15 (Boot, signing, updates)

**Friday:**
- 09:00-12:00: Modules 21-25 (Advanced Topics)
- 13:00-17:00: Choose project path
- Planning for Week 2

**Week 2: Project Development**

**Monday-Tuesday:** Project 1 (STM32U5)
- Hardware setup
- Driver development
- Security integration

**Wednesday-Thursday:** Project 2 (NRF52840)
- ML model training
- BLE implementation
- User recognition

**Friday:**
- Project presentations
- Wrap-up & certification

---

### Self-Paced Learning (2-3 Months)

**Weeks 1-2:** Foundations + Core Concepts
- Modules 1-10
- Labs 1-4
- 2-3 hours per day

**Weeks 3-4:** Secure Services
- Modules 11-15
- Labs 5-10
- 2-3 hours per day

**Weeks 5-6:** Secure Boot
- Modules 16-20
- MCUboot guide
- Labs 11-15
- 2-3 hours per day

**Weeks 7-8:** Advanced Topics
- Modules 21-25
- Labs 16-20
- 2-3 hours per day

**Weeks 9-12:** Projects
- Choose 1 or both projects
- Complete implementation
- Document learnings

---

## 🏆 Certification and Assessment

### Knowledge Assessment

After each section, test your knowledge:

**Foundations Quiz:**
- TrustZone concepts
- PSA architecture
- TF-M components

**Implementation Quiz:**
- SPM operation
- PSA API usage
- Secure service integration

**Security Quiz:**
- Threat modeling
- Attack mitigations
- Secure boot flow

### Practical Assessment

**Project Demonstrations:**
1. Demonstrate working secure tracker
2. Explain security decisions
3. Show OTA update process
4. Discuss attack surface

### PSA Certification Path

After completing this training, you'll be prepared for:

**PSA Certified Level 1** (Evaluation Complete)
- Understanding of PSA concepts
- Ability to integrate PSA APIs
- Ready for vendor-specific certification

**Path to Level 2/3:**
- Additional security hardening
- Formal security testing
- Lab evaluation

---

## 📚 Additional Resources

### Official Documentation
- [TF-M Documentation](https://tf-m-user-guide.trustedfirmware.org/)
- [PSA Certified](https://www.psacertified.org/)
- [MCUboot Documentation](https://www.mcuboot.com/)

### Community
- [TF-M Mailing List](https://lists.trustedfirmware.org/mailman/listinfo/tf-m)
- [GitHub Discussions](https://github.com/TrustedFirmwareM/trusted-firmware-m/discussions)

### Reference Implementations
- [Example Projects](https://git.trustedfirmware.org/TF-M/tf-m-extras.git/)
- [Platform Ports](https://git.trustedfirmware.org/TF-M/trusted-firmware-m.git/tree/platform)

---

## 🤝 Contributing

Found an issue or have a suggestion? Contributions welcome!

1. Report issues in this repository
2. Submit improvements via pull requests
3. Share your project implementations
4. Help other learners in discussions

---

## 📝 License

This training material is provided as part of the Trusted Firmware-M project.

- **Code examples:** BSD-3-Clause (same as TF-M)
- **Documentation:** Creative Commons BY-SA 4.0

---

## 🎓 About This Training

**Created for:** Engineers, developers, and security professionals working with secure IoT devices

**Maintained by:** TF-M Community Contributors

**Version:** 1.0 (Compatible with TF-M v1.8+)

**Last Updated:** 2024

---

## 🚀 Getting Started

**Ready to begin?**

1. ✅ Check prerequisites
2. ✅ Clone TF-M repository
3. ✅ Install toolchain
4. ✅ Start with [Module 1: Introduction to Secure IoT](./TFM_COMPLETE_TRAINING_GUIDE.md#module-1-introduction-to-secure-iot)
5. ✅ Complete [Lab 1: Environment Setup](./TFM_TRAINING_LABS.md#lab-1-environment-setup-and-first-build)

**Questions?**
- Review the [FAQ](#) (TODO: Add FAQ section)
- Join the community discussion
- Consult the official documentation

---

## 📞 Support

For training support:
- **Documentation Issues:** File issue in this repository
- **TF-M Technical Issues:** [TF-M Bug Tracker](https://developer.trustedfirmware.org/maniphest/)
- **General Questions:** [TF-M Mailing List](https://lists.trustedfirmware.org/mailman/listinfo/tf-m)

---

**Happy Learning! Build Secure IoT Products with Confidence! 🔐**

---

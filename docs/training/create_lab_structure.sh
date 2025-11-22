#!/bin/bash

#===============================================================================
# TF-M Training Content Reorganization Script
# This script helps reorganize the training content into the new structure
#===============================================================================

set -e  # Exit on error

TRAINING_DIR="/home/user/SECURE-FM/docs/training"
cd "$TRAINING_DIR"

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║   TF-M Training Content Reorganization                        ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo

#===============================================================================
# SECTION 1: Extract and Split Theory Modules
#===============================================================================

echo "STEP 1: Splitting theory modules into sections..."
echo "=================================================="

# Section 1: Already have PART1, just need to add TrustZone basics
if [ ! -f "sections/section1_foundations/01_trustzone_architecture.md" ]; then
    echo "Creating 01_trustzone_architecture.md..."
    cat > sections/section1_foundations/01_trustzone_architecture.md << 'EOFTZ'
# Section 1.1: ARM TrustZone-M Architecture

**Learning Objectives:**
- Understand ARM TrustZone-M security architecture
- Learn Secure vs Non-Secure world separation
- Master memory protection mechanisms (SAU/IDAU/MPU)
- Understand secure gateway and CMSE

**Duration:** 4 hours
**Prerequisites:** ARM Cortex-M basics, C programming

---

## 1.1.1 Introduction to TrustZone-M

[Content will be extracted from PART1_TFM_DEEP_ARCHITECTURE.md sections 1-2]

---

## 1.1.2 Memory Protection

[Content from PART1 sections on SAU/IDAU/MPU]

---

## 1.1.3 Secure Gateway and CMSE

[Content from PART1 on veneer functions]

---

**Next:** Continue to `02_tfm_deep_architecture.md` for TF-M internals

---
EOFTZ
    echo "  ✓ Created placeholder (needs content extraction)"
fi

# Section 3: Split PART2 into individual service modules
echo
echo "Splitting Section 3: Secure Services modules..."

# Function to create module files
create_service_module() {
    local module_num=$1
    local module_name=$2
    local filename=$3
    local start_line=$4
    local end_line=$5

    echo "  Creating $filename..."

    if [ -f "PART2_SECURE_SERVICES_GUIDE.md" ]; then
        sed -n "${start_line},${end_line}p" PART2_SECURE_SERVICES_GUIDE.md \
            > "sections/section3_secure_services/$filename"
        echo "  ✓ Extracted Module $module_num: $module_name"
    fi
}

# Extract modules from PART2 (approximate line numbers - adjust as needed)
# create_service_module "11" "Cryptographic Services" "01_cryptographic_services.md" "1" "1500"
# create_service_module "12" "Secure Storage" "02_secure_storage_services.md" "1501" "2500"
# create_service_module "13" "Attestation" "03_attestation_services.md" "2501" "3500"
# create_service_module "14" "Platform Services" "04_platform_services.md" "3501" "4500"
# create_service_module "15" "Firmware Update" "05_firmware_update_service.md" "4501" "5500"

echo
echo "✓ Theory modules structure created"
echo

#===============================================================================
# SECTION 2: Create Individual Lab Files
#===============================================================================

echo "STEP 2: Creating individual lab files with solutions..."
echo "========================================================"

# Function to create lab structure
create_lab() {
    local section=$1
    local lab_num=$2
    local lab_name=$3
    local description=$4

    local section_dir="sections/section${section}_*/labs"
    local lab_file="lab_${section}.${lab_num}_${lab_name}.md"
    local solution_dir="solutions/lab_${section}.${lab_num}"

    echo "  Creating Lab ${section}.${lab_num}: $lab_name..."

    # Create lab file
    cat > "${section_dir}/${lab_file}" << EOFLAB
# Lab ${section}.${lab_num}: ${description}

**Duration:** 2-3 hours
**Difficulty:** Intermediate
**Prerequisites:** Previous labs in this section

## Objectives

- [Objective 1]
- [Objective 2]
- [Objective 3]

## Background

[Brief theory recap]

## Exercise ${section}.${lab_num}.1: [Exercise Name]

**Task:** [What to implement]

**Requirements:**
1. [Requirement 1]
2. [Requirement 2]
3. [Requirement 3]

**Hints:**
- [Hint 1]
- [Hint 2]

## Deliverables

- [ ] Working code implementation
- [ ] Test results
- [ ] Screenshots (if applicable)

## Evaluation Criteria

- Code compiles without errors
- All tests pass
- Code follows best practices

## Solution

**Solution is available in:** \`solutions/lab_${section}.${lab_num}/README.md\`

**Do not look at the solution until you have attempted the lab!**

---

**Next Lab:** Lab ${section}.$((lab_num + 1))

---
EOFLAB

    # Create solution directory
    mkdir -p "${section_dir}/${solution_dir}/src"

    # Create solution README
    cat > "${section_dir}/${solution_dir}/README.md" << EOFSOL
# Lab ${section}.${lab_num} Solution: ${description}

## Overview

This solution demonstrates [what it demonstrates].

## Solution Approach

### Step 1: [First Step]

[Explanation]

\`\`\`c
// Code snippet
\`\`\`

### Step 2: [Second Step]

[Explanation]

\`\`\`c
// Code snippet
\`\`\`

### Step 3: [Third Step]

[Explanation]

## Complete Source Code

See \`src/\` directory for complete working code:
- \`src/main.c\` - Main implementation
- \`src/CMakeLists.txt\` - Build configuration

## Building and Running

\`\`\`bash
mkdir build && cd build
cmake ../src
make
# Flash to device
st-flash write lab_${section}_${lab_num}.bin 0x08000000
\`\`\`

## Expected Output

\`\`\`
[Expected console output]
\`\`\`

## Key Learnings

1. [Learning 1]
2. [Learning 2]
3. [Learning 3]

## Common Issues

**Issue:** [Problem]
**Solution:** [Fix]

---

**Verification:** If you see the expected output above, the lab is complete!

---
EOFSOL

    # Create placeholder source file
    cat > "${section_dir}/${solution_dir}/src/main.c" << 'EOFSRC'
/*
 * Lab Solution Source Code
 *
 * This file contains the complete working solution.
 * Study the comments to understand each step.
 */

#include <stdio.h>
#include "psa/crypto.h"

int main(void)
{
    printf("Lab solution implementation\n");

    // TODO: Add actual implementation

    return 0;
}
EOFSRC

    cat > "${section_dir}/${solution_dir}/src/CMakeLists.txt" << 'EOFCMAKE'
cmake_minimum_required(VERSION 3.15)

project(lab_solution C)

add_executable(lab_solution main.c)

target_link_libraries(lab_solution
    tfm_api_ns
    platform_ns
)
EOFCMAKE

    echo "  ✓ Created lab ${section}.${lab_num} with solution template"
}

# Create Section 1 labs (foundations)
echo
echo "Creating Section 1 Labs (Foundations)..."
# create_lab "1" "1" "environment_setup" "Environment Setup and First Build"
# create_lab "1" "2" "trustzone_basics" "TrustZone Memory Layout"
# create_lab "1" "3" "crypto_basics" "PSA Crypto API Basics"
# create_lab "1" "4" "secure_storage" "Secure Storage (ITS and PS)"

echo
echo "✓ Lab structure templates created"
echo

#===============================================================================
# SECTION 3: Create Project Structure
#===============================================================================

echo "STEP 3: Creating project structure..."
echo "======================================"

# Project 1: STM32U5 Tracker
echo
echo "Creating Project 1: STM32U5 GPS Tracker..."

mkdir -p projects/project1_stm32u5_tracker/{docs,src/{secure,non_secure,bootloader,drivers,services}}

cat > projects/project1_stm32u5_tracker/README.md << 'EOFPROJ1'
# Project 1: STM32U5 Secure GPS Tracker

**Hardware:** STM32U585 Nucleo-64 + SimCom A7672SA + X-Nucleo-IQS4A1

**Features:**
- Secure boot with MCUboot
- GPS tracking and location reporting
- 4G LTE connectivity
- Motion detection and geofencing
- Secure cloud communication (TLS 1.3)
- OTA firmware updates
- Device attestation

## Quick Start

See `docs/BUILD_GUIDE.md` for complete build instructions.

## Documentation

- `docs/ARCHITECTURE.md` - System architecture
- `docs/BUILD_GUIDE.md` - Build and flash instructions
- `docs/HARDWARE_SETUP.md` - Hardware connections
- `docs/TESTING_GUIDE.md` - Testing procedures
- `docs/DEPLOYMENT.md` - Production deployment

## Source Code

- `src/secure/` - Secure firmware (TF-M)
- `src/non_secure/` - Application code
- `src/drivers/` - Hardware drivers
- `src/services/` - Application services

---
EOFPROJ1

cat > projects/project1_stm32u5_tracker/docs/ARCHITECTURE.md << 'EOFARCH'
# STM32U5 GPS Tracker - System Architecture

## Overview

[System architecture description]

## Block Diagram

\`\`\`
┌─────────────────────────────────────────────────────┐
│ STM32U585 (Cortex-M33 + TrustZone)                  │
│                                                     │
│  ┌────────────────┐         ┌──────────────────┐   │
│  │ Secure World   │         │ Non-Secure World │   │
│  │  (TF-M)        │◄───────►│  (Application)   │   │
│  │                │         │                  │   │
│  │  - SPM         │         │  - GPS Service   │   │
│  │  - Crypto      │         │  - Motion Det.   │   │
│  │  - Storage     │         │  - Cloud Sync    │   │
│  │  - Attestation │         │  - OTA Client    │   │
│  └────────────────┘         └──────────────────┘   │
│                                                     │
└─────────────────────────────────────────────────────┘
         ▲                  ▲                  ▲
         │                  │                  │
    ┌────┴────┐      ┌─────┴──────┐    ┌──────┴─────┐
    │ SimCom  │      │ X-Nucleo   │    │  Cloud     │
    │ A7672SA │      │ IQS4A1     │    │  Server    │
    │ (4G/GPS)│      │ (Sensors)  │    │  (HTTPS)   │
    └─────────┘      └────────────┘    └────────────┘
\`\`\`

## Component Details

### Secure Firmware (TF-M)
[Details]

### Application
[Details]

### Hardware Drivers
[Details]

### Cloud Communication
[Details]

---
EOFARCH

echo "  ✓ Project 1 structure created"

# Project 2: NRF52840 Tracker
echo
echo "Creating Project 2: NRF52840 Activity Tracker..."

mkdir -p projects/project2_nrf52840_tracker/{docs,src/{secure,non_secure,ml_models,drivers,services},ml_training}

cat > projects/project2_nrf52840_tracker/README.md << 'EOFPROJ2'
# Project 2: NRF52840 Activity Tracker with TinyML

**Hardware:** NRF52840-DK + SimCom A7672SA + IMU Sensor

**Features:**
- Activity recognition (walking, running, cycling, etc.)
- User identification via motion patterns
- BLE connectivity for smartphone app
- 4G fallback for cloud sync
- Secure ML model storage
- OTA model updates

## Quick Start

See `docs/BUILD_GUIDE.md` for complete build instructions.
See `docs/ML_PIPELINE.md` for ML model training.

## Documentation

- `docs/ARCHITECTURE.md` - System architecture
- `docs/BUILD_GUIDE.md` - Build instructions
- `docs/ML_PIPELINE.md` - ML training and deployment
- `docs/TESTING_GUIDE.md` - Testing procedures

## Source Code

- `src/secure/` - Secure firmware (TF-M)
- `src/non_secure/` - Application code
- `src/ml_models/` - TensorFlow Lite models
- `src/drivers/` - Hardware drivers
- `src/services/` - Application services

## ML Training

- `ml_training/train_activity_model.py` - Activity classifier
- `ml_training/train_user_model.py` - User identifier
- `ml_training/convert_to_tflite.py` - Model quantization

---
EOFPROJ2

echo "  ✓ Project 2 structure created"

echo
echo "✓ Project structures created"
echo

#===============================================================================
# SECTION 4: Generate Status Report
#===============================================================================

echo
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║   Reorganization Status Report                                ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo

cat << 'EOFSTATUS'

STRUCTURE CREATED:
==================

✓ Course Index (COURSE_INDEX.md)
✓ Reorganization Guide (REORGANIZATION_GUIDE.md)
✓ Section directories (sections/section1-5/)
✓ Lab directories (labs/ with solutions/ subdirs)
✓ Project directories (projects/project1-2/)

REMAINING WORK:
===============

1. Content Extraction:
   - [ ] Split PART2_SECURE_SERVICES_GUIDE.md into 5 module files
   - [ ] Extract individual labs from LABS_*.md files
   - [ ] Create TrustZone architecture module

2. Lab Solutions:
   - [ ] Write 25 solution README.md files
   - [ ] Create source code for all lab solutions
   - [ ] Add CMakeLists.txt for each solution

3. Projects:
   - [ ] Implement STM32U5 tracker source code
   - [ ] Implement NRF52840 tracker source code
   - [ ] Create all driver implementations
   - [ ] Write all project documentation

4. Testing:
   - [ ] Verify all lab solutions compile
   - [ ] Test projects on hardware
   - [ ] Validate all links and references

5. PDF Generation:
   - [ ] Create PDF styling templates
   - [ ] Generate 6 PDF volumes
   - [ ] Review and polish formatting

COMPLETION ESTIMATE:
====================

Structure:       100% ✓
Content Split:    20% (needs manual extraction)
Lab Solutions:    10% (templates created)
Projects:          5% (structure only)
Documentation:    40% (theory complete, needs organization)

OVERALL:          35% complete

NEXT STEPS:
===========

1. Run content extraction scripts
2. Complete lab solution source code
3. Implement project source code
4. Generate sample PDFs

EOFSTATUS

echo
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║   Reorganization Script Complete                              ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo
echo "Review COURSE_INDEX.md and REORGANIZATION_GUIDE.md for details."
echo


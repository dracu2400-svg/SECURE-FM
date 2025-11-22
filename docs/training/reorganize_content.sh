#!/bin/bash

echo "=== Reorganizing TF-M Training Content ==="
echo

# Section 1: Foundations
echo "Creating Section 1: Foundations..."
cp PART1_TFM_DEEP_ARCHITECTURE.md sections/section1_foundations/02_tfm_deep_architecture.md

# Section 2: Build & Configuration
echo "Creating Section 2: Build & Configuration..."
# Extract relevant parts from LABS_05_10
head -n 1500 LABS_05_10_BUILD_AND_CONFIG.md > sections/section2_build_configuration/01_build_system_deep_dive.md
tail -n +1501 LABS_05_10_BUILD_AND_CONFIG.md > sections/section2_build_configuration/02_debugging_profiling.md

# Section 3: Secure Services
echo "Creating Section 3: Secure Services..."
cp PART2_SECURE_SERVICES_GUIDE.md sections/section3_secure_services/00_complete_guide.md

# Section 4: Boot & Update
echo "Creating Section 4: Boot & Update..."
if [ -f MCUBOOT_COMPLETE_GUIDE.md ]; then
    cp MCUBOOT_COMPLETE_GUIDE.md sections/section4_boot_update/01_mcuboot_complete_guide.md
fi

# Section 5: Advanced
echo "Creating Section 5: Advanced..."
# Will be created from labs

echo
echo "✓ Content reorganization structure created"
echo "  Next: Create individual lab files and solutions"


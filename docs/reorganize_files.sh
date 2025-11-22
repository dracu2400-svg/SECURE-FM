#!/bin/bash
set -e

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║   Moving Content to Clean Structure                           ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo

# Section 1: Foundations
echo "Section 1: Moving foundation content..."
if [ -f "training_old_backup/PART1_TFM_DEEP_ARCHITECTURE.md" ]; then
    cp "training_old_backup/PART1_TFM_DEEP_ARCHITECTURE.md" \
       "training/section_01_foundations/02_theory_tfm_architecture.md"
    echo "  ✓ Moved TF-M architecture (2097 lines)"
fi

# Section 3: Secure Services - Copy complete guide
echo
echo "Section 3: Moving secure services content..."
if [ -f "training_old_backup/PART2_SECURE_SERVICES_GUIDE.md" ]; then
    cp "training_old_backup/PART2_SECURE_SERVICES_GUIDE.md" \
       "training/section_03_secure_services/00_complete_guide.md"
    echo "  ✓ Moved secure services guide (5400+ lines)"
fi

# Section 4: MCUboot
echo
echo "Section 4: Moving MCUboot content..."
if [ -f "training_old_backup/MCUBOOT_COMPLETE_GUIDE.md" ]; then
    cp "training_old_backup/MCUBOOT_COMPLETE_GUIDE.md" \
       "training/section_04_mcuboot/02_theory_mcuboot_architecture.md"
    echo "  ✓ Moved MCUboot guide"
fi

# Tools: Cloud Server
echo
echo "Tools: Moving cloud server..."
if [ -d "training_old_backup/tools/cloud_server" ]; then
    cp -r "training_old_backup/tools/cloud_server" "training/tools/"
    echo "  ✓ Moved cloud server"
fi

# Move lab files
echo
echo "Moving lab content files..."
if [ -f "training_old_backup/TFM_TRAINING_LABS.md" ]; then
    cp "training_old_backup/TFM_TRAINING_LABS.md" \
       "training/section_01_foundations/labs/00_labs_01_04_combined.md"
    echo "  ✓ Moved Labs 1-4 (to be split)"
fi

if [ -f "training_old_backup/LABS_05_10_BUILD_AND_CONFIG.md" ]; then
    cp "training_old_backup/LABS_05_10_BUILD_AND_CONFIG.md" \
       "training/section_02_build_config/labs/00_labs_05_10_combined.md"
    echo "  ✓ Moved Labs 5-10 (to be split)"
fi

if [ -f "training_old_backup/LABS_11_15_SECURE_SERVICES.md" ]; then
    cp "training_old_backup/LABS_11_15_SECURE_SERVICES.md" \
       "training/section_03_secure_services/labs/00_labs_11_15_combined.md"
    echo "  ✓ Moved Labs 11-15 (to be split)"
fi

if [ -f "training_old_backup/LABS_16_20_BOOT_AND_UPDATE.md" ]; then
    cp "training_old_backup/LABS_16_20_BOOT_AND_UPDATE.md" \
       "training/section_04_mcuboot/labs/00_labs_16_20_combined.md"
    echo "  ✓ Moved Labs 16-20 (to be split)"
fi

if [ -f "training_old_backup/LABS_21_25_ADVANCED.md" ]; then
    cp "training_old_backup/LABS_21_25_ADVANCED.md" \
       "training/section_05_advanced/labs/00_labs_21_25_combined.md"
    echo "  ✓ Moved Labs 21-25 (to be split)"
fi

echo
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║   Content Moved Successfully                                  ║"
echo "╚════════════════════════════════════════════════════════════════╝"


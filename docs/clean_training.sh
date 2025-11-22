#!/bin/bash
set -e

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║   Creating Clean Training Structure                           ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo

# Backup old training folder
echo "Step 1: Backing up old training folder..."
if [ -d "training" ]; then
    mv training training_old_backup
    echo "  ✓ Backed up to training_old_backup/"
fi

# Create new clean structure
echo
echo "Step 2: Creating new clean structure..."

mkdir -p training/{section_01_foundations,section_02_build_config,section_03_secure_services,section_04_mcuboot,section_05_advanced}/labs/solutions
mkdir -p training/{project_01_stm32u5_tracker,project_02_nrf52840_tracker}
mkdir -p training/tools

echo "  ✓ Created section directories"

# Create labs subdirectories
for i in {1..5}; do
    section_name=""
    case $i in
        1) section_name="section_01_foundations" ;;
        2) section_name="section_02_build_config" ;;
        3) section_name="section_03_secure_services" ;;
        4) section_name="section_04_mcuboot" ;;
        5) section_name="section_05_advanced" ;;
    esac
    
    mkdir -p "training/${section_name}/labs/solutions"
done

echo "  ✓ Created lab directories"
echo

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║   Clean Structure Created                                     ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo
echo "New structure:"
tree -L 2 training/
echo


#!/bin/bash

#############################################################################
# TF-M Training Package - Lab to PDF Conversion Script
#
# This script converts all 30 lab README.md files to professional PDF documents
# using Pandoc with LaTeX engine.
#
# Requirements:
#   - pandoc
#   - pdflatex (texlive-latex-base)
#
# Usage:
#   chmod +x convert_all_labs.sh
#   ./convert_all_labs.sh
#
# Output:
#   PDFs saved to: docs/training/labs_pdf/
#############################################################################

set -e  # Exit on error

# Configuration
BASE_DIR="$(dirname "$0")"
OUTPUT_DIR="$BASE_DIR/labs_pdf"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Create output directory
mkdir -p "$OUTPUT_DIR"

echo -e "${BLUE}=== TF-M Lab to PDF Conversion ===${NC}"
echo ""

# Check for required tools
if ! command -v pandoc &> /dev/null; then
    echo -e "${RED}Error: pandoc not found${NC}"
    echo "Install with: sudo apt install pandoc texlive-latex-base"
    exit 1
fi

if ! command -v pdflatex &> /dev/null; then
    echo -e "${RED}Error: pdflatex not found${NC}"
    echo "Install with: sudo apt install texlive-latex-base texlive-fonts-recommended"
    exit 1
fi

# Counter
total_labs=0
successful=0
failed=0

# Function to convert single lab
convert_lab() {
    local input_file="$1"
    local output_file="$2"
    local lab_number="$3"
    local lab_title="$4"

    if [ ! -f "$input_file" ]; then
        echo -e "${RED}  ⚠️  File not found: $input_file${NC}"
        ((failed++))
        return 1
    fi

    echo -e "  📄 Converting Lab $lab_number: $lab_title"

    # Convert with Pandoc
    pandoc "$input_file" \
        -o "$output_file" \
        --pdf-engine=pdflatex \
        --variable geometry:margin=1in \
        --variable fontsize=11pt \
        --variable documentclass=article \
        --highlight-style=tango \
        --toc \
        --toc-depth=2 \
        --number-sections \
        --metadata title="Lab $lab_number: $lab_title" \
        --metadata author="TF-M Training Package" \
        --metadata date="2025-11-22" \
        2>/dev/null

    if [ $? -eq 0 ]; then
        echo -e "${GREEN}     ✅ Success${NC}"
        ((successful++))
    else
        echo -e "${RED}     ❌ Failed${NC}"
        ((failed++))
    fi

    ((total_labs++))
}

# Section 1: Foundation (Labs 02-04)
echo -e "${BLUE}[Section 1] Foundation Labs${NC}"
convert_lab "$BASE_DIR/section_01_foundation/labs/solutions/lab_02/README.md" \
            "$OUTPUT_DIR/Lab_02_TrustZone_Basics.pdf" \
            "02" "TrustZone Basics"

convert_lab "$BASE_DIR/section_01_foundation/labs/solutions/lab_03/README.md" \
            "$OUTPUT_DIR/Lab_03_PSA_Crypto_API.pdf" \
            "03" "PSA Crypto API"

convert_lab "$BASE_DIR/section_01_foundation/labs/solutions/lab_04/README.md" \
            "$OUTPUT_DIR/Lab_04_PSA_Secure_Storage.pdf" \
            "04" "PSA Secure Storage"

echo ""

# Section 2: Core Services (Labs 05-10)
echo -e "${BLUE}[Section 2] Core Security Labs${NC}"
convert_lab "$BASE_DIR/section_02_core_services/labs/solutions/lab_05/README.md" \
            "$OUTPUT_DIR/Lab_05_Initial_Attestation.pdf" \
            "05" "PSA Initial Attestation"

convert_lab "$BASE_DIR/section_02_core_services/labs/solutions/lab_06/README.md" \
            "$OUTPUT_DIR/Lab_06_MCUboot_Firmware_Update.pdf" \
            "06" "MCUboot Firmware Update"

convert_lab "$BASE_DIR/section_02_core_services/labs/solutions/lab_07/README.md" \
            "$OUTPUT_DIR/Lab_07_Secure_Boot_Measurements.pdf" \
            "07" "Secure Boot Measurements"

convert_lab "$BASE_DIR/section_02_core_services/labs/solutions/lab_08/README.md" \
            "$OUTPUT_DIR/Lab_08_Advanced_Protected_Storage.pdf" \
            "08" "Advanced Protected Storage"

convert_lab "$BASE_DIR/section_02_core_services/labs/solutions/lab_09/README.md" \
            "$OUTPUT_DIR/Lab_09_Runtime_Integrity_Monitoring.pdf" \
            "09" "Runtime Integrity Monitoring"

convert_lab "$BASE_DIR/section_02_core_services/labs/solutions/lab_10/README.md" \
            "$OUTPUT_DIR/Lab_10_Security_Integration_Exercise.pdf" \
            "10" "Security Integration Exercise (Capstone 1)"

echo ""

# Section 3: Advanced Topics (Labs 11-20)
echo -e "${BLUE}[Section 3] System Integration Labs${NC}"

lab_titles_3=(
    "Secure Peripheral Access"
    "Inter-Partition Communication (IPC)"
    "Secure Debug and Production"
    "Power Management and Secure Sleep"
    "Secure Timers and Watchdogs"
    "Secure DMA Operations"
    "Secure Firmware Update Integration"
    "Multi-Threaded Security (RTOS)"
    "Hardware Security Module Integration"
    "Complete System Hardening (Capstone 2)"
)

for i in {11..20}; do
    lab_num=$(printf "%02d" $i)
    idx=$((i - 11))
    lab_title="${lab_titles_3[$idx]}"

    convert_lab "$BASE_DIR/section_03_advanced_topics/labs/solutions/lab_$lab_num/README.md" \
                "$OUTPUT_DIR/Lab_${lab_num}_${lab_title// /_}.pdf" \
                "$lab_num" "$lab_title"
done

echo ""

# Section 4: Real-World Applications (Labs 21-30)
echo -e "${BLUE}[Section 4] Real-World Application Labs${NC}"

lab_titles_4=(
    "Secure Smart Home Gateway"
    "Industrial IoT Edge Device"
    "Medical Device Security"
    "Automotive ECU Security"
    "Payment Terminal Security (PCI PTS)"
    "Drone/UAV Flight Controller Security"
    "Energy Management System (IEC 62351)"
    "Agriculture IoT Sensor Network"
    "Retail Point-of-Sale System"
    "Complete Product Development Lifecycle (FINAL CAPSTONE)"
)

for i in {21..30}; do
    lab_num=$(printf "%02d" $i)
    idx=$((i - 21))
    lab_title="${lab_titles_4[$idx]}"

    convert_lab "$BASE_DIR/section_04_real_world_applications/labs/solutions/lab_$lab_num/README.md" \
                "$OUTPUT_DIR/Lab_${lab_num}_${lab_title// /_}.pdf" \
                "$lab_num" "$lab_title"
done

echo ""
echo -e "${BLUE}=== Conversion Summary ===${NC}"
echo -e "  Total labs: $total_labs"
echo -e "${GREEN}  Successful: $successful${NC}"
if [ $failed -gt 0 ]; then
    echo -e "${RED}  Failed: $failed${NC}"
fi
echo ""
echo -e "${GREEN}✅ PDF files saved to: $OUTPUT_DIR${NC}"
echo ""

# Create distribution archive
echo -e "${BLUE}Creating distribution archive...${NC}"
cd "$OUTPUT_DIR"
zip -q TFM_Training_Labs_PDFs.zip *.pdf
echo -e "${GREEN}✅ Archive created: TFM_Training_Labs_PDFs.zip${NC}"
echo ""

# Calculate total size
total_size=$(du -sh . | cut -f1)
echo -e "Total size: $total_size"
echo ""

exit 0

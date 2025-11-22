# Converting Labs to PDF Documents

## Overview

This guide explains how to convert the 30 training labs from Markdown format to professional PDF documents for distribution.

---

## Lab Files Location

All lab files are located in:
```
docs/training/
├── section_01_foundation/
│   └── labs/solutions/lab_02/README.md
│   └── labs/solutions/lab_03/README.md
│   └── labs/solutions/lab_04/README.md
├── section_02_core_services/
│   └── labs/solutions/lab_05/README.md
│   └── labs/solutions/lab_06/README.md
├── section_03_advanced_topics/
│   └── labs/solutions/lab_07-20/README.md
└── section_04_real_world_applications/
    └── labs/solutions/lab_21-30/README.md
```

**Total:** 30 lab README.md files

---

## Conversion Methods

### Method 1: Pandoc (Recommended)

#### Installation

```bash
# Ubuntu/Debian
sudo apt install pandoc texlive-latex-base texlive-fonts-recommended

# macOS
brew install pandoc basictex

# Windows
# Download from: https://pandoc.org/installing.html
```

#### Convert Single Lab

```bash
cd docs/training/section_01_foundation/labs/solutions/lab_02/

pandoc README.md \
  -o Lab_02_TrustZone_Basics.pdf \
  --pdf-engine=pdflatex \
  --variable geometry:margin=1in \
  --variable fontsize=11pt \
  --variable documentclass=article \
  --highlight-style=tango \
  --toc \
  --toc-depth=2
```

#### Batch Convert All Labs

See `convert_all_labs.sh` script below.

---

### Method 2: VS Code Extension

#### Installation

1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Install **Markdown PDF** extension:
   - Open Extensions (Ctrl+Shift+X)
   - Search "Markdown PDF"
   - Install "yzane.markdown-pdf"

#### Usage

1. Open any lab `README.md`
2. Right-click in editor
3. Select "Markdown PDF: Export (pdf)"
4. PDF saved in same directory

---

### Method 3: Online Converters

#### Using Dillinger (Web-based)

1. Go to [dillinger.io](https://dillinger.io/)
2. Copy/paste lab markdown content
3. Click "Export as" → PDF
4. Download generated PDF

#### Using StackEdit (Web-based)

1. Go to [stackedit.io](https://stackedit.io/)
2. Import lab markdown file
3. Click menu → Export → PDF

---

## Batch Conversion Script

### Pandoc Batch Script

Create `convert_all_labs.sh`:

```bash
#!/bin/bash

# Convert all lab README.md files to PDF

BASE_DIR="docs/training"
OUTPUT_DIR="docs/training/labs_pdf"

# Create output directory
mkdir -p "$OUTPUT_DIR"

echo "=== Converting Labs to PDF ==="

# Section 1: Foundation (Labs 02-04)
echo "Converting Section 1 labs..."
for lab in 02 03 04; do
    input="$BASE_DIR/section_01_foundation/labs/solutions/lab_$lab/README.md"
    output="$OUTPUT_DIR/Lab_${lab}_Section_1.pdf"

    if [ -f "$input" ]; then
        echo "  Converting Lab $lab..."
        pandoc "$input" \
          -o "$output" \
          --pdf-engine=pdflatex \
          --variable geometry:margin=1in \
          --variable fontsize=11pt \
          --variable documentclass=article \
          --highlight-style=tango \
          --toc \
          --toc-depth=2 \
          --metadata title="Lab $lab - TF-M Training"
    fi
done

# Section 2: Core Services (Labs 05-10)
echo "Converting Section 2 labs..."
for lab in $(seq -f "%02g" 5 10); do
    input="$BASE_DIR/section_02_core_services/labs/solutions/lab_$lab/README.md"
    output="$OUTPUT_DIR/Lab_${lab}_Section_2.pdf"

    if [ -f "$input" ]; then
        echo "  Converting Lab $lab..."
        pandoc "$input" \
          -o "$output" \
          --pdf-engine=pdflatex \
          --variable geometry:margin=1in \
          --variable fontsize=11pt \
          --highlight-style=tango \
          --toc \
          --metadata title="Lab $lab - TF-M Training"
    fi
done

# Section 3: Advanced Topics (Labs 11-20)
echo "Converting Section 3 labs..."
for lab in $(seq -f "%02g" 11 20); do
    input="$BASE_DIR/section_03_advanced_topics/labs/solutions/lab_$lab/README.md"
    output="$OUTPUT_DIR/Lab_${lab}_Section_3.pdf"

    if [ -f "$input" ]; then
        echo "  Converting Lab $lab..."
        pandoc "$input" \
          -o "$output" \
          --pdf-engine=pdflatex \
          --variable geometry:margin=1in \
          --variable fontsize=11pt \
          --highlight-style=tango \
          --toc \
          --metadata title="Lab $lab - TF-M Training"
    fi
done

# Section 4: Real-World Applications (Labs 21-30)
echo "Converting Section 4 labs..."
for lab in $(seq -f "%02g" 21 30); do
    input="$BASE_DIR/section_04_real_world_applications/labs/solutions/lab_$lab/README.md"
    output="$OUTPUT_DIR/Lab_${lab}_Section_4.pdf"

    if [ -f "$input" ]; then
        echo "  Converting Lab $lab..."
        pandoc "$input" \
          -o "$output" \
          --pdf-engine=pdflatex \
          --variable geometry:margin=1in \
          --variable fontsize=11pt \
          --highlight-style=tango \
          --toc \
          --metadata title="Lab $lab - TF-M Training"
    fi
done

echo "✅ Conversion complete! PDFs saved to: $OUTPUT_DIR"
echo "   Total labs converted: 30"
```

### Make Executable and Run

```bash
chmod +x convert_all_labs.sh
./convert_all_labs.sh
```

---

## PDF Customization Options

### Professional Layout

```bash
pandoc README.md -o output.pdf \
  --pdf-engine=xelatex \
  --variable geometry:margin=1.5in \
  --variable fontsize=12pt \
  --variable mainfont="Arial" \
  --variable monofont="Courier New" \
  --variable documentclass=report \
  --highlight-style=breezedark \
  --toc \
  --toc-depth=3 \
  --number-sections \
  --metadata title="TF-M Lab XX: Title" \
  --metadata author="Training Team" \
  --metadata date="2025-11-22"
```

### With Cover Page

Create `cover.md`:
```markdown
---
title: "TF-M Secure Firmware Development"
subtitle: "Lab 02: TrustZone Basics"
author: "Training Team"
date: "2025-11-22"
---
```

Convert with cover:
```bash
pandoc cover.md README.md -o Lab_02.pdf \
  --pdf-engine=pdflatex \
  --variable geometry:margin=1in
```

---

## Alternative: HTML to PDF

### Generate HTML First

```bash
pandoc README.md -o output.html \
  --standalone \
  --css=style.css \
  --toc \
  --highlight-style=pygments
```

### Print to PDF

1. Open HTML in Chrome/Firefox
2. Press Ctrl+P (Print)
3. Select "Save as PDF"
4. Adjust margins and layout
5. Save

---

## Python Script (Using WeasyPrint)

### Installation

```bash
pip install weasyprint markdown
```

### Script: `convert_lab.py`

```python
#!/usr/bin/env python3

import markdown
import os
from weasyprint import HTML

def convert_md_to_pdf(md_file, pdf_file):
    """Convert Markdown to PDF with styling"""

    # Read markdown
    with open(md_file, 'r', encoding='utf-8') as f:
        md_content = f.read()

    # Convert to HTML
    html = markdown.markdown(md_content,
                              extensions=['fenced_code', 'tables', 'toc'])

    # Add CSS styling
    styled_html = f"""
    <html>
    <head>
        <style>
            body {{
                font-family: Arial, sans-serif;
                font-size: 11pt;
                margin: 1in;
            }}
            code {{
                background-color: #f4f4f4;
                padding: 2px 5px;
                border-radius: 3px;
                font-family: 'Courier New', monospace;
            }}
            pre {{
                background-color: #1e1e1e;
                color: #d4d4d4;
                padding: 10px;
                border-radius: 5px;
                overflow-x: auto;
            }}
            h1 {{
                color: #00AA00;
                border-bottom: 2px solid #00AA00;
            }}
            h2 {{
                color: #0066CC;
            }}
            table {{
                border-collapse: collapse;
                width: 100%;
            }}
            th, td {{
                border: 1px solid #ddd;
                padding: 8px;
                text-align: left;
            }}
            th {{
                background-color: #00AA00;
                color: white;
            }}
        </style>
    </head>
    <body>
        {html}
    </body>
    </html>
    """

    # Convert to PDF
    HTML(string=styled_html).write_pdf(pdf_file)
    print(f"✅ Created: {pdf_file}")

if __name__ == '__main__':
    import sys

    if len(sys.argv) != 3:
        print("Usage: python convert_lab.py input.md output.pdf")
        sys.exit(1)

    convert_md_to_pdf(sys.argv[1], sys.argv[2])
```

### Usage

```bash
python convert_lab.py README.md Lab_02.pdf
```

---

## Automated GitHub Actions Workflow

### `.github/workflows/generate-pdfs.yml`

```yaml
name: Generate Lab PDFs

on:
  push:
    paths:
      - 'docs/training/**/labs/**/*.md'

jobs:
  generate-pdfs:
    runs-on: ubuntu-latest

    steps:
      - uses: actions/checkout@v2

      - name: Install Pandoc
        run: |
          sudo apt update
          sudo apt install -y pandoc texlive-latex-base texlive-fonts-recommended

      - name: Convert Labs to PDF
        run: |
          chmod +x convert_all_labs.sh
          ./convert_all_labs.sh

      - name: Upload PDFs
        uses: actions/upload-artifact@v2
        with:
          name: lab-pdfs
          path: docs/training/labs_pdf/*.pdf

      - name: Create Release
        uses: softprops/action-gh-release@v1
        if: startsWith(github.ref, 'refs/tags/')
        with:
          files: docs/training/labs_pdf/*.pdf
```

---

## Quality Checklist

After conversion, verify:

- [ ] All code blocks properly formatted
- [ ] Syntax highlighting applied
- [ ] Tables rendered correctly
- [ ] Images included (if any)
- [ ] Table of contents generated
- [ ] Page breaks appropriate
- [ ] Headers/footers visible
- [ ] No text cutoff
- [ ] Hyperlinks working
- [ ] File size reasonable (<5 MB per lab)

---

## Distribution Package

### Create ZIP Archive

```bash
# After converting all PDFs
cd docs/training/labs_pdf/

# Create distribution package
zip -r TFM_Training_Labs_PDFs.zip *.pdf

# Or create tar.gz
tar czf TFM_Training_Labs_PDFs.tar.gz *.pdf

echo "Distribution package created!"
```

### File Naming Convention

```
Lab_02_TrustZone_Basics.pdf
Lab_03_PSA_Crypto_API.pdf
Lab_04_Secure_Storage.pdf
...
Lab_30_Complete_Product_Lifecycle.pdf
```

---

## Troubleshooting

### Issue: "pdflatex not found"

**Solution:** Install TeX Live
```bash
sudo apt install texlive-latex-base texlive-fonts-recommended
```

### Issue: "Syntax highlighting not working"

**Solution:** Specify highlight style
```bash
pandoc ... --highlight-style=tango
```

### Issue: "Images not appearing"

**Solution:** Use absolute paths or copy images
```bash
pandoc ... --resource-path=.:images/
```

### Issue: "PDF too large"

**Solution:** Optimize images first
```bash
# Compress images before conversion
for img in *.png; do
    pngquant --quality=65-80 "$img" --output "optimized/$img"
done
```

---

## Summary

**Recommended Workflow:**

1. Install Pandoc + LaTeX (one-time setup)
2. Run `convert_all_labs.sh` script
3. Verify PDF quality
4. Create distribution ZIP
5. Share with students

**Estimated Time:** 15-20 minutes for all 30 labs

---

**Last Updated:** 2025-11-22
**Version:** 1.0

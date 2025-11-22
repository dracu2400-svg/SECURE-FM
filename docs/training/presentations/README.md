# TF-M Training Package - Presentations

## Overview

This directory contains **6 comprehensive presentation slide decks** for the TF-M Secure Firmware Development training package, totaling **300 slides** across all sections.

All presentations are created in **Marp format** (Markdown Presentation Ecosystem), which can be easily converted to PowerPoint (PPTX) or PDF.

---

## Presentation Files

| File | Title | Slides | Duration |
|------|-------|--------|----------|
| `section_01_trustzone_foundations.md` | TrustZone-M Foundations | 50 | 2.5 hours |
| `section_02_psa_core_services.md` | PSA Core Services | 55 | 3 hours |
| `section_03_advanced_topics.md` | Advanced TF-M Topics | 50 | 2 hours |
| `section_04_integration_deployment.md` | Integration & Deployment | 45 | 2 hours |
| `section_05_performance_optimization.md` | Performance & Optimization | 40 | 1.5 hours |
| `section_06_security_attacks.md` | Security & Attack Mitigations | 60 | 3 hours |
| **TOTAL** | | **300** | **~14 hours** |

---

## Converting to PowerPoint/PDF

### Option 1: Using Marp CLI (Recommended)

#### Installation

```bash
# Install Node.js (if not installed)
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt install -y nodejs

# Install Marp CLI
npm install -g @marp-team/marp-cli
```

#### Convert to PowerPoint (PPTX)

```bash
# Convert single presentation
marp section_01_trustzone_foundations.md --pptx -o section_01.pptx

# Convert all presentations
marp section_*.md --pptx
```

#### Convert to PDF

```bash
# Convert single presentation
marp section_01_trustzone_foundations.md --pdf -o section_01.pdf

# Convert all presentations with Chrome
marp section_*.md --pdf --allow-local-files
```

#### Convert to HTML (for web viewing)

```bash
# Interactive HTML presentation
marp section_01_trustzone_foundations.md --html -o section_01.html
```

---

### Option 2: Using VS Code Extension

#### Installation

1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Install **Marp for VS Code** extension:
   - Open VS Code
   - Go to Extensions (Ctrl+Shift+X)
   - Search for "Marp for VS Code"
   - Click Install

#### Usage

1. Open any `.md` file in VS Code
2. Click the **Marp** icon in the top-right corner
3. Choose export format:
   - Export to PowerPoint (PPTX)
   - Export to PDF
   - Export to HTML

---

### Option 3: Using Pandoc (Alternative)

```bash
# Install Pandoc
sudo apt install pandoc

# Convert to PowerPoint
pandoc section_01_trustzone_foundations.md -o section_01.pptx

# Convert to PDF (requires LaTeX)
sudo apt install texlive-latex-base
pandoc section_01_trustzone_foundations.md -t beamer -o section_01.pdf
```

---

## Customization

### Editing Presentations

The presentations are standard Markdown files with special Marp syntax. Edit them with any text editor:

```markdown
---
marp: true
theme: default
paginate: true
---

# Slide Title

Content goes here

---

# Next Slide

More content
```

### Customizing Theme

Edit the `style` section in the YAML front matter:

```yaml
style: |
  section {
    background-color: #ffffff;
  }
  h1 {
    color: #00AA00;  /* Change heading color */
  }
```

### Adding Images

Replace placeholder images with actual images:

```markdown
![width:800px](path/to/your/image.png)

# Or as background
![bg right:40%](path/to/image.png)
```

---

## Batch Conversion Script

Create a script to convert all presentations at once:

```bash
#!/bin/bash
# convert_all.sh

echo "Converting all presentations to PowerPoint..."

for file in section_*.md; do
    basename="${file%.md}"
    echo "Converting $file → $basename.pptx"
    marp "$file" --pptx -o "$basename.pptx"
done

echo "Converting all presentations to PDF..."

for file in section_*.md; do
    basename="${file%.md}"
    echo "Converting $file → $basename.pdf"
    marp "$file" --pdf --allow-local-files -o "$basename.pdf"
done

echo "✅ Conversion complete!"
```

Run with:
```bash
chmod +x convert_all.sh
./convert_all.sh
```

---

## Content Overview

### Section 1: TrustZone-M Foundations
- Why security matters
- TrustZone-M architecture
- SAU, IDAU, NSC regions
- CMSE intrinsics
- TF-M overview
- Development tools

### Section 2: PSA Core Services
- PSA Crypto API (AES, ECDSA, SHA)
- Key management
- Secure Storage (ITS, PS)
- Initial Attestation
- Cloud integration

### Section 3: Advanced TF-M Topics
- Custom Secure Services
- Partition design
- IPC mechanism
- Isolation levels
- MCUboot firmware update

### Section 4: Integration & Deployment
- RTOS integration (FreeRTOS, Zephyr)
- Secure boot process
- OTA firmware updates
- Debug authentication
- Production deployment (RDP levels)

### Section 5: Performance & Optimization
- Memory optimization
- Power management (Sleep, Stop, Standby)
- Hardware acceleration
- Code size reduction
- Benchmarking

### Section 6: Security & Attack Mitigations
- Threat modeling (STRIDE, TARA)
- Software attacks (buffer overflow, TOCTOU)
- Hardware attacks (JTAG, flash readout)
- Side-channel attacks (power, timing, EM)
- Fault injection countermeasures
- Secure coding practices

---

## Presentation Delivery Tips

### For Instructors

1. **Timing:**
   - Allow 2-3 minutes per slide average
   - Include breaks every 90 minutes
   - Reserve time for Q&A

2. **Labs:**
   - Reference labs at relevant points
   - Show live demonstrations on NUCLEO board
   - Encourage hands-on practice

3. **Engagement:**
   - Ask questions to check understanding
   - Use code examples extensively
   - Show real-world security incidents

4. **Materials:**
   - Provide lab access beforehand
   - Share code examples repository
   - Distribute PDF slides after session

---

## License

**Copyright © 2025**

This training package is provided for educational purposes.

---

## Support

For questions or issues:
- **Email:** training@example.com
- **Website:** secure-fm-training.com
- **Documentation:** See `docs/training/` directory

---

## Version History

- **v1.0** (2025-11-22): Initial release
  - 300 slides across 6 sections
  - Marp format for easy conversion
  - Complete TF-M training coverage

---

**🎓 Ready to teach secure embedded systems development!**

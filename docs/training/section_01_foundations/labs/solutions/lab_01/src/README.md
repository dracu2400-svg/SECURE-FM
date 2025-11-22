# Lab 01 Source Files

This directory contains helper scripts and configuration files for Lab 01.

## Files

### Build Scripts

**`build_tfm_nucleo_u545.sh`** - Automated TF-M Build Script
- Configures and builds TF-M for NUCLEO-U545RE-Q
- Shows build progress and memory usage
- Usage:
  ```bash
  ./build_tfm_nucleo_u545.sh [profile_small|profile_medium|profile_large]
  # Default: profile_medium
  ```
- Output: Binary files in `~/tfm-nucleo-u545/trusted-firmware-m/build_nucleo_u545/bin/`

**`flash_tfm_nucleo_u545.sh`** - Automated Flash Script
- Flashes all three images (BL2, Secure, Non-Secure) to the board
- Verifies ST-Link connection
- Optionally erases flash first (default)
- Usage:
  ```bash
  ./flash_tfm_nucleo_u545.sh [yes|no]
  # yes = erase first (default), no = skip erase
  ```

**`connect_serial.sh`** - Serial Console Connection
- Automatically detects serial port (/dev/ttyACM0)
- Uses screen, minicom, or picocom (whichever is available)
- Sets correct baud rate (115200 8N1)
- Usage:
  ```bash
  ./connect_serial.sh
  ```

### Configuration Files

**`config_nucleo_u545.cmake`** - CMake Configuration Template
- Complete CMake configuration for NUCLEO-U545RE-Q
- Documented settings for all TF-M options
- Can be used with: `cmake <TFM_ROOT> -C config_nucleo_u545.cmake`
- Alternative to command-line CMake options

## Quick Start

```bash
# 1. Build TF-M
./build_tfm_nucleo_u545.sh

# 2. Flash to board
./flash_tfm_nucleo_u545.sh

# 3. Connect to serial console
./connect_serial.sh

# 4. Press RESET button on board and observe boot messages
```

## Requirements

- ARM GNU Toolchain 13.2+
- CMake 3.21+
- stlink-tools (for flashing)
- screen/minicom/picocom (for serial console)
- NUCLEO-U545RE-Q connected via USB

## Troubleshooting

**Build fails:**
- Check that ARM toolchain is in PATH: `arm-none-eabi-gcc --version`
- Ensure all Python dependencies installed: `pip3 install -r tools/requirements.txt`

**Flash fails:**
- Check ST-Link connection: `st-info --probe`
- Try with sudo: `sudo ./flash_tfm_nucleo_u545.sh`
- Check udev rules: `/etc/udev/rules.d/99-stlink.rules`

**No serial output:**
- Check port: `ls -l /dev/ttyACM*`
- Try different port: Edit connect_serial.sh
- Check permissions: Add yourself to dialout group

## Notes

- All scripts assume TF-M is cloned to `~/tfm-nucleo-u545/trusted-firmware-m`
- Scripts use green/yellow/red colored output for readability
- Build script shows comprehensive memory usage statistics
- Flash script includes detailed memory layout documentation

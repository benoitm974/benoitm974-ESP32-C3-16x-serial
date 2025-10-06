# LittleFS Build and Upload Guide

## Overview
The ESP32 Serial Multiplexer now uses LittleFS with gzip compression for web assets. This provides better performance and maintainability.

## File Structure
```
project/
├── data-src/           # Source web files (editable)
│   ├── index.html
│   ├── style.css
│   ├── script.js
│   └── terminal-icon.svg
├── data/              # Generated compressed files (auto-created)
│   ├── index.html.gz
│   ├── style.css.gz
│   ├── script.js.gz
│   └── terminal-icon.svg.gz
└── scripts/
    └── compress_data.py
```

## Build Commands

### 1. Build the main firmware
```bash
pio run
```

### 2. Build the LittleFS filesystem (with compression)
```bash
pio run --target buildfs
```

### 3. Upload firmware (automatically uploads filesystem too!)
```bash
pio run --target upload
```
**Note**: The filesystem is now automatically uploaded after firmware upload!

### 4. Upload only the LittleFS filesystem
```bash
pio run --target uploadfs
```

### 5. Manual filesystem upload (if auto-upload fails)
```bash
pio run --target uploadfs
```

## VSCode PlatformIO Extension Commands

If using VSCode with PlatformIO extension:

1. **Upload Project**: Click "Upload" button (uploads firmware + filesystem automatically)
2. **Build Filesystem Image**: Click "Build Filesystem Image" in PlatformIO toolbar
3. **Upload Filesystem Image**: Click "Upload Filesystem Image" in PlatformIO toolbar (manual)

## How It Works

1. **Pre-build**: The `compress_data.py` script runs automatically before filesystem build
2. **Compression**: Files from `data-src/` are gzipped and saved to `data/`
3. **Firmware Upload**: PlatformIO uploads the main firmware to ESP32
4. **Auto Filesystem Upload**: The `auto_uploadfs.py` script automatically uploads the `data/` folder
5. **Serving**: ESP32 serves gzipped files with proper `Content-Encoding: gzip` headers

## File Compression Results

Typical compression ratios:
- **HTML**: ~60-70% reduction
- **CSS**: ~70-80% reduction  
- **JavaScript**: ~60-70% reduction
- **SVG**: ~50-60% reduction

## Troubleshooting

### "File not found" error
- The filesystem should upload automatically after firmware upload
- If auto-upload fails, manually run: `pio run --target uploadfs`
- Check that LittleFS initialization succeeds (see Serial Monitor)

### Auto-upload not working
- Check the console output for error messages during upload
- Manually upload filesystem: `pio run --target uploadfs`
- Ensure `scripts/auto_uploadfs.py` exists and is executable

### Files not compressing
- Verify files are in `data-src/` directory
- Check file extensions are supported (.html, .css, .js, .svg, .json, .txt)

### Build errors
- Ensure Python is available for the compression script
- Check that `data-src/` directory exists with source files

## Memory Usage

- **Before**: ~8KB HTML/CSS/JS in RAM
- **After**: ~3KB compressed files in flash storage
- **Savings**: ~5KB RAM freed + faster loading
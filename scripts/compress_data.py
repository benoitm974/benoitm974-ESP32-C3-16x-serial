#!/usr/bin/env python3
"""
PlatformIO pre-build script to compress data files with gzip
This script compresses files from data-src/ to data/ directory
before they are uploaded to the ESP32's LittleFS filesystem.
"""

import os
import gzip
import shutil
from pathlib import Path

Import("env")

def compress_file(source_path, target_path):
    """Compress a single file with gzip"""
    try:
        # Ensure target directory exists
        target_path.parent.mkdir(parents=True, exist_ok=True)
        
        with open(source_path, 'rb') as f_in:
            with gzip.open(target_path, 'wb') as f_out:
                shutil.copyfileobj(f_in, f_out)
        
        # Get compression ratio
        original_size = os.path.getsize(source_path)
        compressed_size = os.path.getsize(target_path)
        ratio = (1 - compressed_size / original_size) * 100
        
        print(f"Compressed {source_path.name}: {original_size} -> {compressed_size} bytes ({ratio:.1f}% reduction)")
        return True
    except Exception as e:
        print(f"Error compressing {source_path}: {e}")
        return False

def copy_file(source_path, target_path):
    """Copy a file without compression"""
    try:
        # Ensure target directory exists
        target_path.parent.mkdir(parents=True, exist_ok=True)
        
        shutil.copy2(source_path, target_path)
        print(f"Copied {source_path.name}: {os.path.getsize(source_path)} bytes")
        return True
    except Exception as e:
        print(f"Error copying {source_path}: {e}")
        return False

def prepare_data_files():
    """Prepare data files from data-src to data directory"""
    project_dir = Path(env.subst("$PROJECT_DIR"))
    data_src_dir = project_dir / "data-src"
    data_dir = project_dir / "data"
    
    if not data_src_dir.exists():
        print("No data-src directory found, skipping file preparation")
        return
    
    # Clean existing data directory
    if data_dir.exists():
        shutil.rmtree(data_dir)
    data_dir.mkdir(exist_ok=True)
    
    print("Preparing data files for LittleFS...")
    
    # Files to compress (web assets)
    compress_extensions = {'.html', '.css', '.js', '.svg', '.json', '.txt'}
    # Files to copy without compression
    copy_extensions = {'.ico', '.png', '.jpg', '.jpeg', '.gif', '.woff', '.woff2'}
    
    processed_count = 0
    total_original = 0
    total_final = 0
    
    for file_path in data_src_dir.rglob('*'):
        if file_path.is_file():
            # Calculate relative path to preserve directory structure
            rel_path = file_path.relative_to(data_src_dir)
            
            if file_path.suffix.lower() in compress_extensions:
                # Compress and add .gz extension
                target_path = data_dir / (str(rel_path) + '.gz')
                if compress_file(file_path, target_path):
                    processed_count += 1
                    total_original += os.path.getsize(file_path)
                    total_final += os.path.getsize(target_path)
            
            elif file_path.suffix.lower() in copy_extensions:
                # Copy without compression
                target_path = data_dir / rel_path
                if copy_file(file_path, target_path):
                    processed_count += 1
                    file_size = os.path.getsize(file_path)
                    total_original += file_size
                    total_final += file_size
            
            else:
                print(f"Skipping unknown file type: {file_path.name}")
    
    if processed_count > 0:
        if total_original > 0:
            overall_ratio = (1 - total_final / total_original) * 100
            print(f"File preparation complete: {processed_count} files, {total_original} -> {total_final} bytes ({overall_ratio:.1f}% total reduction)")
        else:
            print(f"File preparation complete: {processed_count} files processed")
    else:
        print("No files were processed")

# Run file preparation before building filesystem
prepare_data_files()
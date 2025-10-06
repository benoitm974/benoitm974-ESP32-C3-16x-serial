#!/usr/bin/env python3
"""
PlatformIO post-upload script to automatically upload filesystem
This script runs after firmware upload to automatically upload the LittleFS filesystem
"""

import subprocess
import sys
from pathlib import Path

Import("env")

def auto_upload_filesystem():
    """Automatically upload filesystem after firmware upload"""
    
    # Only run this for the 'upload' target, not for other targets like 'uploadfs'
    upload_targets = env.get("UPLOAD_FLAGS", [])
    current_targets = [str(t) for t in COMMAND_LINE_TARGETS]
    
    # Check if we're doing a regular upload (not uploadfs)
    if "upload" in current_targets and "uploadfs" not in current_targets:
        print("\n" + "="*60)
        print("🚀 Firmware upload complete! Now uploading filesystem...")
        print("="*60)
        
        try:
            # Get the project directory
            project_dir = Path(env.subst("$PROJECT_DIR"))
            
            # Check if data directory exists and has files
            data_dir = project_dir / "data"
            if not data_dir.exists() or not any(data_dir.iterdir()):
                print("⚠️  No data directory or files found. Skipping filesystem upload.")
                return
            
            # Run the uploadfs command
            result = subprocess.run([
                "pio", "run", "--target", "uploadfs"
            ], cwd=project_dir, capture_output=True, text=True)
            
            if result.returncode == 0:
                print("✅ Filesystem upload successful!")
                print("🌐 Web interface is now ready at the ESP32's IP address")
            else:
                print("❌ Filesystem upload failed:")
                print(result.stderr)
                print("\n💡 You can manually upload with: pio run --target uploadfs")
                
        except Exception as e:
            print(f"❌ Error during automatic filesystem upload: {e}")
            print("💡 You can manually upload with: pio run --target uploadfs")
        
        print("="*60)
    
    # For uploadfs target, don't run this script to avoid recursion
    elif "uploadfs" in current_targets:
        pass  # Do nothing for uploadfs target

# Run the auto upload
auto_upload_filesystem()
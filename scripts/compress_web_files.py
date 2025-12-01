#!/usr/bin/env python3
"""
Pre-build script to compress web files for LittleFS
Compresses HTML, CSS, and JS files using gzip
"""

import gzip
import os
import shutil
from pathlib import Path

Import("env")

def compress_web_files(source, target, env):
    """Compress web files before building filesystem"""

    # Get project directory
    project_dir = env.get("PROJECT_DIR", ".")
    data_dir = os.path.join(project_dir, "data")
    web_dir = os.path.join(data_dir, "web")

    if not os.path.exists(web_dir):
        print(f"[Compress] Web directory not found: {web_dir}")
        return

    print("[Compress] Compressing web files...")

    # File extensions to compress
    compress_extensions = ['.html', '.css', '.js', '.json', '.svg']

    # Walk through web directory
    for root, dirs, files in os.walk(web_dir):
        for filename in files:
            filepath = os.path.join(root, filename)
            ext = os.path.splitext(filename)[1].lower()

            # Skip already compressed files
            if filename.endswith('.gz'):
                continue

            if ext in compress_extensions:
                gz_filepath = filepath + '.gz'

                # Check if source is newer than compressed
                if os.path.exists(gz_filepath):
                    src_mtime = os.path.getmtime(filepath)
                    gz_mtime = os.path.getmtime(gz_filepath)
                    if gz_mtime >= src_mtime:
                        continue  # Compressed file is up to date

                # Compress the file
                try:
                    with open(filepath, 'rb') as f_in:
                        with gzip.open(gz_filepath, 'wb', compresslevel=9) as f_out:
                            shutil.copyfileobj(f_in, f_out)

                    original_size = os.path.getsize(filepath)
                    compressed_size = os.path.getsize(gz_filepath)
                    ratio = (1 - compressed_size / original_size) * 100

                    print(f"[Compress] {filename}: {original_size} -> {compressed_size} bytes ({ratio:.1f}% reduction)")

                except Exception as e:
                    print(f"[Compress] Error compressing {filename}: {e}")

    print("[Compress] Done!")


# Register the pre-build action
env.AddPreAction("$BUILD_DIR/littlefs.bin", compress_web_files)
env.AddPreAction("buildfs", compress_web_files)

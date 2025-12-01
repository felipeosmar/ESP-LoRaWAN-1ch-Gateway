#!/usr/bin/env python3
"""
Pre-build script to compress web files for LittleFS
Compresses HTML, CSS, and JS files using gzip

Strategy:
- Keep original files in data/web/ for development/editing
- Create .gz versions for production
- Use data_src/ as staging area with only .gz files for filesystem build
- This saves significant flash space on 2MB devices
"""

import gzip
import os
import shutil
import sys
from pathlib import Path

Import("env")

# Configuration
COMPRESS_EXTENSIONS = ['.html', '.css', '.js', '.svg']
KEEP_ORIGINALS_IN_FS = False  # Set to True to include both original and .gz files


def compress_web_files(project_dir, for_filesystem=False):
    """
    Compress web files before building filesystem

    Args:
        project_dir: Project root directory
        for_filesystem: If True, prepare files for filesystem build (remove originals)
    """
    data_dir = os.path.join(project_dir, "data")
    web_dir = os.path.join(data_dir, "web")

    if not os.path.exists(web_dir):
        print(f"[Compress] Web directory not found: {web_dir}")
        return

    print("[Compress] Compressing web files...")

    compressed_count = 0
    skipped_count = 0
    removed_count = 0

    # Walk through web directory
    for root, dirs, files in os.walk(web_dir):
        for filename in files:
            filepath = os.path.join(root, filename)
            ext = os.path.splitext(filename)[1].lower()

            # Skip already compressed files
            if filename.endswith('.gz'):
                continue

            # Skip config.json (it changes at runtime)
            if filename == 'config.json':
                continue

            if ext in COMPRESS_EXTENSIONS:
                gz_filepath = filepath + '.gz'

                # Check if source is newer than compressed
                needs_compression = True
                if os.path.exists(gz_filepath):
                    src_mtime = os.path.getmtime(filepath)
                    gz_mtime = os.path.getmtime(gz_filepath)
                    if gz_mtime >= src_mtime:
                        needs_compression = False
                        skipped_count += 1

                # Compress the file if needed
                if needs_compression:
                    try:
                        with open(filepath, 'rb') as f_in:
                            with gzip.open(gz_filepath, 'wb', compresslevel=9) as f_out:
                                shutil.copyfileobj(f_in, f_out)

                        original_size = os.path.getsize(filepath)
                        compressed_size = os.path.getsize(gz_filepath)
                        ratio = (1 - compressed_size / original_size) * 100
                        compressed_count += 1

                        print(f"[Compress] {filename}: {original_size} -> {compressed_size} bytes ({ratio:.1f}% reduction)")

                    except Exception as e:
                        print(f"[Compress] Error compressing {filename}: {e}")
                        continue

                # For filesystem build, delete original temporarily
                # so mklittlefs only picks up the .gz files
                # We keep a backup outside data/ folder
                if for_filesystem and not KEEP_ORIGINALS_IN_FS:
                    # Store backup path for restoration
                    backup_dir = os.path.join(project_dir, ".web_backup")
                    os.makedirs(backup_dir, exist_ok=True)

                    rel_path = os.path.relpath(filepath, data_dir)
                    backup_path = os.path.join(backup_dir, rel_path)
                    os.makedirs(os.path.dirname(backup_path), exist_ok=True)

                    if os.path.exists(filepath):
                        shutil.move(filepath, backup_path)
                        removed_count += 1

    if compressed_count > 0 or skipped_count > 0:
        print(f"[Compress] Done! {compressed_count} compressed, {skipped_count} up to date.")

    if removed_count > 0:
        print(f"[Compress] {removed_count} originals temporarily renamed for FS build.")


def restore_original_files(project_dir):
    """Restore original files after filesystem build"""
    data_dir = os.path.join(project_dir, "data")
    backup_dir = os.path.join(project_dir, ".web_backup")

    if not os.path.exists(backup_dir):
        return

    restored_count = 0

    for root, dirs, files in os.walk(backup_dir):
        for filename in files:
            backup_path = os.path.join(root, filename)
            rel_path = os.path.relpath(backup_path, backup_dir)
            original_path = os.path.join(data_dir, rel_path)

            # Ensure directory exists
            os.makedirs(os.path.dirname(original_path), exist_ok=True)

            # Restore the file
            shutil.move(backup_path, original_path)
            restored_count += 1

    # Clean up backup directory
    try:
        shutil.rmtree(backup_dir)
    except:
        pass

    if restored_count > 0:
        print(f"[Compress] Restored {restored_count} original files.")


def pre_buildfs(source, target, env):
    """Called before building filesystem - compress and hide originals"""
    project_dir = env.get("PROJECT_DIR", ".")
    compress_web_files(project_dir, for_filesystem=True)


def post_buildfs(source, target, env):
    """Called after building filesystem - restore originals"""
    project_dir = env.get("PROJECT_DIR", ".")
    restore_original_files(project_dir)


# Get build targets from command line
build_targets = COMMAND_LINE_TARGETS if 'COMMAND_LINE_TARGETS' in dir() else []

# Get project directory
project_dir = env.get("PROJECT_DIR", ".")

# Check if any filesystem-related target is being built
fs_targets = ['buildfs', 'uploadfs', 'erase_upload_fs']
building_fs = any(t in build_targets for t in fs_targets)

if building_fs:
    # For filesystem builds, compress and temporarily hide originals
    compress_web_files(project_dir, for_filesystem=True)

    # Register post-build action to restore originals
    env.AddPostAction("buildfs", post_buildfs)
    env.AddPostAction("uploadfs", post_buildfs)
else:
    # For regular builds, just ensure .gz files exist (for development)
    compress_web_files(project_dir, for_filesystem=False)

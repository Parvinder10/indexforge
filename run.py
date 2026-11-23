#!/usr/bin/env python3
"""
IndexForge — 1-Click Local Launcher
Builds C++ engine (if needed), starts FastAPI backend, and opens the live search UI.
"""

import os
import sys
import time
import shutil
import subprocess
import threading
import webbrowser

def find_engine_library():
    """Check for compiled C++ shared library across standard build locations."""
    repo_root = os.path.dirname(os.path.abspath(__file__))
    candidates = [
        # Windows
        os.path.join(repo_root, "build", "indexforge.dll"),
        os.path.join(repo_root, "build", "Release", "indexforge.dll"),
        os.path.join(repo_root, "build", "Debug", "indexforge.dll"),
        os.path.join(repo_root, "indexforge.dll"),
        # Linux
        os.path.join(repo_root, "build", "libindexforge.so"),
        os.path.join(repo_root, "libindexforge.so"),
        # macOS
        os.path.join(repo_root, "build", "libindexforge.dylib"),
        os.path.join(repo_root, "libindexforge.dylib"),
    ]
    for path in candidates:
        if os.path.isfile(path):
            return path
    return None

def build_engine():
    """Attempt automated CMake build if library is missing."""
    repo_root = os.path.dirname(os.path.abspath(__file__))
    cmake_bin = shutil.which("cmake")
    if not cmake_bin:
        print("[!] CMake not found in PATH. Please install CMake and a C++20 compiler.")
        return False
    
    print("[*] Compiling C++20 IndexForge engine via CMake...")
    build_dir = os.path.join(repo_root, "build")
    
    # Configure
    cmd_config = [cmake_bin, "-B", build_dir, "-DCMAKE_BUILD_TYPE=Release"]
    if shutil.which("ninja"):
        cmd_config.extend(["-G", "Ninja"])
    
    res = subprocess.run(cmd_config, cwd=repo_root)
    if res.returncode != 0:
        print("[!] CMake configuration failed.")
        return False
        
    # Build
    cmd_build = [cmake_bin, "--build", build_dir, "--config", "Release"]
    res = subprocess.run(cmd_build, cwd=repo_root)
    if res.returncode != 0:
        print("[!] CMake compilation failed.")
        return False
        
    return find_engine_library() is not None

def open_browser():
    """Wait for server startup and launch default browser."""
    time.sleep(1.2)
    url = "http://127.0.0.1:8000"
    print(f"\n[+] Opening live interface in browser: {url}")
    webbrowser.open(url)

def main():
    repo_root = os.path.dirname(os.path.abspath(__file__))
    os.chdir(repo_root)

    print("=" * 65)
    print("      IndexForge — High-Performance Full-Text Search Engine")
    print("=" * 65)

    # 1. Verify / Build C++ Engine
    lib_path = find_engine_library()
    if not lib_path:
        print("[*] Engine library not found. Triggering automated build...")
        if not build_engine():
            print("\n[!] Could not locate or build indexforge shared library.")
            print("    Please run: cmake -B build && cmake --build build")
            sys.exit(1)
        lib_path = find_engine_library()

    print(f"[✓] C++ Engine Core located at: {lib_path}")

    # 2. Check Python Dependencies
    try:
        import fastapi
        import uvicorn
        import pydantic
    except ImportError as e:
        print(f"[!] Missing Python dependency: {e.name}")
        print("[*] Installing required dependencies...")
        subprocess.run([sys.executable, "-m", "pip", "install", "fastapi", "uvicorn", "pydantic"])

    # 3. Open browser in separate daemon thread
    threading.Thread(target=open_browser, daemon=True).start()

    # 4. Start Uvicorn Server
    print("[*] Starting FastAPI Search Service on http://127.0.0.1:8000")
    print("[*] Press Ctrl+C to terminate.")
    print("=" * 65)

    import uvicorn
    uvicorn.run("api.main:app", host="127.0.0.1", port=8000, log_level="info")

if __name__ == "__main__":
    main()

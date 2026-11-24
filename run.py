#!/usr/bin/env python3
"""
IndexForge — Unified Full-Stack Local Launcher
Coordinates C++ build verification, FastAPI backend (http://127.0.0.1:8000),
and React + Vite development server (http://localhost:5173).
"""

import os
import sys
import time
import signal
import socket
import shutil
import urllib.request
import subprocess
import threading
import webbrowser
import argparse

REPO_ROOT = os.path.dirname(os.path.abspath(__file__))
FRONTEND_DIR = os.path.join(REPO_ROOT, "frontend")

def is_port_in_use(port, host="127.0.0.1"):
    """Check if a network port is already bound."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.settimeout(0.5)
        return s.connect_ex((host, port)) == 0

def find_available_port(start_port, host="127.0.0.1"):
    """Find next available free TCP port."""
    port = start_port
    while is_port_in_use(port, host):
        port += 1
    return port

def find_engine_library():
    """Locate the compiled C++20 engine library."""
    candidates = [
        # Windows
        os.path.join(REPO_ROOT, "build", "indexforge.dll"),
        os.path.join(REPO_ROOT, "build", "Release", "indexforge.dll"),
        os.path.join(REPO_ROOT, "build", "Debug", "indexforge.dll"),
        os.path.join(REPO_ROOT, "indexforge.dll"),
        # Linux
        os.path.join(REPO_ROOT, "build", "libindexforge.so"),
        os.path.join(REPO_ROOT, "libindexforge.so"),
        # macOS
        os.path.join(REPO_ROOT, "build", "libindexforge.dylib"),
        os.path.join(REPO_ROOT, "libindexforge.dylib"),
    ]
    for path in candidates:
        if os.path.isfile(path):
            return path
    return None

def build_engine():
    """Compile C++20 engine via CMake if missing."""
    cmake_bin = shutil.which("cmake")
    if not cmake_bin:
        print("[!] CMake not found in system PATH. Please install CMake and a C++20 compiler.")
        return False
    
    print("[*] Compiling C++20 IndexForge engine via CMake...")
    build_dir = os.path.join(REPO_ROOT, "build")
    
    cmd_config = [cmake_bin, "-B", build_dir, "-DCMAKE_BUILD_TYPE=Release"]
    if shutil.which("ninja"):
        cmd_config.extend(["-G", "Ninja"])
    
    res = subprocess.run(cmd_config, cwd=REPO_ROOT)
    if res.returncode != 0:
        return False
        
    cmd_build = [cmake_bin, "--build", build_dir, "--config", "Release"]
    res = subprocess.run(cmd_build, cwd=REPO_ROOT)
    return res.returncode == 0

def check_python_deps():
    """Verify required Python packages."""
    required = ["fastapi", "uvicorn", "pydantic"]
    missing = []
    for pkg in required:
        try:
            __import__(pkg)
        except ImportError:
            missing.append(pkg)
    if missing:
        print(f"[*] Installing missing Python packages: {missing}...")
        subprocess.run([sys.executable, "-m", "pip", "install", *missing], check=True)

def check_node_deps():
    """Verify frontend node_modules."""
    node_modules = os.path.join(FRONTEND_DIR, "node_modules")
    if not os.path.isdir(node_modules):
        npm_bin = shutil.which("npm.cmd" if os.name == "nt" else "npm")
        if not npm_bin:
            print("[!] npm not found. Please install Node.js (v18+) to run the Vite dev server.")
            return False
        print("[*] Installing frontend npm packages in frontend/...")
        res = subprocess.run([npm_bin, "install"], cwd=FRONTEND_DIR)
        return res.returncode == 0
    return True

def wait_for_url(url, timeout=15):
    """Wait until an HTTP endpoint responds with 200 OK."""
    start = time.time()
    while time.time() - start < timeout:
        try:
            with urllib.request.urlopen(url, timeout=1) as resp:
                if resp.status == 200:
                    return True
        except Exception:
            time.sleep(0.3)
    return False

def main():
    parser = argparse.ArgumentParser(description="IndexForge Full-Stack Local Launcher")
    parser.add_argument("--backend-only", action="store_true", help="Launch only FastAPI backend")
    parser.add_argument("--frontend-only", action="store_true", help="Launch only React + Vite dev server")
    parser.add_argument("--build-only", action="store_true", help="Build C++ library and frontend dist, then exit")
    parser.add_argument("--no-browser", action="store_true", help="Do not automatically open default browser")
    parser.add_argument("--backend-port", type=int, default=8000, help="Backend port (default: 8000)")
    parser.add_argument("--frontend-port", type=int, default=5173, help="Frontend port (default: 5173)")
    args = parser.parse_args()

    os.chdir(REPO_ROOT)

    print("=" * 68)
    print("      IndexForge — High-Performance Full-Text Search Engine")
    print("=" * 68)

    # 1. Verify/Build C++ Engine
    lib_path = find_engine_library()
    if not lib_path:
        print("[*] Engine library not found. Running CMake build...")
        if not build_engine():
            print("[!] Failed to build C++ engine library.")
            sys.exit(1)
        lib_path = find_engine_library()

    print(f"[✓] C++20 Core Library: {lib_path}")

    # 2. Check Dependencies
    check_python_deps()

    if args.build_only:
        print("[*] Building frontend distribution assets...")
        npm_bin = shutil.which("npm.cmd" if os.name == "nt" else "npm")
        if npm_bin:
            check_node_deps()
            subprocess.run([npm_bin, "run", "build"], cwd=FRONTEND_DIR)
        print("[✓] Build complete.")
        return

    backend_port = find_available_port(args.backend_port)
    if backend_port != args.backend_port:
        print(f"[*] Port {args.backend_port} is in use by another app. Assigned backend to port {backend_port}.")

    frontend_port = find_available_port(args.frontend_port)
    if frontend_port != args.frontend_port:
        print(f"[*] Port {args.frontend_port} is in use by another app. Assigned frontend to port {frontend_port}.")

    processes = []

    def shutdown_all():
        print("\n[*] Shutting down local services...")
        for p in processes:
            try:
                p.terminate()
                p.wait(timeout=3)
            except Exception:
                try:
                    p.kill()
                except Exception:
                    pass
        print("[✓] All services terminated.")

    # 3. Start Backend (FastAPI)
    if not args.frontend_only:
        print(f"[*] Starting FastAPI Backend on http://127.0.0.1:{backend_port}...")
        cmd_backend = [
            sys.executable, "-m", "uvicorn",
            "api.main:app",
            "--host", "127.0.0.1",
            "--port", str(backend_port),
            "--log-level", "warning"
        ]
        p_back = subprocess.Popen(cmd_backend, cwd=REPO_ROOT, stdin=subprocess.PIPE)
        processes.append(p_back)

    # 4. Start Frontend (React + Vite)
    if not args.backend_only:
        check_node_deps()
        npm_bin = shutil.which("npm.cmd" if os.name == "nt" else "npm")
        if not npm_bin:
            print("[!] npm not found. Cannot launch Vite dev server.")
        else:
            print(f"[*] Starting React + Vite Dev Server on http://localhost:{frontend_port}...")
            env = os.environ.copy()
            env["BACKEND_PORT"] = str(backend_port)
            env["FRONTEND_PORT"] = str(frontend_port)
            cmd_frontend = [npm_bin, "run", "dev", "--", "--host", "localhost", "--port", str(frontend_port)]
            p_front = subprocess.Popen(cmd_frontend, cwd=FRONTEND_DIR, shell=(os.name == 'nt'), env=env, stdin=subprocess.PIPE)
            processes.append(p_front)

    # 5. Health Check & Browser Launch
    target_url = f"http://localhost:{frontend_port}" if not args.backend_only else f"http://127.0.0.1:{backend_port}"
    health_url = f"http://127.0.0.1:{backend_port}/api/health" if not args.frontend_only else target_url

    print("\n" + "=" * 68)
    if not args.frontend_only:
        print(f"  Backend API:         http://127.0.0.1:{backend_port}")
        print(f"  API Documentation:   http://127.0.0.1:{backend_port}/docs")
    if not args.backend_only:
        print(f"  Frontend UI (Vite):  http://localhost:{frontend_port}")
    print("=" * 68 + "\n")

    if not args.no_browser:
        def open_browser():
            if wait_for_url(target_url, timeout=12):
                print(f"[+] Opening local interface in browser: {target_url}\n")
                webbrowser.open(target_url)
        threading.Thread(target=open_browser, daemon=True).start()

    print("[*] Local development stack is live. Press Ctrl+C to stop all services.\n")

    try:
        while True:
            for p in processes:
                if p.poll() is not None:
                    print(f"[!] Process {p.args} exited unexpectedly.")
                    shutdown_all()
                    sys.exit(1)
            time.sleep(0.5)
    except KeyboardInterrupt:
        shutdown_all()

if __name__ == "__main__":
    main()

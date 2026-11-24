#!/usr/bin/env python3
"""
IndexForge — Unified Hybrid C++20 & Java 21 Search Platform Launcher
Orchestrates:
  1. C++20 Search Engine Core via gRPC Server (port 50051)
  2. Java 21 + Spring Boot 3 Application Backend (http://127.0.0.1:8080)
  3. React 18 + TypeScript + Vite Frontend (http://localhost:5174)
"""

import os
import sys
import time
import socket
import shutil
import urllib.request
import subprocess
import threading
import webbrowser
import argparse

REPO_ROOT = os.path.dirname(os.path.abspath(__file__))
FRONTEND_DIR = os.path.join(REPO_ROOT, "frontend")
JAVA_API_DIR = os.path.join(REPO_ROOT, "java-api")
GRPC_SERVER_DIR = os.path.join(REPO_ROOT, "cpp-search-engine", "grpc_server")

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
        os.path.join(REPO_ROOT, "indexforge.dll"),
        # Linux / Unix
        os.path.join(REPO_ROOT, "build", "libindexforge.so"),
        os.path.join(REPO_ROOT, "libindexforge.so"),
        # macOS
        os.path.join(REPO_ROOT, "build", "libindexforge.dylib"),
    ]
    for path in candidates:
        if os.path.isfile(path):
            return path
    return None

def find_java_binary():
    """Locate Java 21 binary in tools/ or system PATH."""
    local_jdk = os.path.join(REPO_ROOT, "tools", "jdk-21", "bin", "java.exe" if os.name == "nt" else "java")
    if os.path.isfile(local_jdk):
        return local_jdk
    sys_java = shutil.which("java")
    if sys_java:
        return sys_java
    return None

def find_maven_binary():
    """Locate Apache Maven binary in tools/ or system PATH."""
    local_mvn = os.path.join(REPO_ROOT, "tools", "maven", "bin", "mvn.cmd" if os.name == "nt" else "mvn")
    if os.path.isfile(local_mvn):
        return local_mvn
    sys_mvn = shutil.which("mvn.cmd" if os.name == "nt" else "mvn")
    if sys_mvn:
        return sys_mvn
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

def wait_for_url(url, timeout=20):
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

def wait_for_socket(port, host="127.0.0.1", timeout=15):
    """Wait until a TCP port accepts socket connections."""
    start = time.time()
    while time.time() - start < timeout:
        if is_port_in_use(port, host):
            return True
        time.sleep(0.2)
    return False

def main():
    parser = argparse.ArgumentParser(description="IndexForge Hybrid Platform Local Launcher")
    parser.add_argument("--grpc-port", type=int, default=50051, help="C++ Engine gRPC port (default: 50051)")
    parser.add_argument("--backend-port", type=int, default=8080, help="Java Spring Boot API port (default: 8080)")
    parser.add_argument("--frontend-port", type=int, default=5174, help="React + Vite port (default: 5174)")
    parser.add_argument("--engine-only", action="store_true", help="Launch only C++ gRPC engine")
    parser.add_argument("--backend-only", action="store_true", help="Launch only gRPC engine + Java Spring Boot")
    parser.add_argument("--frontend-only", action="store_true", help="Launch only React + Vite dev server")
    parser.add_argument("--build-only", action="store_true", help="Build all modules and exit")
    parser.add_argument("--no-browser", action="store_true", help="Do not automatically open default browser")
    args = parser.parse_args()

    os.chdir(REPO_ROOT)

    print("=" * 72)
    print("   IndexForge — Hybrid C++20 & Java 21 Enterprise Search Platform")
    print("=" * 72)

    # 1. Verify/Build C++ Engine
    lib_path = find_engine_library()
    if not lib_path:
        print("[*] Engine library not found. Running CMake build...")
        if not build_engine():
            print("[!] Failed to build C++ engine library.")
            sys.exit(1)
        lib_path = find_engine_library()

    print(f"[✓] C++20 Core Library: {lib_path}")

    # 2. Check Java 21
    java_bin = find_java_binary()
    if not java_bin:
        print("[!] Java 21 not found. Please install Java 21 or extract OpenJDK to tools/jdk-21.")
        sys.exit(1)
    print(f"[✓] Java Runtime:       {java_bin}")

    # Check Spring Boot Fat JAR
    jar_path = os.path.join(JAVA_API_DIR, "target", "indexforge-java-api-1.0.0.jar")
    if not os.path.isfile(jar_path):
        print("[*] Spring Boot JAR not found. Packaging with Maven...")
        mvn_bin = find_maven_binary()
        mvn_settings = os.path.join(REPO_ROOT, "tools", "maven", "conf", "settings.xml")
        cmd_mvn = [mvn_bin, "package", "-DskipTests"]
        if os.path.isfile(mvn_settings):
            cmd_mvn.extend(["-s", mvn_settings])
        subprocess.run(cmd_mvn, cwd=JAVA_API_DIR, check=True)

    if args.build_only:
        print("[*] Building frontend distribution...")
        npm_bin = shutil.which("npm.cmd" if os.name == "nt" else "npm")
        if npm_bin:
            check_node_deps()
            subprocess.run([npm_bin, "run", "build"], cwd=FRONTEND_DIR)
        print("[✓] All modules built successfully.")
        return

    grpc_port = find_available_port(args.grpc_port)
    backend_port = find_available_port(args.backend_port)
    frontend_port = find_available_port(args.frontend_port)

    processes = []

    def shutdown_all():
        print("\n[*] Shutting down IndexForge services...")
        for p in processes:
            try:
                p.terminate()
                p.wait(timeout=2)
            except Exception:
                try:
                    p.kill()
                except Exception:
                    pass
        print("[✓] All services stopped.")

    # 3. Start C++ Search Engine gRPC Server
    if not args.frontend_only:
        print(f"[*] Starting C++ Search Engine gRPC Server on port {grpc_port}...")
        server_py = os.path.join(GRPC_SERVER_DIR, "server.py")
        cmd_grpc = [sys.executable, server_py, "--port", str(grpc_port)]
        p_grpc = subprocess.Popen(cmd_grpc, cwd=REPO_ROOT, stdin=subprocess.PIPE)
        processes.append(p_grpc)

        if not wait_for_socket(grpc_port, timeout=8):
            print(f"[!] Warning: C++ gRPC Server on port {grpc_port} taking longer to respond.")
        else:
            print(f"[✓] C++ gRPC Engine is listening on port {grpc_port}.")

    if args.engine_only:
        print(f"\n[*] C++ Engine running on port {grpc_port}. Press Ctrl+C to terminate.")
        try:
            while True:
                time.sleep(1)
        except KeyboardInterrupt:
            shutdown_all()
        return

    # 4. Start Java 21 Spring Boot Application
    if not args.frontend_only:
        print(f"[*] Starting Java 21 Spring Boot Backend on http://127.0.0.1:{backend_port}...")
        cmd_java = [
            java_bin,
            "-XX:+UseZGC",
            "-XX:+ZGenerational",
            "-Xms128m",
            "-Xmx512m",
            "-jar", jar_path,
            f"--server.port={backend_port}",
            f"--indexforge.grpc.port={grpc_port}",
        ]
        p_java = subprocess.Popen(cmd_java, cwd=REPO_ROOT, stdin=subprocess.PIPE)
        processes.append(p_java)

        health_url = f"http://127.0.0.1:{backend_port}/api/index/health"
        if wait_for_url(health_url, timeout=25):
            print(f"[✓] Spring Boot Backend initialized successfully on port {backend_port}.")
        else:
            print(f"[*] Backend starting up on port {backend_port}...")

    # 5. Start React + Vite Dev Server
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

    target_url = f"http://localhost:{frontend_port}" if not args.backend_only else f"http://127.0.0.1:{backend_port}"

    print("\n" + "=" * 72)
    print("                 IndexForge Services Are Running")
    print("=" * 72)
    if not args.frontend_only:
        print(f"  ⚡ C++ gRPC Engine Core:    127.0.0.1:{grpc_port}")
        print(f"  ☕ Spring Boot 3 REST API:   http://127.0.0.1:{backend_port}")
        print(f"  📖 Swagger / OpenAPI Docs:  http://127.0.0.1:{backend_port}/swagger-ui/index.html")
    if not args.backend_only:
        print(f"  💻 React 18 Web Interface:  http://localhost:{frontend_port}")
    print("=" * 72 + "\n")

    if not args.no_browser and not args.backend_only:
        def open_browser():
            if wait_for_url(target_url, timeout=15):
                print(f"[+] Opening local interface in browser: {target_url}\n")
                webbrowser.open(target_url)
        threading.Thread(target=open_browser, daemon=True).start()

    print("[*] Local development stack is live. Press Ctrl+C to stop all services.\n")

    try:
        while True:
            for p in processes:
                if p.poll() is not None:
                    print(f"[!] A service exited unexpectedly (code {p.returncode}).")
                    shutdown_all()
                    sys.exit(1)
            time.sleep(0.5)
    except KeyboardInterrupt:
        shutdown_all()

if __name__ == "__main__":
    main()

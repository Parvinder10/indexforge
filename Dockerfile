# =======================================================
# Stage 1: C++20 Core Build
# =======================================================
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    clang \
    libc++-dev \
    libc++abi-dev \
    git \
    ca-certificates \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY CMakeLists.txt ./
COPY include/ ./include/
COPY src/ ./src/
COPY tests/ ./tests/
COPY benchmarks/ ./benchmarks/

# Build static library, shared library (.so), CLI, tests, and benchmarks
RUN cmake -B build -G Ninja \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build

# Run unit tests to verify build integrity
RUN ./build/indexforge_tests

# =======================================================
# Stage 2: Production Python API & Runtime
# =======================================================
FROM python:3.11-slim AS runtime

ENV PYTHONUNBUFFERED=1 \
    PYTHONDONTWRITEBYTECODE=1

WORKDIR /app

# Install runtime dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 \
    sqlite3 \
 && rm -rf /var/lib/apt/lists/*

# Copy built C++ shared library and executables from builder
COPY --from=builder /app/build/indexforge.so /app/build/indexforge.so
COPY --from=builder /app/build/indexforge_cli /app/build/indexforge_cli
COPY --from=builder /app/build/indexforge_benchmark /app/build/indexforge_benchmark

# Copy Python API code, sample documents, and data directory
COPY api/ ./api/
COPY data/ ./data/

# Install Python requirements
RUN pip install --no-cache-dir \
    fastapi>=0.100.0 \
    uvicorn>=0.20.0 \
    pydantic>=2.0.0 \
    requests>=2.28.0

EXPOSE 8000

HEALTHCHECK --interval=30s --timeout=5s --start-period=5s --retries=3 \
  CMD python -c "import urllib.request; urllib.request.urlopen('http://127.0.0.1:8000/api/health')" || exit 1

CMD ["python", "-m", "uvicorn", "api.main:app", "--host", "0.0.0.0", "--port", "8000"]

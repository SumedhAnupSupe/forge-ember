#!/usr/bin/env bash
# One-command setup, build, and local launch for FORGE + FastAPI + POSTO.
# Run from any directory: bash start-ember.sh
set -Eeuo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
FORGE_DIR="$ROOT/forge"
BACKEND_DIR="$ROOT/backend"
FRONTEND_DIR="$ROOT/ember/posto-—-elegant-mail-client"
BUILD_DIR="$ROOT/build/forge"
LOG_DIR="$ROOT/.run-logs"
PIDS=()

info() { printf '\n\033[1;34m[EMBER]\033[0m %s\n' "$*"; }
warn() { printf '\n\033[1;33m[WARN]\033[0m %s\n' "$*" >&2; }
die() { printf '\n\033[1;31m[ERROR]\033[0m %s\n' "$*" >&2; exit 1; }

cleanup() {
  local pid
  if (("${#PIDS[@]}" > 0)); then
    info "Stopping servers..."
    for pid in "${PIDS[@]}"; do kill "$pid" 2>/dev/null || true; done
    for pid in "${PIDS[@]}"; do wait "$pid" 2>/dev/null || true; done
  fi
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

[[ -d "$FORGE_DIR" && -d "$BACKEND_DIR" && -d "$FRONTEND_DIR" ]] || die "Run this script from the repository checkout; expected forge/, backend/, and ember/posto-—-elegant-mail-client/."

install_system_deps() {
  if command -v apt-get >/dev/null 2>&1; then
    info "Installing required system packages (sudo may ask for your password)..."
    if [[ $EUID -eq 0 ]]; then
      apt-get update
      DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential cmake pkg-config libargon2-dev curl ca-certificates git python3 python3-venv
    elif command -v sudo >/dev/null 2>&1; then
      sudo apt-get update
      sudo DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential cmake pkg-config libargon2-dev curl ca-certificates git python3 python3-venv
    else
      die "apt-get is available but sudo is not. Install build-essential, cmake, pkg-config, libargon2-dev, curl, and python3-venv manually."
    fi
  else
    warn "Not a Debian/Ubuntu system: skipping system package installation. Install a C++17 compiler, CMake, libargon2 headers/library, Python 3.11+, Node.js 20.19+/22.12+, and npm manually."
  fi
}

ensure_uv() {
  if ! command -v uv >/dev/null 2>&1; then
    info "Installing uv for the current user..."
    command -v curl >/dev/null 2>&1 || die "curl is required to install uv."
    curl -LsSf https://astral.sh/uv/install.sh | sh
    export PATH="$HOME/.local/bin:$HOME/.cargo/bin:$PATH"
  fi
  command -v uv >/dev/null 2>&1 || die "uv installation did not produce an executable on PATH. Install uv from https://docs.astral.sh/uv/ and rerun."
}

check_node() {
  command -v node >/dev/null 2>&1 || die "Node.js is missing. Install Node.js 22 LTS and npm, then rerun."
  command -v npm >/dev/null 2>&1 || die "npm is missing. Install npm alongside Node.js, then rerun."
  local major minor
  major="$(node -p 'Number(process.versions.node.split(".")[0])')"
  minor="$(node -p 'Number(process.versions.node.split(".")[1])')"
  if (( major < 20 || (major == 20 && minor < 19) || (major == 21) || (major == 22 && minor < 12) )); then
    die "Node.js $(node --version) is too old for this frontend's Vite version. Install Node.js 22.12+ (recommended: Node.js 22 LTS) and rerun."
  fi
}

wait_for_http() {
  local url="$1" label="$2" attempts="${3:-30}"
  local n
  for ((n=1; n<=attempts; n++)); do
    if curl --silent --fail "$url" >/dev/null; then
      info "$label is responding at $url"
      return 0
    fi
    sleep 1
  done
  warn "$label did not answer $url in time. Check logs under $LOG_DIR."
  return 1
}

# Set INSTALL_SYSTEM_DEPS=0 to skip apt installation on systems where dependencies
# are already managed separately.
if [[ "${INSTALL_SYSTEM_DEPS:-1}" == "1" ]]; then install_system_deps; fi
ensure_uv
check_node
mkdir -p "$LOG_DIR" "$ROOT/forge/data"

info "Installing Python backend dependencies with uv..."
(
  cd "$BACKEND_DIR"
  [[ -f .env ]] || cp .env.example .env
  uv sync
)

info "Installing frontend npm dependencies..."
(
  cd "$FRONTEND_DIR"
  if [[ -f package-lock.json ]]; then npm ci; else npm install; fi
)

info "Configuring and building FORGE C++ targets..."
cmake -S "$FORGE_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --parallel "$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"

if [[ "${RUN_FORGE_TESTS:-1}" == "1" ]]; then
  info "Running FORGE tests..."
  ctest --test-dir "$BUILD_DIR" --output-on-failure
fi

info "Starting FORGE HTTP server on 127.0.0.1:8080..."
"$BUILD_DIR/forge-server" --host 127.0.0.1 --port 8080 --data-dir "$ROOT/forge/data" >"$LOG_DIR/forge.log" 2>&1 &
PIDS+=("$!")

wait_for_http "http://127.0.0.1:8080/health" "FORGE" 30 || {
  cat "$LOG_DIR/forge.log" >&2
  die "FORGE failed to start."
}

info "Starting FastAPI on 127.0.0.1:8000..."
(
  cd "$BACKEND_DIR"
  exec uv run uvicorn app.main:app --host 127.0.0.1 --port 8000
) >"$LOG_DIR/backend.log" 2>&1 &
PIDS+=("$!")

wait_for_http "http://127.0.0.1:8000/health" "FastAPI" 30 || {
  cat "$LOG_DIR/backend.log" >&2
  die "FastAPI failed to start."
}

info "Checking frontend TypeScript..."
(
  cd "$FRONTEND_DIR"
  npm run lint
)

info "Starting POSTO frontend on 0.0.0.0:3000..."
(
  cd "$FRONTEND_DIR"
  exec npm run dev
) >"$LOG_DIR/frontend.log" 2>&1 &
PIDS+=("$!")

sleep 2
if ! kill -0 "${PIDS[2]}" 2>/dev/null; then
  cat "$LOG_DIR/frontend.log" >&2
  die "Frontend failed to start."
fi

cat <<EOF

============================================================
 EMBER + FORGE is running
============================================================
 POSTO frontend : http://localhost:3000
 FastAPI docs   : http://127.0.0.1:8000/docs
 FastAPI health : http://127.0.0.1:8000/health
 FORGE health   : http://127.0.0.1:8080/health

 Logs:
   $LOG_DIR/forge.log
   $LOG_DIR/backend.log
   $LOG_DIR/frontend.log

Press Ctrl+C to stop all three servers.
Note: FORGE data persists under forge/data/ between runs.
============================================================

EOF

# Keep the launcher in the foreground and shut down child servers on Ctrl+C.
while true; do
  for pid in "${PIDS[@]}"; do
    if ! kill -0 "$pid" 2>/dev/null; then
      warn "A server process (PID $pid) exited. Inspect logs in $LOG_DIR."
      exit 1
    fi
  done
  sleep 2
done

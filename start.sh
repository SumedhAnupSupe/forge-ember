#!/usr/bin/env bash
# One-command setup, build, test, and launch for FORGE + EMBER + POSTO.
# Run from any directory: ./start.sh
set -Eeuo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
FORGE_DIR="$ROOT/forge"
BACKEND_DIR="$ROOT/backend"
FRONTEND_DIR="$ROOT/ember/posto-—-elegant-mail-client"
BUILD_DIR="$ROOT/build/forge"
LOG_DIR="$ROOT/.run-logs"
PIDS=()

info() { printf '\n\033[1;34m[FORGE + EMBER]\033[0m %s\n' "$*"; }
warn() { printf '\n\033[1;33m[WARN]\033[0m %s\n' "$*" >&2; }
die() { printf '\n\033[1;31m[ERROR]\033[0m %s\n' "$*" >&2; exit 1; }

cleanup() {
  local pid
  if (("${#PIDS[@]}" > 0)); then
    info "Stopping application services..."
    for pid in "${PIDS[@]}"; do kill "$pid" 2>/dev/null || true; done
    for pid in "${PIDS[@]}"; do wait "$pid" 2>/dev/null || true; done
  fi
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

[[ -d "$FORGE_DIR" && -d "$BACKEND_DIR" && -d "$FRONTEND_DIR" ]] ||
  die "Run this script from the repository checkout; expected forge/, backend/, and ember/posto-—-elegant-mail-client/."

install_system_deps() {
  if ! command -v apt-get >/dev/null 2>&1; then
    warn "apt-get is unavailable. Install a C++17 compiler, CMake, pkg-config, libargon2-dev, curl, Python 3.11+, Node.js 22.12+ and npm manually."
    return
  fi

  info "Installing FORGE system dependencies (sudo may ask for your password)..."
  if [[ $EUID -eq 0 ]]; then
    apt-get update
    DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential cmake pkg-config libargon2-dev curl ca-certificates git python3 python3-venv
  elif command -v sudo >/dev/null 2>&1; then
    sudo apt-get update
    sudo DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential cmake pkg-config libargon2-dev curl ca-certificates git python3 python3-venv
  else
    die "apt-get is available but sudo is not. Install build-essential, cmake, pkg-config, libargon2-dev, curl, ca-certificates, git, and python3-venv manually."
  fi
}

ensure_uv() {
  if ! command -v uv >/dev/null 2>&1; then
    info "Installing uv for the current user..."
    command -v curl >/dev/null 2>&1 || die "curl is required to install uv."
    curl -LsSf https://astral.sh/uv/install.sh | sh
    export PATH="$HOME/.local/bin:$HOME/.cargo/bin:$PATH"
  fi
  command -v uv >/dev/null 2>&1 || die "uv installation failed. Install it from https://docs.astral.sh/uv/ and rerun."
}

ensure_node() {
  if command -v node >/dev/null 2>&1 && command -v npm >/dev/null 2>&1; then
    return
  fi

  command -v apt-get >/dev/null 2>&1 ||
    die "Node.js/npm are missing. Install Node.js 22 LTS and npm, then rerun."

  info "Installing Node.js 22 LTS and npm..."
  local setup_file
  setup_file="$(mktemp)"
  curl -fsSL https://deb.nodesource.com/setup_22.x -o "$setup_file"
  if [[ $EUID -eq 0 ]]; then
    bash "$setup_file"
    DEBIAN_FRONTEND=noninteractive apt-get install -y nodejs
  elif command -v sudo >/dev/null 2>&1; then
    sudo bash "$setup_file"
    sudo DEBIAN_FRONTEND=noninteractive apt-get install -y nodejs
  else
    rm -f "$setup_file"
    die "Node.js is missing and sudo is unavailable. Install Node.js 22 LTS and npm manually."
  fi
  rm -f "$setup_file"
}

check_node() {
  command -v node >/dev/null 2>&1 || die "Node.js is missing."
  command -v npm >/dev/null 2>&1 || die "npm is missing."
  local major minor
  major="$(node -p 'Number(process.versions.node.split(".")[0])')"
  minor="$(node -p 'Number(process.versions.node.split(".")[1])')"
  if (( major < 20 || (major == 20 && minor < 19) || major == 21 || (major == 22 && minor < 12) )); then
    die "Node.js $(node --version) is too old for this Vite version. Use Node.js 22.12+ (recommended: Node.js 22 LTS)."
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

if [[ "${INSTALL_SYSTEM_DEPS:-1}" == "1" ]]; then install_system_deps; fi
ensure_uv
ensure_node
check_node

mkdir -p "$LOG_DIR" "$ROOT/forge/data"

info "Installing Python backend dependencies..."
(
  cd "$BACKEND_DIR"
  [[ -f .env ]] || cp .env.example .env
  uv sync
)

info "Installing POSTO frontend dependencies..."
(
  cd "$FRONTEND_DIR"
  if [[ -f package-lock.json ]]; then npm ci; else npm install; fi
)

info "Configuring and building FORGE..."
cmake -S "$FORGE_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --parallel "$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"

if [[ "${RUN_FORGE_TESTS:-1}" == "1" ]]; then
  info "Running FORGE test suite..."
  ctest --test-dir "$BUILD_DIR" --output-on-failure
fi

info "Starting FORGE HTTP server on 127.0.0.1:8080..."
"$BUILD_DIR/forge-server" --host 127.0.0.1 --port 8080 --data-dir "$ROOT/forge/data" >"$LOG_DIR/forge.log" 2>&1 &
PIDS+=("$!")

wait_for_http "http://127.0.0.1:8080/health" "FORGE" 30 || {
  cat "$LOG_DIR/forge.log" >&2
  die "FORGE failed to start."
}

info "Starting EMBER FastAPI on 127.0.0.1:8000..."
(
  cd "$BACKEND_DIR"
  exec uv run uvicorn app.main:app --host 127.0.0.1 --port 8000
) >"$LOG_DIR/backend.log" 2>&1 &
PIDS+=("$!")

wait_for_http "http://127.0.0.1:8000/health" "EMBER" 30 || {
  cat "$LOG_DIR/backend.log" >&2
  die "EMBER failed to start."
}

info "Checking POSTO TypeScript..."
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
  die "POSTO failed to start."
fi

cat <<EOF

============================================================
 FORGE + EMBER + POSTO are running
============================================================
 POSTO frontend : http://localhost:3000
 EMBER API docs : http://127.0.0.1:8000/docs
 EMBER health   : http://127.0.0.1:8000/health
 FORGE health   : http://127.0.0.1:8080/health

 Logs:
   $LOG_DIR/forge.log
   $LOG_DIR/backend.log
   $LOG_DIR/frontend.log

Press Ctrl+C to stop all three services.
FORGE data persists under forge/data/ between runs.
============================================================

EOF

while true; do
  for pid in "${PIDS[@]}"; do
    if ! kill -0 "$pid" 2>/dev/null; then
      warn "A service process (PID $pid) exited. Inspect logs in $LOG_DIR."
      exit 1
    fi
  done
  sleep 2
done

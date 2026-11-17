#!/usr/bin/env bash
# One-command local setup and launcher for Forge + Ember + Posto (no Docker).
set -Eeuo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
FORGE_DIR="$ROOT_DIR/forge"
BACKEND_DIR="$ROOT_DIR/backend"
FRONTEND_DIR="$ROOT_DIR/ember/posto-—-elegant-mail-client"
BUILD_DIR="$ROOT_DIR/build/forge"
DATA_DIR="$ROOT_DIR/forge/data"
LOG_DIR="$ROOT_DIR/logs"
FORGE_PORT=8080
EMBER_PORT=8000
POSTO_PORT=3000
PIDS=()

log()  { printf '\n[dev] %s\n' "$*"; }
ok()   { printf '[ ok ] %s\n' "$*"; }
warn() { printf '[warn] %s\n' "$*" >&2; }
die()  { printf '[error] %s\n' "$*" >&2; exit 1; }

usage() {
  cat <<USAGE
Usage: ./start.sh [options]
  --setup-only   Install dependencies, build and test; do not start servers.
  --no-tests     Skip the FORGE CTest suite.
  --help         Show this help.

Default: install prerequisites, install project dependencies, build/test,
then start FORGE, EMBER and POSTO. Press Ctrl+C to stop all services.
Ubuntu/Debian only. No Docker required.
USAGE
}

SETUP_ONLY=0
RUN_TESTS=1
for arg in "$@"; do
  case "$arg" in
    --setup-only) SETUP_ONLY=1 ;;
    --no-tests) RUN_TESTS=0 ;;
    --help|-h) usage; exit 0 ;;
    *) die "Unknown option: $arg (use --help)" ;;
  esac
done

[[ -d "$FORGE_DIR" && -d "$BACKEND_DIR" && -d "$FRONTEND_DIR" ]] || \
  die "Expected forge/, backend/, and ember/posto-—-elegant-mail-client/ beside this script. Put start.sh in the repository root."
command -v sudo >/dev/null 2>&1 || die "sudo is required to install system packages."

log "Installing Ubuntu system dependencies..."
sudo apt-get update
sudo apt-get install -y build-essential cmake g++ make libargon2-dev curl ca-certificates git python3 python3-venv

NODE_MAJOR=""
if command -v node >/dev/null 2>&1; then
  NODE_MAJOR="$(node -p 'process.versions.node.split(".")[0]' 2>/dev/null || true)"
fi
if [[ -z "$NODE_MAJOR" || "$NODE_MAJOR" -lt 22 ]]; then
  log "Installing Node.js 22..."
  curl -fsSL https://deb.nodesource.com/setup_22.x | sudo -E bash -
  sudo apt-get install -y nodejs
fi

export PATH="$HOME/.local/bin:$PATH"
if ! command -v uv >/dev/null 2>&1; then
  log "Installing uv..."
  curl -LsSf https://astral.sh/uv/install.sh | sh
  export PATH="$HOME/.local/bin:$PATH"
fi

for tool in cmake g++ node npm uv; do command -v "$tool" >/dev/null 2>&1 || die "$tool is not available after installation."; done
log "Tool versions:"
cmake --version | head -n 1
g++ --version | head -n 1
node --version
npm --version
uv --version

# Copy backend defaults only if the repository supplies an example and no local file exists.
if [[ -f "$BACKEND_DIR/.env.example" && ! -f "$BACKEND_DIR/.env" ]]; then
  cp "$BACKEND_DIR/.env.example" "$BACKEND_DIR/.env"
  ok "Created backend/.env from .env.example"
fi
# Preserve any existing frontend local configuration.
if [[ ! -f "$FRONTEND_DIR/.env.local" ]]; then
  printf '%s\n' 'VITE_API_BASE_URL=http://127.0.0.1:8000/api/v1' > "$FRONTEND_DIR/.env.local"
  ok "Created POSTO .env.local"
fi
mkdir -p "$DATA_DIR" "$LOG_DIR"

log "Installing EMBER Python dependencies..."
(cd "$BACKEND_DIR" && uv sync)
ok "EMBER dependencies installed"

log "Installing POSTO Node dependencies..."
(cd "$FRONTEND_DIR" && npm ci)
ok "POSTO dependencies installed"

log "Configuring and building FORGE..."
cmake -S "$FORGE_DIR" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" --clean-first -j"$(nproc)"
ok "FORGE build completed"

if [[ "$RUN_TESTS" -eq 1 ]]; then
  log "Running FORGE tests..."
  ctest --test-dir "$BUILD_DIR" --output-on-failure
  ok "FORGE tests passed"
fi

log "Checking POSTO TypeScript and production build..."
(cd "$FRONTEND_DIR" && npm run lint && npm run build)
ok "POSTO checks passed"

if [[ "$SETUP_ONLY" -eq 1 ]]; then
  log "Setup completed. Later run ./start.sh --no-tests to start all services."
  exit 0
fi

cleanup() {
  local pid
  log "Stopping local services..."
  for pid in "${PIDS[@]:-}"; do
    if kill -0 "$pid" 2>/dev/null; then kill "$pid" 2>/dev/null || true; fi
  done
  for pid in "${PIDS[@]:-}"; do wait "$pid" 2>/dev/null || true; done
  ok "All launched services stopped"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

wait_for_url() {
  local url="$1" label="$2" pid="$3" attempt log_file
  log_file="$LOG_DIR/${label,,}.log"
  for attempt in $(seq 1 40); do
    if curl --silent --fail "$url" >/dev/null 2>&1; then
      ok "$label is responding at $url"
      return 0
    fi
    if ! kill -0 "$pid" 2>/dev/null; then
      warn "$label process exited. Recent log:"
      tail -n 80 "$log_file" 2>/dev/null || true
      return 1
    fi
    sleep 1
  done
  warn "$label did not respond at $url within 40 seconds. Recent log:"
  tail -n 80 "$log_file" 2>/dev/null || true
  return 1
}

log "Starting FORGE on 127.0.0.1:${FORGE_PORT}..."
(cd "$ROOT_DIR" && exec "$BUILD_DIR/forge-server" --host 127.0.0.1 --port "$FORGE_PORT" --data-dir "$DATA_DIR") >"$LOG_DIR/forge.log" 2>&1 &
FORGE_PID=$!
PIDS+=("$FORGE_PID")
wait_for_url "http://127.0.0.1:${FORGE_PORT}/health" "FORGE" "$FORGE_PID" || exit 1

log "Starting EMBER on 127.0.0.1:${EMBER_PORT}..."
(cd "$BACKEND_DIR" && exec uv run uvicorn app.main:app --host 127.0.0.1 --port "$EMBER_PORT") >"$LOG_DIR/ember.log" 2>&1 &
EMBER_PID=$!
PIDS+=("$EMBER_PID")
wait_for_url "http://127.0.0.1:${EMBER_PORT}/docs" "EMBER" "$EMBER_PID" || exit 1

log "Starting POSTO on 0.0.0.0:${POSTO_PORT}..."
(cd "$FRONTEND_DIR" && exec npm run dev -- --host 0.0.0.0) >"$LOG_DIR/posto.log" 2>&1 &
POSTO_PID=$!
PIDS+=("$POSTO_PID")
wait_for_url "http://127.0.0.1:${POSTO_PORT}" "POSTO" "$POSTO_PID" || exit 1

cat <<SUMMARY

============================================================
 FORGE + EMBER + POSTO are running (without Docker)

 POSTO:  http://localhost:${POSTO_PORT}
 EMBER:  http://127.0.0.1:${EMBER_PORT}/docs
 FORGE:  http://127.0.0.1:${FORGE_PORT}/health

 Logs:   $LOG_DIR/{forge,ember,posto}.log
 Data:   $DATA_DIR

 Press Ctrl+C here to stop all three services.
============================================================
SUMMARY

while true; do
  for i in "${!PIDS[@]}"; do
    pid="${PIDS[$i]}"
    if ! kill -0 "$pid" 2>/dev/null; then
      case "$i" in 0) name=FORGE ;; 1) name=EMBER ;; 2) name=POSTO ;; esac
      warn "$name has stopped. Check $LOG_DIR/${name,,}.log"
      exit 1
    fi
  done
  sleep 2
done

#!/usr/bin/env bash
set -euo pipefail

SERVER="./build/forge/forge-server"
PORT="${FORGE_TEST_PORT:-18473}"
DATA_DIR="$(mktemp -d)"
RESPONSE="$DATA_DIR/response.json"
LOG="$DATA_DIR/server.log"
PID=""

cleanup() {
  if [[ -n "$PID" ]]; then
    kill "$PID" 2>/dev/null || true
    wait "$PID" 2>/dev/null || true
  fi
  rm -rf "$DATA_DIR"
}
trap cleanup EXIT

request() {
  curl -sS -o "$RESPONSE" -w '%{http_code}' "$@"
}

json_value() {
  python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))[sys.argv[2]])' "$RESPONSE" "$1"
}

assert_code() {
  local expected="$1"
  local actual="$2"
  if [[ "$actual" != "$expected" ]]; then
    echo "Expected HTTP $expected, got $actual"
    cat "$RESPONSE"
    exit 1
  fi
}

"$SERVER" --host 127.0.0.1 --port "$PORT" --data-dir "$DATA_DIR/data" >"$LOG" 2>&1 &
PID="$!"

ready=0
for _ in $(seq 1 40); do
  if curl -fsS "http://127.0.0.1:$PORT/health" >"$RESPONSE"; then ready=1; break; fi
  sleep 0.25
done
if [[ "$ready" != "1" ]]; then
  echo "FORGE server did not become healthy"
  cat "$LOG"
  exit 1
fi

# Logout must reject missing credentials.
code=$(request -X POST "http://127.0.0.1:$PORT/api/v1/auth/logout")
assert_code 401 "$code"

# Register three isolated users.
code=$(request -X POST "http://127.0.0.1:$PORT/api/v1/users" -H 'Content-Type: application/json' -d '{"name":"Alice Smoke","email":"alice-smoke@example.test","password":"SmokePass123!"}')
assert_code 201 "$code"
ALICE_ID="$(json_value id)"

code=$(request -X POST "http://127.0.0.1:$PORT/api/v1/users" -H 'Content-Type: application/json' -d '{"name":"Bob Smoke","email":"bob-smoke@example.test","password":"SmokePass123!"}')
assert_code 201 "$code"
BOB_ID="$(json_value id)"

code=$(request -X POST "http://127.0.0.1:$PORT/api/v1/users" -H 'Content-Type: application/json' -d '{"name":"Carol Smoke","email":"carol-smoke@example.test","password":"SmokePass123!"}')
assert_code 201 "$code"

code=$(request -X POST "http://127.0.0.1:$PORT/api/v1/auth/login" -H 'Content-Type: application/json' -d '{"email":"alice-smoke@example.test","password":"SmokePass123!"}')
assert_code 200 "$code"
ALICE_TOKEN="$(json_value token)"

code=$(request -X POST "http://127.0.0.1:$PORT/api/v1/auth/login" -H 'Content-Type: application/json' -d '{"email":"bob-smoke@example.test","password":"SmokePass123!"}')
assert_code 200 "$code"
BOB_TOKEN="$(json_value token)"

code=$(request -X POST "http://127.0.0.1:$PORT/api/v1/auth/login" -H 'Content-Type: application/json' -d '{"email":"carol-smoke@example.test","password":"SmokePass123!"}')
assert_code 200 "$code"
CAROL_TOKEN="$(json_value token)"

# Schema metadata is served by the server and requires authentication.
code=$(request "http://127.0.0.1:$PORT/api/v1/schema/tables" -H "Authorization: Bearer $ALICE_TOKEN")
assert_code 200 "$code"
python3 - "$RESPONSE" <<'PY'
import json, sys
assert json.load(open(sys.argv[1]))["tables"] == ["users", "messages", "recipients"]
PY

# SELECT is read-only and scoped to the caller, including SELECT *.
code=$(request -X POST "http://127.0.0.1:$PORT/api/v1/query" -H "Authorization: Bearer $ALICE_TOKEN" -H 'Content-Type: application/json' -d '{"query":"SELECT * FROM users;"}')
assert_code 200 "$code"
python3 - "$RESPONSE" "$ALICE_ID" <<'PY'
import json, sys
r = json.load(open(sys.argv[1]))
assert r["row_count"] == 1 and len(r["rows"]) == 1
assert r["rows"][0]["id"] == sys.argv[2]
assert "password_hash" not in r["rows"][0]
PY

# Reject multiple statements and unauthenticated queries.
code=$(request -X POST "http://127.0.0.1:$PORT/api/v1/query" -H "Authorization: Bearer $ALICE_TOKEN" -H 'Content-Type: application/json' -d '{"query":"SELECT * FROM users; SELECT * FROM messages;"}')
assert_code 400 "$code"
code=$(request -X POST "http://127.0.0.1:$PORT/api/v1/query" -H 'Content-Type: application/json' -d '{"query":"SELECT * FROM users;"}')
assert_code 401 "$code"

# Create a message from Alice to Bob; both sender and recipient can list/read it.
code=$(request -X POST "http://127.0.0.1:$PORT/api/v1/messages" -H "Authorization: Bearer $ALICE_TOKEN" -H 'Content-Type: application/json' -d "{\"subject\":\"Smoke subject\",\"body\":\"Smoke body\",\"recipients\":[{\"user_id\":$BOB_ID,\"type\":\"TO\"}]}")
assert_code 201 "$code"
MESSAGE_ID="$(json_value id)"

code=$(request "http://127.0.0.1:$PORT/api/v1/messages" -H "Authorization: Bearer $ALICE_TOKEN")
assert_code 200 "$code"
python3 - "$RESPONSE" "$MESSAGE_ID" <<'PY'
import json, sys
rows = json.load(open(sys.argv[1]))
assert any(str(row["id"]) == sys.argv[2] for row in rows)
PY

code=$(request "http://127.0.0.1:$PORT/api/v1/messages/$MESSAGE_ID" -H "Authorization: Bearer $BOB_TOKEN")
assert_code 200 "$code"

# A non-participant cannot read or delete the message.
code=$(request "http://127.0.0.1:$PORT/api/v1/messages/$MESSAGE_ID" -H "Authorization: Bearer $CAROL_TOKEN")
assert_code 404 "$code"
code=$(request -X DELETE "http://127.0.0.1:$PORT/api/v1/messages/$MESSAGE_ID" -H "Authorization: Bearer $BOB_TOKEN")
assert_code 403 "$code"

# Only the sender can delete the message.
code=$(request -X DELETE "http://127.0.0.1:$PORT/api/v1/messages/$MESSAGE_ID" -H "Authorization: Bearer $ALICE_TOKEN")
assert_code 200 "$code"
code=$(request "http://127.0.0.1:$PORT/api/v1/messages/$MESSAGE_ID" -H "Authorization: Bearer $BOB_TOKEN")
assert_code 404 "$code"

# Logout invalidates the token.
code=$(request -X POST "http://127.0.0.1:$PORT/api/v1/auth/logout" -H "Authorization: Bearer $ALICE_TOKEN")
assert_code 200 "$code"
code=$(request "http://127.0.0.1:$PORT/api/v1/users/me" -H "Authorization: Bearer $ALICE_TOKEN")
assert_code 401 "$code"

# Restart the server against the same data directory and verify durable user data.
kill "$PID"
wait "$PID" 2>/dev/null || true
PID=""
"$SERVER" --host 127.0.0.1 --port "$PORT" --data-dir "$DATA_DIR/data" >"$LOG" 2>&1 &
PID="$!"
ready=0
for _ in $(seq 1 40); do
  if curl -fsS "http://127.0.0.1:$PORT/health" >"$RESPONSE"; then ready=1; break; fi
  sleep 0.25
done
if [[ "$ready" != "1" ]]; then
  echo "FORGE server failed to restart"
  cat "$LOG"
  exit 1
fi
code=$(request -X POST "http://127.0.0.1:$PORT/api/v1/auth/login" -H 'Content-Type: application/json' -d '{"email":"alice-smoke@example.test","password":"SmokePass123!"}')
assert_code 200 "$code"
RESTARTED_TOKEN="$(json_value token)"
code=$(request "http://127.0.0.1:$PORT/api/v1/users/me" -H "Authorization: Bearer $RESTARTED_TOKEN")
assert_code 200 "$code"
python3 - "$RESPONSE" "$ALICE_ID" <<'PY'
import json, sys
assert str(json.load(open(sys.argv[1]))["id"]) == sys.argv[2]
PY

echo "FORGE API smoke tests passed"

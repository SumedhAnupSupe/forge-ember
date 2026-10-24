#!/usr/bin/env bash
set -Eeuo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FORGE_BIN="$ROOT/build/forge/forge-server"
EMBER_BIN="$ROOT/build/ember/ember-server"
DATA_DIR="$ROOT/build/curl-e2e-data"
LOG_DIR="$ROOT/build/curl-e2e-logs"
FORGE_PID=""
EMBER_PID=""

cleanup() {
  [[ -n "$EMBER_PID" ]] && kill "$EMBER_PID" 2>/dev/null || true
  [[ -n "$FORGE_PID" ]] && kill "$FORGE_PID" 2>/dev/null || true
}
trap cleanup EXIT

fail() { echo "FAIL: $*" >&2; [[ -f "$LOG_DIR/forge.log" ]] && tail -80 "$LOG_DIR/forge.log" >&2 || true; [[ -f "$LOG_DIR/ember.log" ]] && tail -80 "$LOG_DIR/ember.log" >&2 || true; exit 1; }
pass() { echo "PASS: $*"; }

command -v curl >/dev/null || fail "curl is required"
command -v jq >/dev/null || fail "jq is required"
[[ -x "$FORGE_BIN" ]] || fail "FORGE executable missing: $FORGE_BIN"
[[ -x "$EMBER_BIN" ]] || fail "EMBER executable missing: $EMBER_BIN"

rm -rf "$DATA_DIR" "$LOG_DIR"
mkdir -p "$DATA_DIR" "$LOG_DIR"

"$FORGE_BIN" --host 127.0.0.1 --port 8080 --data-dir "$DATA_DIR" >"$LOG_DIR/forge.log" 2>&1 &
FORGE_PID=$!
for _ in $(seq 1 60); do
  if curl -fsS http://127.0.0.1:8080/health >/dev/null; then break; fi
  kill -0 "$FORGE_PID" 2>/dev/null || fail "FORGE exited before becoming healthy"
  sleep 1
done
curl -fsS http://127.0.0.1:8080/health | jq -e '.status == "ok"' >/dev/null || fail "FORGE health check failed"
pass "FORGE started and health endpoint returns JSON"

FORGE_ORIGIN=http://127.0.0.1:8080 EMBER_HOST=127.0.0.1 EMBER_PORT=8081 "$EMBER_BIN" >"$LOG_DIR/ember.log" 2>&1 &
EMBER_PID=$!
for _ in $(seq 1 60); do
  if curl -fsS http://127.0.0.1:8081/health >/dev/null; then break; fi
  kill -0 "$EMBER_PID" 2>/dev/null || fail "EMBER exited before becoming healthy"
  sleep 1
done
curl -fsS http://127.0.0.1:8081/health | jq -e '.status == "ok" and .service == "ember"' >/dev/null || fail "EMBER health check failed"
pass "EMBER started and health endpoint returns JSON"

suffix="$(date +%s)-$$"
email_a="curl-sender-${suffix}@example.test"
email_b="curl-recipient-${suffix}@example.test"
password='Curl-E2E-Password-42!'
subject_ab="Curl E2E A to B ${suffix}"
body_ab="Message from account A to account B ${suffix}"
subject_ba="Curl E2E B to A ${suffix}"
body_ba="Reply from account B to account A ${suffix}"

register_a="$(curl -sS -w '\n%{http_code}' -H 'Content-Type: application/json' -d "$(jq -nc --arg name 'Curl Sender' --arg email "$email_a" --arg password "$password" '{name:$name,email:$email,password:$password}')" http://127.0.0.1:8081/api/v1/users)"
status="${register_a##*$'\n'}"; body="${register_a%$'\n'*}"
[[ "$status" == "201" ]] || fail "Register account A returned HTTP $status: $body"
id_a="$(jq -er '.id' <<<"$body")"
pass "Account A registered (id=$id_a)"

register_b="$(curl -sS -w '\n%{http_code}' -H 'Content-Type: application/json' -d "$(jq -nc --arg name 'Curl Recipient' --arg email "$email_b" --arg password "$password" '{name:$name,email:$email,password:$password}')" http://127.0.0.1:8081/api/v1/users)"
status="${register_b##*$'\n'}"; body="${register_b%$'\n'*}"
[[ "$status" == "201" ]] || fail "Register account B returned HTTP $status: $body"
id_b="$(jq -er '.id' <<<"$body")"
pass "Account B registered (id=$id_b)"

login_a="$(curl -fsS -H 'Content-Type: application/json' -d "$(jq -nc --arg email "$email_a" --arg password "$password" '{email:$email,password:$password}')" http://127.0.0.1:8081/api/v1/auth/login)" || fail "Account A login failed"
token_a="$(jq -er '.token' <<<"$login_a")"
login_b="$(curl -fsS -H 'Content-Type: application/json' -d "$(jq -nc --arg email "$email_b" --arg password "$password" '{email:$email,password:$password}')" http://127.0.0.1:8081/api/v1/auth/login)" || fail "Account B login failed"
token_b="$(jq -er '.token' <<<"$login_b")"
curl -fsS -H "Authorization: Bearer $token_a" http://127.0.0.1:8081/api/v1/users/me | jq -e --arg email "$email_a" '.email == $email' >/dev/null || fail "Account A /users/me failed"
curl -fsS -H "Authorization: Bearer $token_b" http://127.0.0.1:8081/api/v1/users/me | jq -e --arg email "$email_b" '.email == $email' >/dev/null || fail "Account B /users/me failed"
pass "Both accounts logged in and bearer tokens authenticated"

lookup_b="$(curl -fsS -G --data-urlencode "email=$email_b" -H "Authorization: Bearer $token_a" http://127.0.0.1:8081/api/v1/users/lookup)" || fail "A could not look up B by exact email"
[[ "$(jq -er '.id' <<<"$lookup_b")" == "$id_b" ]] || fail "Recipient lookup returned wrong user"
pass "Authenticated exact-email recipient lookup works"

send_ab="$(curl -sS -w '\n%{http_code}' -H 'Content-Type: application/json' -H "Authorization: Bearer $token_a" -d "$(jq -nc --arg subject "$subject_ab" --arg body "$body_ab" --argjson id "$id_b" '{subject:$subject,body:$body,recipients:[{user_id:$id,type:"TO"}]}')" http://127.0.0.1:8081/api/v1/messages)"
status="${send_ab##*$'\n'}"; body="${send_ab%$'\n'*}"
[[ "$status" == "201" ]] || fail "A -> B send returned HTTP $status: $body"
id_ab="$(jq -er '.id' <<<"$body")"
pass "A sent message to B (message id=$id_ab)"

inbox_b="$(curl -fsS -H "Authorization: Bearer $token_b" http://127.0.0.1:8081/api/v1/messages)" || fail "B inbox/list request failed after A -> B send"
jq -e --argjson id "$id_ab" --arg subject "$subject_ab" --arg body "$body_ab" 'any(.[]; .id == $id and .subject == $subject and .body == $body)' <<<"$inbox_b" >/dev/null || fail "B cannot see A -> B message in GET /messages: $inbox_b"
pass "B sees A -> B message and body in refreshed message list"

sent_a="$(curl -fsS -H "Authorization: Bearer $token_a" http://127.0.0.1:8081/api/v1/messages)" || fail "A sent/list request failed after sending"
jq -e --argjson id "$id_ab" 'any(.[]; .id == $id)' <<<"$sent_a" >/dev/null || fail "A cannot see sent message in GET /messages"
pass "A sees the sent message after refetch"

send_ba="$(curl -sS -w '\n%{http_code}' -H 'Content-Type: application/json' -H "Authorization: Bearer $token_b" -d "$(jq -nc --arg subject "$subject_ba" --arg body "$body_ba" --argjson id "$id_a" '{subject:$subject,body:$body,recipients:[{user_id:$id,type:"TO"}]}')" http://127.0.0.1:8081/api/v1/messages)"
status="${send_ba##*$'\n'}"; body="${send_ba%$'\n'*}"
[[ "$status" == "201" ]] || fail "B -> A send returned HTTP $status: $body"
id_ba="$(jq -er '.id' <<<"$body")"
pass "B replied to A (message id=$id_ba)"

inbox_a="$(curl -fsS -H "Authorization: Bearer $token_a" http://127.0.0.1:8081/api/v1/messages)" || fail "A inbox/list request failed after B -> A send"
jq -e --argjson id "$id_ba" --arg subject "$subject_ba" --arg body "$body_ba" 'any(.[]; .id == $id and .subject == $subject and .body == $body)' <<<"$inbox_a" >/dev/null || fail "A cannot see B -> A message: $inbox_a"
pass "A sees B's reply and body after refetch"

detail="$(curl -fsS -H "Authorization: Bearer $token_b" "http://127.0.0.1:8081/api/v1/messages/$id_ab")" || fail "B cannot fetch message detail"
jq -e --arg subject "$subject_ab" '.subject == $subject' <<<"$detail" >/dev/null || fail "Message detail does not match sent message"
pass "Recipient can open message detail"

unauthorized="$(curl -sS -o "$LOG_DIR/unauthorized.json" -w '%{http_code}' -H 'Authorization: Bearer invalid-token' http://127.0.0.1:8081/api/v1/messages)"
[[ "$unauthorized" == "401" ]] || fail "Invalid token expected HTTP 401, got $unauthorized"
pass "Invalid token is rejected with HTTP 401"

echo "ALL CURL END-TO-END CHECKS PASSED"

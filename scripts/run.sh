#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

DUDU="${DUDU:-/home/vega/.local/bin/dudu}"
HOST="${HOST:-127.0.0.1}"
PORT_WAS_SET="${PORT+x}"

port_is_free() {
  local port="$1"
  ! ss -ltn "sport = :$port" | grep -q LISTEN
}

if [[ -z "$PORT_WAS_SET" ]]; then
  for candidate in $(seq 8080 8120); do
    if port_is_free "$candidate"; then
      PORT="$candidate"
      export PORT
      break
    fi
  done
fi

if [[ -z "${PORT:-}" ]]; then
  printf '[run] no free port found from 8080 to 8120\n' >&2
  exit 1
fi

printf '[run] starting dudu-webserver on http://%s:%s\n' "$HOST" "$PORT"
exec env HOST="$HOST" PORT="$PORT" "$DUDU" run

#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

DUDU="${DUDU:-/home/vega/.local/bin/dudu}"
exec "$DUDU" build

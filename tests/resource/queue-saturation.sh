#!/usr/bin/env bash
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
command -v curl >/dev/null 2>&1 || { echo 'curl is required for this manual acceptance test' >&2; exit 2; }
"$ROOT/bin/synth" run --target dev >/dev/null || exit $?
for _ in $(seq 1 4200); do curl -fsS http://127.0.0.1:9201/api/v1/signal >/dev/null || exit $?; done
health="$(curl -fsS http://127.0.0.1:9201/health)" || exit $?
printf '%s\n' "$health"
printf '%s\n' "$health" | grep -q '"queue_depth":4096' || { echo 'bounded queue contract failed' >&2; exit 1; }

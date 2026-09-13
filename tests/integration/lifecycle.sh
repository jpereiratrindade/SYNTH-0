#!/usr/bin/env bash
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
first="$($ROOT/bin/synth run --target dev)" || exit $?
printf '%s\n' "$first"
second="$($ROOT/bin/synth run --target dev)" || exit $?
printf '%s\n' "$second"
printf '%s\n' "$second" | grep -q '"no_op": true' || { echo 'expected second lifecycle run to be NO_OP' >&2; exit 1; }

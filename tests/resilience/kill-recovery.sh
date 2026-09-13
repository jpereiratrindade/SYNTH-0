#!/usr/bin/env bash
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
"$ROOT/bin/synth" run --target dev || exit $?
"$ROOT/bin/synth" disturb kill pulso || exit $?
sleep 0.2
"$ROOT/bin/synth" status || exit $?
"$ROOT/bin/synth" run --target dev

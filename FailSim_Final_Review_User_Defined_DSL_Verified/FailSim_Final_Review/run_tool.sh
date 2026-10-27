#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
command -v gcc >/dev/null 2>&1 || { echo 'GCC is required to build FailSim.' >&2; exit 1; }
echo 'Building FailSim C compiler and simulator...'
gcc -std=c11 -Wall -Wextra -pedantic src/*.c -o failsim
exec python3 web/app.py

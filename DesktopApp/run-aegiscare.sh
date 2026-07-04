#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h}"
CMAKE="$ROOT/.venv/bin/cmake"
NINJA="$ROOT/.venv/bin/ninja"
QT_ROOT="$ROOT/third_party/Qt/6.8.3/macos"

if [[ ! -x "$CMAKE" || ! -x "$NINJA" || ! -d "$QT_ROOT" ]]; then
    print -u2 "Local build tools are missing. See README.md for setup instructions."
    exit 1
fi

"$CMAKE" -S "$ROOT" -B "$ROOT/build" -G Ninja \
    -DCMAKE_MAKE_PROGRAM="$NINJA" \
    -DCMAKE_PREFIX_PATH="$QT_ROOT" -DCMAKE_BUILD_TYPE=Release
"$CMAKE" --build "$ROOT/build" --parallel
exec open "$ROOT/build/AegisCareAI.app"

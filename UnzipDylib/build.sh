#!/bin/bash
# Build UnzipDylib with Theos.
set -euo pipefail
cd "$(dirname "$0")"

: "${THEOS:?THEOS is not set. Example: export THEOS=~/theos}"

make clean || true
make FINALPACKAGE=0

echo
echo "== BUILD COMPLETE =="
find .theos -type f -name '*.dylib' -print

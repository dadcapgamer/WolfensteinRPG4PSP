#!/bin/sh
set -eu

if [ -z "${PSPDEV:-}" ]; then
    echo "PSPDEV is not set" >&2
    exit 1
fi

if [ ! -x "$PSPDEV/bin/psp-cmake" ]; then
    echo "Cannot find $PSPDEV/bin/psp-cmake" >&2
    exit 1
fi

PATH="$PSPDEV/bin:$PATH"
export PATH

psp-cmake -S . -B build-psp -DCMAKE_BUILD_TYPE=Release
cmake --build build-psp --parallel

echo "Built build-psp/src/EBOOT.PBP"

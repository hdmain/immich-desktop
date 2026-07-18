#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
build_directory="${BUILD_DIR:-$project_root/build-linux}"

if [[ "${1:-}" != "--no-build" ]]; then
    if ! command -v cmake >/dev/null 2>&1; then
        echo "CMake is required to build immich." >&2
        exit 1
    fi

    configure_args=(
        -S "$project_root"
        -B "$build_directory"
        -DCMAKE_BUILD_TYPE=Release
    )

    if [[ ! -f "$build_directory/CMakeCache.txt" ]] &&
       command -v ninja >/dev/null 2>&1; then
        configure_args+=(-G Ninja)
    fi

    echo "Configuring immich for Linux..."
    cmake "${configure_args[@]}"

    echo "Building immich..."
    cmake --build "$build_directory" --parallel
fi

executable="$build_directory/immich"
if [[ ! -x "$executable" ]]; then
    echo "immich was not found. Run this script without --no-build first." >&2
    exit 1
fi

echo "Starting immich..."
exec "$executable"

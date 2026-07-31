#!/usr/bin/env sh
set -eu

FFMPEG_VERSION=7.1.1
FFMPEG_SHA256=733984395e0dbbe5c046abda2dc49a5544e7e0e1e2366bba849222ae9e3a03b1
PREFIX=${1:-"$(pwd)/.bridgeengine-deps/ffmpeg"}

case "$(uname -s)" in
Linux|Darwin) ;;
*)
    printf '%s\n' "This script supports Linux and macOS only." >&2
    exit 1
    ;;
esac

for command in cc curl make tar; do
    if ! command -v "$command" >/dev/null 2>&1; then
        printf '%s\n' "Required command is missing: $command" >&2
        exit 1
    fi
done

if command -v nproc >/dev/null 2>&1; then
    JOBS=$(nproc)
else
    JOBS=$(sysctl -n hw.ncpu)
fi

WORK_DIR=$(mktemp -d)
trap 'rm -rf "$WORK_DIR"' EXIT INT TERM
ARCHIVE="$WORK_DIR/ffmpeg-$FFMPEG_VERSION.tar.xz"

curl --fail --location --retry 3 --output "$ARCHIVE" \
    "https://ffmpeg.org/releases/ffmpeg-$FFMPEG_VERSION.tar.xz"

if command -v sha256sum >/dev/null 2>&1; then
    ACTUAL_SHA256=$(sha256sum "$ARCHIVE" | cut -d ' ' -f 1)
else
    ACTUAL_SHA256=$(shasum -a 256 "$ARCHIVE" | cut -d ' ' -f 1)
fi
if [ "$ACTUAL_SHA256" != "$FFMPEG_SHA256" ]; then
    printf '%s\n' "FFmpeg checksum mismatch: expected $FFMPEG_SHA256, got $ACTUAL_SHA256" >&2
    exit 1
fi

tar -xf "$ARCHIVE" -C "$WORK_DIR"
cd "$WORK_DIR/ffmpeg-$FFMPEG_VERSION"
./configure --prefix="$PREFIX" --disable-programs --disable-doc --disable-avdevice \
    --disable-postproc --enable-shared --disable-static
make -j"$JOBS"
make install

printf '%s\n' "FFmpeg is ready. Configure BridgeEngine with:"
printf '%s\n' "PKG_CONFIG_PATH=\"$PREFIX/lib/pkgconfig\${PKG_CONFIG_PATH:+:\$PKG_CONFIG_PATH}\" cmake --preset default"

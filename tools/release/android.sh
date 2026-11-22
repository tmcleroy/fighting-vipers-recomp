#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Tommy McLeroy. Part of Fighting Vipers Recomp: https://github.com/tmcleroy/fighting-vipers-recomp
# Release build for Android (ARM64, Android 9 and later): build/release/<Game>-android.apk, the launcher and the
# game in one app. The game is a library (lib<id>.so) that the app starts in a process of its own; the launcher is
# recomp-common's, as a library too (satrecomp/android/: the app's Java and its Gradle project). Needs the
# translated code (work/<id>/gen, as every release), CMake and Ninja, a JDK 17 and the Android SDK with platform 34,
# build-tools and NDK 27.2 (ANDROID_HOME; Homebrew's android-commandlinetools is found on its own: brew install
# openjdk@17 android-commandlinetools), and Rust for librashader (the Display page's shaders). SDL 3 is built from
# source (its Java half is compiled into the app).
#
#   DISC=<folder> tools/release/android.sh
#       a development build with your own disc image packed in (the folder's .cue and .bin files), to try on your
#       own phone: the app copies it into its folder the first time it opens. Never share that APK.
#
# Signing: with KEYSTORE, KEYSTORE_PASSWORD, KEY_ALIAS and KEY_PASSWORD the APK is signed with that key. They are
# also read from the file SIGNING names (default ~/.config/<id>/android-signing.env, outside the repository), as
# lines of NAME="value". Without either it is signed with the Android debug key, which is fine for sideloading your
# own build; RELEASE=1 refuses that, because a published APK has to carry the same key every time or it won't
# install over the one before.
set -eu
. "$(dirname "$0")/common.sh"
BUILD="${BUILD:-$ROOT/build/release-android}"
STAGED="$BUILD/staged"
SIGNING="${SIGNING:-$HOME/.config/$GAME/android-signing.env}"
if [ -z "${KEYSTORE:-}" ] && [ -f "$SIGNING" ]; then
    . "$SIGNING"
fi
if [ -n "${RELEASE:-}" ] && [ -z "${KEYSTORE:-}" ]; then
    echo "error: a release APK needs the signing key ($SIGNING)" >&2
    exit 1
fi
if [ -n "${RELEASE:-}" ] && [ -n "${DISC:-}" ]; then
    echo "error: a release APK can't have a disc packed in" >&2
    exit 1
fi

if [ -z "${ANDROID_HOME:-}" ]; then
    for candidate in /opt/homebrew/share/android-commandlinetools "$HOME/Library/Android/sdk" "$HOME/Android/Sdk"; do
        [ -d "$candidate/platforms" ] && ANDROID_HOME="$candidate" && break
    done
fi
[ -d "${ANDROID_HOME:-}" ] || { echo "error: no Android SDK found; set ANDROID_HOME" >&2; exit 1; }
export ANDROID_HOME
NDK="${ANDROID_NDK_HOME:-$ANDROID_HOME/ndk/27.2.12479018}"
[ -f "$NDK/build/cmake/android.toolchain.cmake" ] || { echo "error: no NDK 27.2 in $NDK" >&2; exit 1; }
if [ -z "${JAVA_HOME:-}" ] && [ -d /opt/homebrew/opt/openjdk@17 ]; then
    export JAVA_HOME=/opt/homebrew/opt/openjdk@17
fi
STRIP=$(ls -d "$NDK"/toolchains/llvm/prebuilt/*/bin/llvm-strip | head -1)
# (and the builder's folders left out of what the compiler writes into the programs)
PREFIX="-ffile-prefix-map=$ROOT=. -ffile-prefix-map=$DEPS=deps"
ANDROID="-DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-28
    -DANDROID_STL=c++_static -Wno-deprecated"

# SDL 3 as a shared library (SDL's Java half loads libSDL3.so by name)
sdl_source
SDL="$DEPS/sdl-android"
if [ ! -f "$SDL/lib/libSDL3.so" ]; then
    cmake -S "$DEPS/SDL3-$SDL_VERSION" -B "$SDL-build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DSDL_SHARED=ON -DSDL_STATIC=OFF \
        -DSDL_TEST_LIBRARY=OFF -DCMAKE_INSTALL_PREFIX="$SDL" $ANDROID > /dev/null 2>&1
    cmake --build "$SDL-build" --target install > /dev/null
fi
# (the NDK's toolchain file warns of CMake versions to come, at length: that is left out of what is shown)
ONLY="-DSDL3_DIR=$SDL/lib/cmake/SDL3"
if ! { build_game "$BUILD/game" $ANDROID $ONLY -DCMAKE_C_FLAGS="$PREFIX" -DCMAKE_CXX_FLAGS="$PREFIX" &&
       build_launcher "$BUILD/launcher" $ANDROID $ONLY -DCMAKE_CXX_FLAGS="$PREFIX" \
           -DGAME_VERSION="$VERSION"; } > "$BUILD.log" 2>&1; then
    grep -v "^CMake Warning\|^  \|^$\|^This warning\|^Call Stack" "$BUILD.log" | tail -40 >&2
    echo "error: the build failed (all of it in $BUILD.log)" >&2
    exit 1
fi

LIBS="$STAGED/jniLibs/arm64-v8a"
rm -rf "$STAGED" && mkdir -p "$LIBS" "$STAGED/assets"
cp "$SDL/lib/libSDL3.so" "$BUILD/game/lib$GAME.so" "$BUILD/launcher/lib$GAME-launcher.so" "$LIBS/"
"$STRIP" --strip-unneeded "$LIBS"/*.so
# (the Display page's shaders: librashader, with OpenGL ES, and the presets, which the game's activity unpacks)
LIBRA=$(shaders "$STAGED/assets" --android "$NDK")
cp "$LIBRA/librashader.so" "$LIBS/"
cp "$ROOT/LICENSE" "$ROOT/THIRD_PARTY_NOTICES.md" "$ROOT/README.md" "$STAGED/assets/"
licenses "$STAGED/assets/licenses" "$BUILD/launcher" "$DEPS/SDL3-$SDL_VERSION/LICENSE.txt"
shader_licenses "$STAGED/assets/licenses" "$LIBRA"
"$ROOT/tools/release/check_archive.sh" "$STAGED"
APK="$PACKAGE-android.apk"
if [ -n "${DISC:-}" ]; then
    # (the launcher's activity copies assets/disc/ into its folder and chooses the disc: LauncherActivity.java)
    mkdir -p "$STAGED/assets/disc"
    for file in "$DISC"/*.cue "$DISC"/*.bin "$DISC"/*.iso; do
        if [ -f "$file" ]; then
            ln -f "$file" "$STAGED/assets/disc/" 2> /dev/null || cp "$file" "$STAGED/assets/disc/"
        fi
    done
    APK="$PACKAGE-android-with-disc-DO-NOT-SHARE.apk"
    echo "packing the disc image in $DISC: a development build, not to be shared"
fi

set -- -PstagedDir="$STAGED" -PsdlDir="$DEPS/SDL3-$SDL_VERSION" -PappId="$($CFG app_id "$GAME")" -PappName="$NAME" \
    -PgameId="$GAME" -PgameVersion="$VERSION"
if [ -n "${KEYSTORE:-}" ]; then
    set -- "$@" -PkeyStore="$KEYSTORE" -PkeyStorePassword="$KEYSTORE_PASSWORD" -PkeyAlias="$KEY_ALIAS" -PkeyPassword="$KEY_PASSWORD"
fi
# (Gradle's own files away from the repository: the project is the toolkit's copy, which is never changed)
GRADLE_PROJECT="$BUILD/gradle"
rm -rf "$GRADLE_PROJECT" && cp -R "$ROOT/satrecomp/android" "$GRADLE_PROJECT"
(cd "$GRADLE_PROJECT" && ./gradlew --console=plain -q assembleRelease "$@")
cp "$GRADLE_PROJECT/app/build/outputs/apk/release/app-release.apk" "$OUT/$APK"
echo "wrote $OUT/$APK ($(du -h "$OUT/$APK" | cut -f1))"

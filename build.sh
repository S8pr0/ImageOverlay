#!/usr/bin/env sh
# Cross-compile ImageOverlay.exe on Linux/macOS with LLVM (clang, lld-link, llvm-rc, llvm-dlltool).
# No Windows SDK is needed: import libraries are generated from imports/*.def.
set -e
cd "$(dirname "$0")"
mkdir -p build

for def in imports/*.def; do
  llvm-dlltool -m i386:x86-64 -d "$def" -l "build/$(basename "$def" .def).lib"
done

llvm-rc -no-preprocess /fo build/app.res res/app.rc

clang --target=x86_64-pc-windows-msvc -O2 -Wall -Wextra -Wno-unused-parameter \
  -ffreestanding -fno-stack-protector -mno-stack-arg-probe \
  -c src/overlay.c -o build/overlay.obj

lld-link /nologo /out:build/ImageOverlay.exe /subsystem:windows,6.0 /entry:WinMainCRTStartup \
  /nodefaultlib /machine:x64 /dynamicbase /nxcompat /highentropyva \
  build/overlay.obj build/app.res build/*.lib

echo "Done: build/ImageOverlay.exe"

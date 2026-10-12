@echo off
rem Build kirby_bezel.dll using TinyCC or MinGW-w64
if exist "..\overlay_toolchain\tcc\tcc.exe" (
    "..\overlay_toolchain\tcc\tcc.exe" -shared -o kirby_bezel.dll kirby_bezel.c
    echo Built kirby_bezel.dll with TinyCC
) else (
    clang -shared -O2 -s -o kirby_bezel.dll kirby_bezel.c
    echo Built kirby_bezel.dll with Clang
)

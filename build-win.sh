#!/usr/bin/sh

CFLAGS="-Wall -O3"
CLIBS="-lraylib -lm -lwinmm -lgdi32"

mkdir -p ./build

x86_64-w64-mingw32-gcc $CFLAGS src/main.c -o ./build/snakegame.exe -L ./src/raylib-5.0_win64_mingw-w64/lib/ -I ./src/raylib-5.0_win64_mingw-w64/include/ $CLIBS
#!/bin/bash

if [ "$1" == "-d" ]; then
	DEBUGFLAGS="-O0 -g -rdynamic"
fi

OUT="cultivation"
SRC="src/main.c"
CFLAGS="-Wall -std=c2x"
LDFLAGS="-lm -ldl"

cc -o $OUT $CFLAGS $LDFLAGS $DEBUGFLAGS $SRC $(pkg-config --cflags --libs sdl2 SDL2_ttf SDL2_image)

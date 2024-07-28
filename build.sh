#!/bin/bash

if [ "$1" == "-d" ]; then
	DEBUGFLAGS="-O0 -g -rdynamic"
fi

OUT="cultivation"
SRC="src/main.c"
INCLUDE_DIRS="-I src"
CFLAGS="-w -std=c2x"
LDFLAGS="-lm -ldl"

cc -o $OUT $CFLAGS $DEBUGFLAGS $INCLUDE_DIRS $SRC $(pkg-config --cflags --libs sdl2 SDL2_ttf SDL2_image) $LDFLAGS

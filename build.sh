#!/bin/bash
set -x
RAYLIB_PATH="raylib/raylib-5.0/src"
CFLAGS="-Wall -Wextra -ggdb -I${RAYLIB_PATH}"

gcc $CFLAGS graph.c -o graphgui -L${RAYLIB_PATH} -lraylib -lm


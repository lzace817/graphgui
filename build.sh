#!/bin/bash
set -x
RAYLIB_PATH="raylib/raylib-5.0/src"
RAYGUI_PATH="raylib/raylib-5.0/examples/shapes/"
CFLAGS="-Wall -Wextra -ggdb -I${RAYLIB_PATH} -I${RAYGUI_PATH}"

gcc $CFLAGS graph.c -o graphgui -L${RAYLIB_PATH} -lraylib -lm


# ============================================================
set +x
date
echo -e "\033[1;32mDONE!\033[0m"
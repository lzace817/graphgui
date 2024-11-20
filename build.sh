#!/bin/bash
set -x
RAYLIB_PATH="raylib/raylib-5.0/src"
RAYGUI_PATH="raylib/raylib-5.0/examples/shapes/"
CFLAGS="-Wall -Wextra -Wstrict-prototypes -ggdb -I${RAYLIB_PATH} -I${RAYGUI_PATH}"
CFLAGS="${CFLAGS} -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable" # NOTE(proto): comment to look for unsused


gcc $CFLAGS graph.c -o graphgui -L${RAYLIB_PATH} -lraylib -lm

# ============================================================
set +x
date
echo -e "\033[1;32mDONE!\033[0m"
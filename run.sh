#!/usr/bin/env sh

OS_NAME=$(uname -s)

case "$OS_NAME" in
    Linux*)
        echo "Operating System: Linux"
		gcc main.c -o main -lSDL2 -lSDL2_image && ./main
        ;;
    Darwin*)
        echo "Operating System: macOS (Darwin)"
		gcc main.c -o main -I/opt/homebrew/include/SDL2 -D_THREAD_SAFE -L/opt/homebrew/lib -lSDL2 -lSDL2_image && ./main
        ;;
    *)
        echo "Operating System: UNKNOWN: $OS_NAME"
        exit 1
        ;;
esac


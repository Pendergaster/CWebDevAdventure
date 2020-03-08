#! /bin/bash

clear

gcc \
    canvas.c src/glad.c \
    -g \
    -Wall -Wextra\
    -Wno-unused-function \
    -Wno-unused-variable \
    -lglfw -lm -ldl \
    -o canvas.out\

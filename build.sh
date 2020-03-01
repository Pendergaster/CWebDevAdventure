#! /bin/bash

clear

gcc \
    main.c \
    -g \
    -Wall -Wextra\
    -o server.out\
    -ltcc -ldl

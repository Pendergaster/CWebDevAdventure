#! /bin/bash

clear

gcc \
    main.c \
    -g \
    -Wall -Wextra\
    -Wno-unused-function \
    -ltcc -ldl -lpthread -lssl -lcrypto \
    -o server.out\

#! /bin/bash

clear

gcc \
    main.c \
    -g \
    -Wall -Wextra\
    -ltcc -ldl -lpthread \
    -o server.out\
    

#!/usr/bin/env bash

gcc -Wall -Werror -std=gnu11 -O0 -g3 -fsanitize=address  -lm main.c -o main
#gcc -Wall -Werror -std=gnu11 -O0 -g3  -lm main.c -o main


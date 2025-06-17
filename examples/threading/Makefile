#! /bin/bash
# Makefile for threading example
# This Makefile compiles a simple threading example using GCC
# It includes rules for building the executable and cleaning up object files

CC=gcc
CFLAGS=-Wall -Werror -pthread
OBJ=main.o threading.o

all: threading

threading: $(OBJ)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJ) threading
#!/bin/sh
cc main.c `pkg-config --libs --cflags raylib` -o game
#!/bin/bash

g++ sample-basic.cpp -L/usr/X11R6/lib `pkg-config --libs --cflags glsk` -lGL -lXxf86vm -o glsk-test

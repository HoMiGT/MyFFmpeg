#!/bin/bash

EXECUTABLE=./lib/MyFFmpeg.cpython-310-x86_64-linux-gnu.so
INSTALL_DIR=./lib/

for LIB in $(ldd $EXECUTABLE | awk '{print $3}'); do 
    if [ -f $LIB ]; then
        cp -v $LIB $INSTALL_DIR
    fi
done

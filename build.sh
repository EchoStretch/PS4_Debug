#!/bin/bash

# Function to first enter directory, perform cleanup, 
# before building fresh copy of target
build_target() {
    echo "Building $1"
    cd "$1" || exit 1
    make clean && make
    cd ..
    echo "Finished..."
}

# Begin to build each part
build_target "kernSDK"
build_target "libPS4"
build_target "debugger"
build_target "kdebugger"
build_target "installer"

# Copy the compiled binary file to root folder
# After renaming it to ps4debug 
cp ./installer/installer.bin ./PS4Debug.bin
echo "Finished!"
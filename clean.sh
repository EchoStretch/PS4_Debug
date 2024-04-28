#!/bin/bash

clean_target() {
    echo "Performing cleanup on $1"
    cd "$1" || exit 1
    make clean
    cd ..
    echo "Finished..."
}

# Begin to build each part
clean_target "ps4-ksdk"
clean_target "ps4-payload-sdk"
clean_target "installer"
clean_target "debugger"
clean_target "kdebugger"


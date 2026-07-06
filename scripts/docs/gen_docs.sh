#!/bin/bash
sudo apt install -y doxygen
mkdir -p ../../../heaven_tmp
mkdir -p ../../../heaven_tmp/html
cd ..
chmod +x env.sh
source env.sh
cd ../doxygen
doxygen Doxyfile
cd ..


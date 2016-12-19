#!/bin/bash

# install updated version of cmake
apt-get install --assume-yes software-properties-common
add-apt-repository -y ppa:george-edison55/cmake-3.x
apt-get update --assume-yes
apt-get install --assume-yes cmake
cmake --version  # This allows us to confirm which version of cmake has been installed.

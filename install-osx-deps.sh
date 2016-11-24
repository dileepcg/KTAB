#!/bin/bash

# always a good idea to update prior to installing anything.
brew update

# install updated version of cmake
brew install cmake
cmake --version  # This allows us to confirm which version of cmake has been installed.

# install additional dependencies
brew tap linuxbrew/homebrew-xorg && brew install libxaw
brew install fltk

#install Qt5 libs
brew tap homebrew/versions
brew install qt55

#symlink the bundles
brew linkapps qt55


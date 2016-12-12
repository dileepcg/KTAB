#!/bin/bash
#
# You must generate a 'build' directory for each subproject
# before using this script. CMake is the recommended was to do so.
#
#------------------------------------------
FILES="kutils  kmodel"

for d in $FILES
do
pushd $d/build ; nice make clean ; sleep 1; popd
done
#------------------------------------------

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#!/bin/bash

set -e

pushd KTAB/ ; ./reconfigKTAB.sh ; popd
pushd examples/ ; ./reconfigEX.sh ; popd

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

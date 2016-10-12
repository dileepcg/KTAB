# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Copyright KAPSARC. MIT Open Source License.
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# see http://zxq9.com/archives/795 for some date formats
#!/bin/bash
#
# usage: monte-carlo-runs.sh <n>
# -------------------------------------------

START=1
END=10

rm -f run-smpc*txt  run-smpc*db  test.db
for i in $(eval echo "{$START..$END}")
do
  # Zulu (UTC) time, with nanoseconds
  timestamp="$(date --utc +%Y-%m-%d_%H-%M-%S-%NZ)"
  name="run-smpc-$timestamp"
  echo "$i: $name" 
  nice ../smpc --seed 0 --euSMP > $name.txt ; mv testsmp.db $name.db
done


echo "Problem sizes"
grep "Number of actors" run-smpc*.txt
grep "Number of SMP dimensions" run-smpc*.txt


sNum=$(grep "Start time" run-smpc*.txt | wc -l)
cNum=$(grep "Elapsed time" run-smpc*.txt | wc -l)

echo "Number of runs started   $sNum"
echo "Number of runs completed $cNum"

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Copyright KAPSARC. MIT Open Source License.
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


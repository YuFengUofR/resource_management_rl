#!/bin/sh

dt=1
round=1
while [ ${dt} -le 16 ]
do
  echo "=== Test Policy with DTime " ${dt} "s ==="
  echo "## PERFORMANCE ##"
  ./linux_policy_test ${round} ${dt} performance  | grep IMPORTANT_RESULT
  echo "## POWERSAVE ##"
  ./linux_policy_test ${round} ${dt} powersave    | grep IMPORTANT_RESULT
  echo "## ONDEMAND ##"
  ./linux_policy_test ${round} ${dt} ondemand     | grep IMPORTANT_RESULT
  echo "## CONSERVATIVE ##"
  ./linux_policy_test ${round} ${dt} conservative | grep IMPORTANT_RESULT
  dt=$(( $dt*2 ))
done

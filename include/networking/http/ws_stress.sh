#!/bin/bash

if [[ -z "$1" ]]; then
  echo "requires \"[host]:[port]\" as first argument"
  exit -1
fi

i=1

echo -n "How many iterations? "
read iter

time (while [[ i -le $iter ]]; do
  echo -n $i | websocat -1 ws://$1
  ((i++))
done && echo -en "\ntime taken:")

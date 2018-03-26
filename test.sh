#!/bin/bash

pwd=$PWD
tmp="$(mktemp -d /tmp/plot.XXX)"
trap "rm -rf $tmp" EXIT
cd "$tmp"

fails=0

# with SSE2
$pwd/plot -k 123 -s 0 -n 20
sum="$(md5sum 123_0_20_20 | cut -f1 -d' ')"
expected=1993aa905f27e9aaea76ff83b7a2ba4a

pred="$sum == $expected"
if eval test $pred; then
    echo -e "\e[32mpass: $pred"
else
    echo -e "\e[31mfail: $pred"
    let fails++
fi
echo -e "\e[0m"

# without SSE2
$pwd/plot -k 123 -s 0 -n 20 -x 0
sum="$(md5sum 123_0_20_20 | cut -f1 -d' ')"
expected=1993aa905f27e9aaea76ff83b7a2ba4a

pred="$sum == $expected"
if eval test $pred; then
    echo -e "\e[32mpass: $pred"; exit 0
else
    echo -e "\e[31mfail: $pred"; exit 1
    let fails++
fi
echo -en "\e[0m"

return $fails

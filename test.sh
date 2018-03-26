#!/bin/bash

pwd=$PWD
tmp="$(mktemp -d /tmp/plot.XXX)"
trap "rm -rf $tmp" EXIT
cd "$tmp"

fails=0

# with SSE2
$pwd/plot -k 123 -s 0 -n 10
sum="$(md5sum 123_0_10_10 | cut -f1 -d' ')"
expected=b4be195887ed7c88f9c171a38502fd61

pred="$sum == $expected"
if eval test $pred; then
    echo -e "\e[32mpass: $pred"
else
    echo -e "\e[31mfail: $pred"
    let fails++
fi
echo -e "\e[0m"

# without SSE2
$pwd/plot -k 123 -s 0 -n 10 -x 0
sum="$(md5sum 123_0_10_10 | cut -f1 -d' ')"
expected=b4be195887ed7c88f9c171a38502fd61

pred="$sum == $expected"
if eval test $pred; then
    echo -e "\e[32mpass: $pred"; exit 0
else
    echo -e "\e[31mfail: $pred"; exit 1
    let fails++
fi
echo -en "\e[0m"

return $fails

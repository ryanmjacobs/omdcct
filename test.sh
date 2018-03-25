#!/bin/bash

pwd=$PWD
tmp="$(mktemp -d /tmp/plot.XXX)"
trap "rm -rf $tmp" EXIT
cd "$tmp"

$pwd/plot -k 123 -s 0 -n 10
sum="$(md5sum 123_0_10_10 | cut -f1 -d' ')"
expected=b4be195887ed7c88f9c171a38502fd61

pred="$sum == $expected"
if eval test $pred; then
    echo "pass: $pred"; exit 0
else
    echo "fail: $pred"; exit 1
fi

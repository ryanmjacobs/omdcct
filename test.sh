#!/bin/bash

pwd=$PWD
tmp="$(mktemp -d /tmp/plot.XXX)"
trap "rm -rf $tmp" EXIT
cd "$tmp"

$pwd/plot -k 123 -s 0 -n 100
sum="$(md5sum 123_0_100_100 | cut -f1 -d' ')"
expected=e949c49f889d57499fd7a1aa579dd7d1

pred="$sum == $expected"
if eval test $pred; then
    echo "pass: $pred"; exit 0
else
    echo "fail: $pred"; exit 1
fi

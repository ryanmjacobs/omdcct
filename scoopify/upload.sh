#!/bin/bash

fname="$1"

tmp="$(mktemp /tmp/scoopify.XXX)"
trap "rm -v $tmp" EXIT

gdrive upload -f "$fname" | tee "$tmp"
id="$(grep "Id" "$tmp" | cut -d' ' -f 2)"

echo "$id,$fname" | tee -a scoopify.log

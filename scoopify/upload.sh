#!/bin/bash

fname="$1"

log="$(mktemp /tmp/log.XXX)"
trap "rm -v $log" EXIT

gdrive upload -f "$fname" | tee "$log"
id="$(grep "Id" "$log" | cut -d' ' -f 2)"

echo "$id,$fname" | tee -a scoopify.log

#!/bin/bash

fname="$1"
folder_id=`cat folder_id.txt`

log="$(mktemp /tmp/log.XXX)"
trap "rm -v $log" EXIT

gdrive upload -p "$folder_id" -f "$fname" | tee "$log"
id="$(grep "Id" "$log" | cut -d' ' -f 2)"

echo "$id,$fname" | tee -a scoopify.log

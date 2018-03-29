#!/bin/bash
set -e

readarray lines < plot.sh.log

for l in "${lines[@]}"; do
    id="$(cut -f1 -d, <<< "$l")"
    gdrive download -i "$id"
done

./scoopify default

for l in "${lines[@]}"; do
    gdrive delete -i "$id"
done

#!/bin/bash
set -e

parent="1bkNsSBPYWzBC_81uFlJrOc4pBVpFDohy"

file="$1"
link="$2"

# append if we have previous data
if [ "$link" != "null" ]; then
    gdrive download -i "$link" -s >> "$file"
fi

# upload to drive
log="$(mktemp)"
pv -rbpe "$file" | gdrive upload -p "$parent" -s -t "$(basename "$file")" > "$log"
grep ^Id "$log" | cut -d' ' -f2
rm -f "$log"

# delete old file
if [ "$link" != "null" ]; then
    gdrive delete -i "$link" &>/dev/null
fi
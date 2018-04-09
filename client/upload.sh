#!/bin/bash
set -e

parent="1bkNsSBPYWzBC_81uFlJrOc4pBVpFDohy"

file="$1"
link="$2"

echo "file=$file, link=$link"

# append if we have previous data
if [ "$link" != "null" ]; then
    gdrive download -i "$link" -s >> "$file"
fi

# upload to drive
pv -rbpe "$file" | gdrive upload -p "$parent" -s -t "$file"

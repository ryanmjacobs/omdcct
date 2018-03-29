
#!/bin/bash

readarray lines < plot.sh.log

for l in "${lines[@]}"; do
    id="$(cut -f1 -d, <<< "$l")"
    gdrive download -i "$id"
done

./scoopify default

for l in "${lines[@]}"; do
    id="$(cut -f1 -d, <<< "$l")"
    fname="$(cut -f2 -d, <<< "$l")"

    gdrive delete -i "$id"
    rm -v "$fname"
done

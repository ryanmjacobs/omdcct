#!/bin/bash
set -e

stage() {
    mkdir -p staging
    
    for tarf in "${scoop_tars[@]}"; do
        mkdir -p scoops.tmp
    
        echo "$tarf"
        tar xf "$tarf" -C scoops.tmp
    
        for f in scoops.tmp/scoop_*; do
            scoop="$(cut -d_ -f2 <<< "$f")"

            [ "$(($scoop % 100))" -eq 0 ] &&
                printf "\r  $scoop  "

            cat "$f" >> staging/"$(basename "$f")"
            rm "$f"
        done
        echo
    done
}

upload() {
    while true; do
        # upload as many scoops as we can
        plotdir=staging
        iter=-1 # special flag for us
        node upload.js "$plotdir" "$iter"

        # exit when we have nothing left to upload
        ls "$plotdir"/scoop_ &>/dev/null || break

        sleep 5
    done
}

cleanup() {
    rm -rf staging
    rm -rf scoops.tmp
}
trap cleanup EXIT

echo "obtaining lock..."; (
  flock -e 200
  # get a snapshot of which scoops to use,
  # so that we can safely delete them later
  scoop_tars=(scoops.*.tar)

  tarsize="$(du -cb "${scoop_tars[@]}" | tail -n1 | cut -f1)"
 #if [ "$tarsize" -gt "$((50*1024*1024*1024))" ]; then
  if [ "$tarsize" -gt "$((50*1024*1024))" ]; then
      time stage
      time upload
      rm -v "${scoop_tars[@]}"
  fi
) 200>upload_tarballs.lockfile 

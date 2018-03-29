#!/bin/bash

failures=0
plotdir="$(mktemp -d /dev/shm/plotdir.XXX)"

# plot directory setup
mkdir -p "$plotdir"
if [ ! -d "$plotdir" ] || [ ! -O "$plotdir" ]; then
    >&2 echo "error: could not open directory '$plotdir' for writing"
    exit 1
fi
cleanup() {
   #rm -rf "$plotdir" "$log"

    [ "$failures" -ne 0 ] && curl -d "pid=$pid" -X POST http://ucla.red.rmj.us:3745/fail
}
trap cleanup EXIT

# find the correct plot program, if it fails, use our self-compiled one
# (glib issues)
plot=plot
"$plot" --help &>/dev/null
if [ "$?" -ne 255 ] && [ "$?" -ne 0 ]; then
    plot="$(mktemp $plotdir/plot.XXX)"
    cp -r ~/omdcct "$plotdir"
    pushd "$plotdir"/omdcct

    make clean plot
    cp -v plot "$plot"
    chmod +x "$plot"

    popd
fi

# self-compile pv if necessary
compile_pv() {
    let failures++
    set -e
    pvdir="$(mktemp -d $plotdir/pv.XXX)"

   #git clone --depth=1 https://github.com/icetee/pv $pvdir
    cp -r pv_src/* "$pvdir"
    cd "$pvdir"

    mkdir usr
    ./configure --prefix="$PWD/usr"
    make -j8
    make install

    shopt -s expand_aliases
    alias pv="$pvdir/usr/bin/pv"
    set +e
    let failures--
}
pv --help &>/dev/null || compile_pv

# get plotting parameters
parameters=`curl http://ucla.red.rmj.us:3745/next`
   pid="$(echo $parameters | cut -d, -f1)"
snonce="$(echo $parameters | cut -d, -f2)"
nonces="$(echo $parameters | cut -d, -f3)"
echo "got plotting parameters: $parameters"

# run plot
time nice -n10\
    $plot -k 5801048965275211042 -x 1 -d $plotdir -t`nproc`\
        -s "$snonce"\
        -n "$nonces"\
        -m "$nonces"\

# grab file
f="$plotdir/5801048965275211042_${snonce}_${nonces}_${nonces}"
if [ ! -f "$f" ]; then
    >&2 echo "error: unable to read plotfile"
    let failures++
    exit 1
fi

bn="$(basename "$f")"
log="$(mktemp /tmp/log.XXX)"

# upload it
time pv -rbpe "$f" |\
    gdrive upload -p "1TCGD4Cw5liGG1TnPfB2CQzCNPNVkYgj7"\
      -s -t "$bn" | tee $log

if [ "$?" -ne 0 ]; then
    let failures++
    exit 1
fi

# let the orchestrator know
gid="$(grep "Id" $log | cut -d' ' -f 2)"
curl -d "pid=$pid&google_drive_id=$gid" -X POST http://ucla.red.rmj.us:3745/complete

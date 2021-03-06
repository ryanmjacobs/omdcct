#!/bin/bash

# current script path
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# curl workaround, (becauase I messed with glibc versions)
curl() { LD_LIBRARY_PATH= /usr/bin/curl "$@"; }

# check for connection to orchestrator
if [ "$(curl ucla.red.rmj.us:3745/status)" != "orchestrator" ]; then
    >&2 echo "error: unable to connect to orchestrator"
    exit 1
fi

failures=0
plotdir="$(mktemp -d /dev/shm/plotdir.XXX)"

# plot directory setup
mkdir -p "$plotdir"
if [ ! -d "$plotdir" ] || [ ! -O "$plotdir" ]; then
    >&2 echo "error: could not open directory '$plotdir' for writing"
    exit 1
fi

# cleanup
cleanup() {
    rm -rf "$plotdir" "$log"
    [ "$failures" -ne 0 ] && curl -d "pid=$pid" -X POST http://ucla.red.rmj.us:3745/fail
}
trap cleanup EXIT

# find the correct plot program, if it fails, use our self-compiled one
# (glib issues)
compile_plot() {
    plot="$(mktemp $plotdir/plot.bin.XXX)"
    cp -r "$DIR/plot" "$plotdir"/omdcct
    pushd "$plotdir"/omdcct

    make clean plot
    cp -v plot "$plot"
    chmod +x "$plot"

    popd
}
compile_plot

# self-compile pv if necessary
compile_pv() {
    let failures++
    set -e
    pvdir="$(mktemp -d $plotdir/pv.XXX)"

   #git clone --depth=1 https://github.com/icetee/pv $pvdir
    cp -r ~/pv_src/* "$pvdir"
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
pushd "$plotdir"
time nice -n10\
    "$plot" -k 5801048965275211042 -t `nproc`\
        -s "$snonce"\
        -n "$nonces"\
        -m "$nonces"
popd

if [ "$?" -ne 0 ]; then
    let failures++
    exit 1
fi

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
    gdrive upload -p "1_EyTA09Jl4a3AyjuYjHEiqMMHCESe9g0"\
      -s -t "$bn" | tee $log

if [ "$?" -ne 0 ]; then
    let failures++
    exit 1
fi

# let the orchestrator know
gid="$(grep "Id" $log | cut -d' ' -f 2)"
curl -d "pid=$pid&google_drive_id=$gid" -X POST http://ucla.red.rmj.us:3745/complete

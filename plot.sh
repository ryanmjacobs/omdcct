#!/bin/bash

plotdir=/dev/shm/plot
snonce="$1"
nonces="$2"

# plot directory setup
mkdir -p $plotdir
if [ ! -d $plotdir ] || [ ! -O $plotdir ]; then
    >&2 echo "error: could not open directory $plotdir for writing"
    exit 1
fi
trap "rm -rf $plotdir $log" EXIT

# find the correct plot program, if it fails, use our self-compiled one
# (glib issues)
plot=plot
if ! $plot --help &>/dev/null; then
    plot="$(mktemp $plotdir/plot.XXX)"
    cp -r ~/omdcct $plotdir
    pushd $plotdir/omdcct

    make clean plot
    cp -v plot "$plot"
    chmod +x "$plot"

    popd
fi

# self-compile pv if necessary
compile_pv() {
    set -e
    pvdir=$(mktemp -d $plotdir/pv.XXX)

   #git clone --depth=1 https://github.com/icetee/pv $pvdir
    cp -r pv_src/* $pvdir
    cd $pvdir

    mkdir usr
    ./configure --prefix=$PWD/usr
    make -j8
    make install

    shopt -s expand_aliases
    alias pv=$pvdir/usr/bin/pv
    set +e
}
pv --help &>/dev/null || compile_pv

# get plotting parameters
parameters=`curl localhost:3745`
   pid="$(echo $parameters | cut -d, -f1)"
snonce="$(echo $parameters | cut -d, -f2)"
nonces="$(echo $parameters | cut -d, -f3)"

# run plot
time nice -n10\
    $plot -k 5801048965275211042 -x 1 -d $plotdir -t`nproc`\
        -s "$snonce"\
        -n "$nonces"

# grab file
f="$plotdir/5801048965275211042_$snonce_$nonces_*"
if [ ! -f "$f" ]; then
    >&2 echo "error: unable to read plotfile"
    curl -d "pid=$pid" -X POST localhost:3745/fail
    exit 1
fi

bn="$(basename "$f")"
log="$(mktemp /tmp/log.XXX)"

# upload it
time pv -rbpe "$f" |\
    gdrive upload -p "1TCGD4Cw5liGG1TnPfB2CQzCNPNVkYgj7"\
      -s -t "$bn" | tee $log

# let the orchestrator know
gid="$(grep "Id" $log | cut -d' ' -f 2)"
curl -d "pid=$pid&google_drive_id=$gid" -X POST localhost:3745/complete

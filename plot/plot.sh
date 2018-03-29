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
trap "rm -rf $plotdir" EXIT

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

time nice -n10\
    $plot -k 5801048965275211042 -x 1 -d $plotdir -t`nproc`\
        -s "$snonce"\
        -n "$nonces"

for f in $plotdir/580*; do
    [ ! -f "$f" ] && break
    echo "$f"

    log="$(mktemp /tmp/log.XXX)"
    bn="$(basename "$f")"

    time pv -rbpe "$f" |\
        gdrive upload -p "1TCGD4Cw5liGG1TnPfB2CQzCNPNVkYgj7"\
          -s -t "$bn" | tee $log

    id="$(grep "Id" $log | cut -d' ' -f 2)"
    echo "$id,$bn" | tee -a ~/plot.sh.log

    rm -v "$log"
done

gdrive upload -p "1TCGD4Cw5liGG1TnPfB2CQzCNPNVkYgj7" -f ~/plot.sh.log

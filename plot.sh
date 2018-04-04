#!/bin/bash
LD_LIBRARY_PATH=
GDRIVE_PARENT="1fSGVpnRxZgIU6ZSl42NIRQps1_8VqoYi"
ORCHESTRATOR=http://localhost:3745

curl() { `which curl` -s "$@"; }

# current script path
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# check for connection to orchestrator
if [ "$(curl $ORCHESTRATOR/health-check)" != "OK" ]; then
    >&2 echo "error: unable to connect to orchestrator"
    exit 1
fi

# ensure the predicate ($@) is true,
# otherwise increment failures and exit
failures=0
ensure() {
    if ! test "$@"; then
        let failures++
        exit 1
    fi
}

# cleanup
cleanup() {
    rm -rf "$plotdir" "$log"
    [ "$failures" -ne 0 ] && curl -d "iter=$iter" -X POST $ORCHESTRATOR/fail
}
trap cleanup EXIT

# plot directory setup
plotdir="$(mktemp -d /dev/shm/plotdir.XXX)"
mkdir -p "$plotdir"
if [ ! -d "$plotdir" ] || [ ! -O "$plotdir" ]; then
    >&2 echo "error: could not open directory '$plotdir' for writing"
    exit 1
fi

# self compile our plot program
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
parameters=`curl $ORCHESTRATOR/next`
  iter="$(echo $parameters | cut -d, -f1)"
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
ensure $? -eq 0

####
# upload the scoops
pushd "$plotdir"

# create sparse array of upload status
uploaded=()
for n in {0..4095}; do
    uploaded[$n]=0
done

sum_uploads() {
    sum=0
    for n in {0..4095}; do
        let sum+=${uploaded[n]}
    done
    echo $sum
}

# returns 0 if all uploads have been completed
uploads_complete() {
    sum=`sum_uploads`
    [ $sum -eq 4096 ] && return 0 || return 1
}

# upload a file and echo the ID
gdrive_upload() {
    log="$(mktemp $plotdir/log.XXX)"

    # upload it
    pv -rbpe "$1" | gdrive upload -p "$GDRIVE_PARENT" -s -t "$1" > $log
    ensure "$?" -eq 0

    # let the orchestrator know
    gid="$(grep "Id" $log | cut -d' ' -f 2)"
    echo "$gid"
}

while ! uploads_complete; do
    echo uploads completed: `sum_uploads`

    for n in {0..4095}; do
        echo -e "\n"
        sleep 1

        f="scoop_${n}_5801048965275211042"
        ensure -f "$f"

        gdrive_link=`curl -d "scoop=$n" -X POST $ORCHESTRATOR/lock`
        echo res: $gdrive_link

        if [[ "$gdrive_link" =~ *"already locked"* ]]; then
            # skip locked scoops
            continue
        elif [ "$gdrive_link" != "null" ] && [ "$gdrive_link" != "undefined" ]; then
            # download, append, upload -> unlock
            gdrive download -i "$gdrive_link" -s >> "$f"
        fi

        gid=`gdrive_upload "$f"`
        curl -d "scoop=$n&link=$gid" -X POST $ORCHESTRATOR/unlock
        echo
        gdrive delete -i "$gdrive_link"

        uploads[$n]=1
    done

    sleep 1
done

popd
exit

curl -d "iter=$iter&google_drive_id=$gid" -X POST $ORCHESTRATOR/complete

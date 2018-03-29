#!/bin/bash

sesh="multiplot"

run() {
    tmux send-keys -t $sesh "$1" Enter
}

create() {
    tmux new-window -t $sesh

    run "$1"
    shift

    for cmd in "$@"; do
        tmux split -ht "$sesh"
        run "$cmd"
    done
}

tmux new -d -s $sesh
tmux send-keys -t $sesh "tmux kill-session"

for i in {1..9}; do
    m=lnxsrv0$i
    nonces=4096
    snonce=$(( (i-1) * nonces ))
    create "ssh $m -t time ~/plot.sh $snonce $nonces" "ssh $m -t htop"
done

tmux attach -t $sesh

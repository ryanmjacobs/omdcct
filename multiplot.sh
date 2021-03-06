   #create "ssh $m -t 'while true; do ~/omdcct_ryan/plot.sh || break; done'" "ssh $m -t 'sleep 5; htop'2
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
   #create "ssh $m -t 'while true; do ~/omdcct_ryan/plot.sh || break; done'"\
   #       "sleep 3; ssh $m -t 'sleep 1; htop'"
    create "ssh $m -t ~/omdcct_ryan/plot.sh" "sleep 2; ssh $m -t 'sleep 1; htop'"
done

tmux attach -t $sesh

#!/bin/bash
compile() {
    echo "Compiling all targets..."
    cd /root/c++-multicast/
    /usr/sbin/cmake --build /root/c++-multicast/build --config Release --target all -j 14 --
    echo "All targets are compiled."
}

start() {
    cd /root/c++-multicast/
    /root/c++-multicast/build/statistics-writer-main $2 &
    # while [ -z $(pgrep statistics-wr) ]; do
    #     sleep 1
    # done
    echo "statistics-writer is started"

    for (( i=0; i<$(($2-1)); i++ ))
    do 
        /root/c++-multicast/build/multicast-dh-example false $1 $2 &
    done
    /root/c++-multicast/build/multicast-dh-example true $1 $2 &
    echo "Initial sponsor is started"
    while [[ $(($(pgrep multicast-dh | wc -l)-1)) != $(($2-1)) ]]; do
        echo "Waiting for all subscribers to start up, $(($(pgrep multicast-dh | wc -l)-1))/$(($2-1)) are up"
        sleep 1
    done
    echo "All subscribers started up"
    while [[ -n $(pgrep statistics-wr) ]]; do
        sleep 1
        echo "Waiting for statistics-writer to stop"
    done
    echo "statistics-writer is stopped"
}

function on_exit() {
    echo "Terminating all processes ..."
    pkill "statistics-wr"
    pkill "multicast-dh"
    echo "All processes terminated"
    exit 1
}

if [ $# -ne 2 ]; then
    echo "Not enough parameters" 1>&2
    echo "Usage: $0 <service_id> <member_count>"
    echo "Example: $0 42 20"
    exit 1
fi

trap 'on_exit' SIGINT

compile
start $1 $2
#!/bin/bash
compile() {
    echo "Compiling all targets..."
    cd /root/c++-multicast/
    /usr/sbin/cmake --build /root/c++-multicast/build --config Release --target all -j 14 --
    echo "All targets are compiled."
}

get_process_count() {
    echo $(pgrep multicast-dh | wc -l)
}

get_subscriber_count() {
    echo $(($1-1))
}

start() {
    cd /root/c++-multicast/
    /root/c++-multicast/build/statistics-writer-main $2 &
    echo "statistics-writer is started"

    for (( i=0; i<$(get_subscriber_count $2); i++ ))
    do
        /root/c++-multicast/build/multicast-dh-example false $1 $2 $3 $4 $5 &
    done
    while [[ $(get_process_count) != $(get_subscriber_count $2) ]]; do
        echo "Waiting for all subscribers to start up, $(get_process_count)/$(get_subscriber_count $2) are up"
        sleep 1
    done
    echo "All subscribers started up"
    SLEEP_TIME_FOR_PORT_BOUND_ONCE_CHECK=5
    echo "Sleeping $SLEEP_TIME_FOR_PORT_BOUND_ONCE_CHECK seconds more for port bound once checks"
    sleep 5

    /root/c++-multicast/build/multicast-dh-example true $1 $2 $3 $4 $5 &
    echo "Initial sponsor is started"
    sleep 3

    while [[ -n $(pgrep statistics-wr) ]]; do
        sleep 1
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

if [ $# -ne 5 ]; then
    echo "Not enough parameters" 1>&2
    echo "Usage: $0 <service_id> <member_count> <request_delay_min(ms)> <request_delay_max(ms)> <request_count_target>"
    echo "Example: $0 42 20 10 100 10"
    exit 1
fi

trap 'on_exit' SIGINT

compile
start $1 $2 $3 $4 $5
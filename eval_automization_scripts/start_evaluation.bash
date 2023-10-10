#!/bin/bash
compile() {
    echo "Compiling all targets..."
    cd /root/c++-multicast/
    /usr/sbin/cmake --build /root/c++-multicast/build --config Release --target all -j 14 --
    echo "done."
}

start() {
    cd /root/c++-multicast/
    /root/c++-multicast/build/statistics-writer-main <member_coount> &
    while [ -z $(pgrep statistics-writer-main) ]; do
        sleep 1
    done
    echo "statistics-writer is started"
    /root/c++-multicast/build/multicast-dh-example true <service_id> <member_count>
    for (( i=0; i<<member_count>-1; i++ ))
    do 
        /root/c++-multicast/build/multicast-dh-example false <service_id> <member_count>
    done
    while [ -n $(pgrep statistics-writer-main) ]; do
        sleep 1
    done
    echo "statistics-writer is stopped"
}

function on_exit() {
    echo "Not implemented"
    exit 1
}

if [ $# -ne 2 ]; then
    echo "Please specify a parameter greater than zero for the sample count." 1>&2
    exit 1
fi

trap 'on_exit' SIGINT

compile
start
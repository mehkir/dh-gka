#!/bin/bash
compile() {
    local CRYPTO_ALGORITHM=$1
    local KEY_AGREEMENT_PROTOCOL=$2
    sed -i -E "s/add_compile_definitions\(.*\)/add_compile_definitions($CRYPTO_ALGORITHM $KEY_AGREEMENT_PROTOCOL)/" /root/c++-multicast/CMakeLists.txt
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

get_members_up_count_by_unique_ports() {
    echo $(ss -lup | grep multicast-dh-ex | awk '{print $4}' |  awk -F: '{print $NF}' | grep -v 65000 | sort -u | wc -l)
}

start() {
    local SERVICE_ID=$1
    local MEMBER_COUNT=$2
    local SCATTER_DELAY_MIN=$3
    local SCATTER_DELAY_MAX=$4
    local CRYPTO_ALGORITHM=$5
    local KEY_AGREEMENT_PROTOCOL=$6

    echo "Starting $KEY_AGREEMENT_PROTOCOL $CRYPTO_ALGORITHM with $MEMBER_COUNT members"

    cd /root/c++-multicast/
    /root/c++-multicast/build/statistics-writer-main $MEMBER_COUNT "${KEY_AGREEMENT_PROTOCOL}-${CRYPTO_ALGORITHM}-${MEMBER_COUNT}" &
    while [[ -z $(pgrep statistics-wr) ]]; do
        echo "Waiting for statistics writer to start up"
        sleep 1
    done
    echo "statistics-writer is started"

    for (( i=0; i<$(get_subscriber_count $MEMBER_COUNT); i++ ))
    do
        /root/c++-multicast/build/multicast-dh-example false $SERVICE_ID $MEMBER_COUNT $SCATTER_DELAY_MIN $SCATTER_DELAY_MAX &
    done
    while [[ $(get_process_count) -ne $(get_subscriber_count $MEMBER_COUNT) ]]; do
        echo "Waiting for all subscribers to start up, $(get_process_count)/$(get_subscriber_count $MEMBER_COUNT) are up"
        sleep 1
    done
    echo "All subscribers started up"
    while [[ $(get_members_up_count_by_unique_ports) < $(get_subscriber_count $MEMBER_COUNT) ]]; do
        echo "There are still ports bound more than once"
        sleep 1
    done
    /root/c++-multicast/build/multicast-dh-example true $SERVICE_ID $MEMBER_COUNT $SCATTER_DELAY_MIN $SCATTER_DELAY_MAX &
    while [[ $(get_process_count) -ne $MEMBER_COUNT ]]; do
        echo "Waiting for initial sponsor to start up"
        sleep 1
    done
    echo "Initial sponsor is started"
    while [[ $(get_members_up_count_by_unique_ports) < $MEMBER_COUNT ]]; do
        echo "Initial sponsor's port is still bound more than once"
        sleep 1
    done
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

if [ $# -ne 6 ]; then
    echo "Not enough parameters" 1>&2
    echo "Usage: $0 <service_id> <member_count> <scatter_delay_min(ms)> <scatter_delay_max(ms)> <crypto_algorithm> <key_agreement_protocol>"
    echo "Example: $0 42 20 10 100 DEFAULT_DH|ECC_DH PROTO_STR_DH|PROTO_DST_DH"
    exit 1
fi

if [[ $1 -lt 1 ]]; then
    echo "service id must be greater than 0"
    exit 1
fi

if [[ $2 -lt 1 ]]; then
    echo "member count must be greater than 1"
    exit 1
fi

if [[ $5 != "DEFAULT_DH" && $5 != "ECC_DH" ]]; then
    echo "Crypto algorithm must be DEFAULT_DH|ECC_DH"
    exit 1
fi

if [[ $6 != "PROTO_STR_DH" && $6 != "PROTO_DST_DH" ]]; then
    echo "Key agreement must be PROTO_STR_DH|PROTO_DST_DH"
    exit 1
fi

trap 'on_exit' SIGINT

compile $5 $6
start $1 $2 $3 $4 $5 $6
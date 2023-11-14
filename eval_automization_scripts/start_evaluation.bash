#!/bin/bash

compile() {
    local CRYPTO_ALGORITHM=$1
    local KEY_AGREEMENT_PROTOCOL=$2
    local ABSOLUTE_PROJECT_PATH=$3
    sed -i -E "s/add_compile_definitions\(.*\)/add_compile_definitions($CRYPTO_ALGORITHM $KEY_AGREEMENT_PROTOCOL)/" ${ABSOLUTE_PROJECT_PATH}/CMakeLists.txt
    echo "Compiling all targets..."
    $(which cmake) --build ${ABSOLUTE_PROJECT_PATH}/build --config Release --target all -j $(nproc) --
    echo "All targets are compiled."
}

get_process_count() {
    echo $(pgrep multicast-dh | wc -l)
}

get_subscriber_count() {
    echo $(($1-1))
}

get_members_up_count_by_unique_ports() {
    echo $(ss -lup | grep multicast-dh-ex | awk '{print $4}' |  awk -F: '{print $NF}' | grep -v $1 | sort -u | wc -l)
}

start() {
    local SERVICE_ID=$1
    local MEMBER_COUNT=$2
    local SCATTER_DELAY_MIN=$3
    local SCATTER_DELAY_MAX=$4
    local CRYPTO_ALGORITHM=$5
    local KEY_AGREEMENT_PROTOCOL=$6
    local ABSOLUTE_PROJECT_PATH=$7
    local ABSOLUTE_RESULTS_DIRECTORY_PATH=$8
    local LISTENING_INTERFACE_BY_IP=$9
    local MULTICAST_IP=${10}
    local MULTICAST_PORT=${11}

    echo "Starting $KEY_AGREEMENT_PROTOCOL $CRYPTO_ALGORITHM with $MEMBER_COUNT members"

    ${ABSOLUTE_PROJECT_PATH}/build/statistics-writer-main $MEMBER_COUNT $ABSOLUTE_RESULTS_DIRECTORY_PATH &
    while [[ -z $(pgrep statistics-wr) ]]; do
        echo "Waiting for statistics writer to start up"
        sleep 1
    done
    echo "statistics-writer is started"

    for (( i=0; i<$(get_subscriber_count $MEMBER_COUNT); i++ ))
    do
        ${ABSOLUTE_PROJECT_PATH}/build/multicast-dh-example false $SERVICE_ID $MEMBER_COUNT $SCATTER_DELAY_MIN $SCATTER_DELAY_MAX $LISTENING_INTERFACE_BY_IP $MULTICAST_IP $MULTICAST_PORT &
    done
    while [[ $(get_process_count) -ne $(get_subscriber_count $MEMBER_COUNT) ]]; do
        echo "Waiting for all subscribers to start up, $(get_process_count)/$(get_subscriber_count $MEMBER_COUNT) are up"
        sleep 1
    done
    echo "All subscribers started up"
    while [[ $(get_members_up_count_by_unique_ports $MULTICAST_PORT) < $(get_subscriber_count $MEMBER_COUNT) ]]; do
        echo "There are still ports bound more than once"
        sleep 1
    done
    ${ABSOLUTE_PROJECT_PATH}/build/multicast-dh-example true $SERVICE_ID $MEMBER_COUNT $SCATTER_DELAY_MIN $SCATTER_DELAY_MAX $LISTENING_INTERFACE_BY_IP $MULTICAST_IP $MULTICAST_PORT &
    # while [[ $(get_process_count) -ne $MEMBER_COUNT ]]; do
    #     echo "Waiting for initial sponsor to start up"
    #     sleep 1
    # done
    echo "Initial sponsor is started"
    # while [[ $(get_members_up_count_by_unique_ports $MULTICAST_PORT) < $MEMBER_COUNT ]]; do
    #     echo "Initial sponsor's port is still bound more than once"
    #     sleep 1
    # done
    # echo "Waiting for statistics writer to stop"
    # while [[ -n $(pgrep statistics-wr) ]]; do
    #     sleep 1
    # done
    # echo "statistics-writer is stopped"
    # while [[ $(get_members_up_count_by_unique_ports $MULTICAST_PORT) > 0 ]]; do
    #     echo "Waiting for all members to stop"
    #     sleep 1
    # done
    # echo "All members are stopped"
    wait
    echo "statistics-writer and all members are stopped"
}

function on_exit() {
    echo "Terminating all processes ..."
    pkill "statistics-wr"
    pkill "multicast-dh"
    echo "All processes terminated"
    exit 1
}

if [ $# -ne 11 ]; then
    echo "Not enough parameters" 1>&2
    echo "Usage: $0 <service_id> <member_count> <scatter_delay_min(ms)> <scatter_delay_max(ms)> <crypto_algorithm> <key_agreement_protocol> <absolute_project_path> <absolute_results_directory_path> <listening_interface_by_ip> <multicast_ip> <multicast_port>"
    echo "Example: $0 42 20 10 100 DEFAULT_DH|ECC_DH PROTO_STR_DH|PROTO_DST_DH /path/to/project/directory /path/to/results/directory 127.0.0.1 239.255.0.1 65000"
    exit 1
fi

if [[ $1 -lt 1 ]]; then
    echo "service id must be greater than 0"
    exit 1
fi

if [[ $2 -lt 2 ]]; then
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

last_char_idx="$((${#7} - 1))"
last_char="${7:last_char_idx}"
CORRECTED_ABSOLUTE_PROJECT_PATH=$7
if [[ $last_char == "/" ]]; then
    CORRECTED_ABSOLUTE_PROJECT_PATH=${7:0:$last_char_idx}
fi
trap 'on_exit' SIGINT

compile $5 $6 $CORRECTED_ABSOLUTE_PROJECT_PATH
start $1 $2 $3 $4 $5 $6 $CORRECTED_ABSOLUTE_PROJECT_PATH $8 $9 ${10} ${11}
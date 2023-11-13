#!/bin/bash
ABSOLUTE_PROJECT_PATH="/home/mehmet/vscode-workspaces/c++-multicast"
ABSOLUTE_RESULTS_DIRECTORY_PATH=${ABSOLUTE_PROJECT_PATH}/statistic_results/test_runs_without_retransmissions
SERVICE_ID=42
SCATTER_DELAY_MIN=0
SCATTER_DELAY_MAX=0
LISTENING_INTERFACE_BY_IP="127.0.0.1"
MULTICAST_IP="239.255.0.1"
MULTICAST_PORT=65000
KEY_AGREEMENT_PROTOCOL=('PROTO_DST_DH' 'PROTO_STR_DH')
# CRYPTO_ALGORITHM=('DEFAULT_DH' 'ECC_DH')
CRYPTO_ALGORITHM=('ECC_DH')

RUNS=100

for protocol in "${KEY_AGREEMENT_PROTOCOL[@]}"; do
    for algorithm in "${CRYPTO_ALGORITHM[@]}"; do
        for i in $(seq 1 $RUNS); do
            echo "Running ${protocol}-${algorithm} ${i}/${RUNS}"
            # for member_count in {100..1000..100}; do
            for member_count in {2..26}; do
                ${ABSOLUTE_PROJECT_PATH}/eval_automization_scripts/start_evaluation.bash $SERVICE_ID $member_count $SCATTER_DELAY_MIN $SCATTER_DELAY_MAX $algorithm $protocol $ABSOLUTE_PROJECT_PATH $ABSOLUTE_RESULTS_DIRECTORY_PATH 1>/dev/null
            done
        done
    done
done

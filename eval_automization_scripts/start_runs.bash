#!/bin/bash

SERVICE_ID=42
SCATTER_DELAY_MIN=10
SCATTER_DELAY_MAX=100
CRYPTO_ALGORITHM=('DEFAULT_DH' 'ECC_DH')
KEY_AGREEMENT_PROTOCOL=('PROTO_DST_DH' 'PROTO_STR_DH')

RUNS=100

for protocol in "${KEY_AGREEMENT_PROTOCOL[@]}"; do
    for algorithm in "${CRYPTO_ALGORITHM[@]}"; do
        for i in $(seq 1 $RUNS); do
            echo "Running ${protocol}-${algorithm} ${i}/${RUNS}"
            for member_count in {100..1000..100}; do
                /root/c++-multicast/eval_automization_scripts/start_evaluation.bash $SERVICE_ID $member_count $SCATTER_DELAY_MIN $SCATTER_DELAY_MAX $algorithm $protocol 1>/dev/null
            done
        done
    done
done
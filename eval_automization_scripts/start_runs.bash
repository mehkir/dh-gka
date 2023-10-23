#!/bin/bash

SERVICE_ID=42
SCATTER_DELAY_MIN=10
SCATTER_DELAY_MAX=100
CRYPTO_ALGORITHM=('DEFAULT_DH' 'ECC_DH')
KEY_AGREEMENT_PROTOCOL=('PROTO_DST_DH' 'PROTO_STR_DH')

for protocol in "${KEY_AGREEMENT_PROTOCOL[@]}"; do
    for algorithm in "${CRYPTO_ALGORITHM[@]}"; do
        for member_count in {100..1000..100}; do
            ./start_evaluation.bash $SERVICE_ID $member_count $SCATTER_DELAY_MIN $SCATTER_DELAY_MAX $algorithm $protocol
        done
    done
done
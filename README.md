# Standalone Group Key Agreement (GKA)
Evaluates a distributed and a contributory ECC as well as traditional Diffie-Hellman (DH) GKA approach.

## Dependencies
- [boost 1.83](https://launchpad.net/~mhier/+archive/ubuntu/libboost-latest)
- [cmake](https://apt.kitware.com/)
- [cryptopp](https://github.com/weidai11/cryptopp)

## Prerequisites
Adjust the `ABSOLUTE_PROJECT_PATH` variable according to your project path.

## Evaluation
The `eval_automization_scripts/start_runs.bash` script starts evaluation runs. The parameters before the for loop can be adjusted. The key agreement protocols and cryptography algorithms can be adjusted by the following compile definitions:

### Key Agreement Protocols
- `PROTO_DST_DH`: The distributed DH protocol
- `PROTO_STR_DH`: The contributory DH protocol

### Cryptography Algorithms
- `DEFAULT_DH`: The traditional DH cryptography algorithm
- `ECC_DH`: The Elliptic Curve DH cryptography algorithm

### Retransmissions
The protocols are also able to maintain the key agreement despite message loss by adding the `RETRANSMISSIONS` compile definition in the `eval_automization_scripts/start_evaluation.bash` script to the `compile` method (e.g., `... add_compile_definitions($CRYPTO_ALGORITHM $KEY_AGREEMENT_PROTOCOL RETRANSMISSIONS) ...`)

### Large Send and Receive Buffers
Large send and receive buffers can be used to carry out the evaluation with several hundred processes without retransmissions. For example, if you want to use 8GB (1024\*1024*8=8388608) for the buffers, create the file `/etc/sysctl.d/99-netbuffer.conf`. Then insert <br />
`net.core.rmem_max = 8388608`<br />
`net.core.wmem_max = 8388608`<br />
in separate lines in `/etc/sysctl.d/99-netbuffer.conf`. To apply the changes reboot or execute `sysctl -p /etc/sysctl.d/99-netbuffer.conf`.

## Known Issues
- It is possible that multiple processes get the same port, which is handled by `multicast_channel/multicast_channel.cpp` in the `multicast_channel::is_port_bound_once` method

## References
[1] Y. Kim et al., “Group Key Agreement Efficient in Communication,”
IEEE Transactions on Computers, vol. 53, pp. 905–921, Jul. 2004. <br/>
[2] Y. Amir et al., “On the Performance of Group Key Agreement Protocols,” ACM Transactions on Information and System Security, vol. 7, pp. 457–488, Aug. 2004.
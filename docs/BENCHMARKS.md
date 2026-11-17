# FORGE benchmarks

The benchmark executable measures the current fixed-record storage path and the custom hash index. It uses a temporary directory and removes it when the run completes.

## Build and run

```bash
cmake -S forge -B build/forge -DCMAKE_BUILD_TYPE=Release
cmake --build build/forge --target forge-benchmarks --parallel 2
./build/forge/forge-benchmarks --messages 1000
```

The count is limited to 1–1000 messages so the benchmark remains practical with the current append-and-scan implementation.

## Measurements

The executable reports:

- Message + recipient + mailbox-state transaction throughput (operations/second).
- Full message, recipient, and mailbox-state scan times.
- Hash-table insert and lookup times.
- A correctness flag confirming that every expected record and index entry was found.

The write path includes the current write-ahead journal and mailbox-state creation. The read measurements are full-file scans, not indexed point reads. The benchmark is a reproducible baseline, not a claim that the custom database outperforms a production database.

## Reporting requirements

For any published comparison, record the CPU model, core count, RAM, OS/kernel, compiler version, build type, message count, and at least five runs. Report the median and range; do not compare measurements from different machines as if they were controlled.

CI runs `forge-benchmarks --messages 100` as a smoke test. Those runner measurements are environment-specific and should not be treated as production capacity numbers.

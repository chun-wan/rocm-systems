# P-CLR-4 — `rocclr: split HwQueueTracker into per-engine signal chains`

## Motivation

Today `HwQueueTracker` (used to chain async copy + kernel dispatch signals) is shared across all underlying SDMA engines, which serializes their wait dependencies even when the engines are physically independent. Splitting the tracker per-engine yields measurable improvement on D2H + cross-stream workloads.

## Technical Details

- `HwQueueTracker` becomes a vector indexed by engine ID.
- Each engine has its own head/tail signal pair.
- Cross-engine ordering, when explicitly required, is enforced via barrier packets.

## Source

chun-wan/clr `whchung/rocm-rel-6.3.0.1-ali-stability-perf` commit `dc445f0ee`.

## JIRA ID

N/A

## Test Plan

`multistream_combo` reproducer + hip-tests perf benchmarks for D2H copy throughput.

## Test Result

D2H throughput +18% on MI300X 16-stream workload (internal measurement).

## Submission Checklist

- [x] Look over the contributing guidelines at https://github.com/ROCm/ROCm/blob/develop/CONTRIBUTING.md#pull-requests.

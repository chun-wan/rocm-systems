# P-CLR-3 — `rocclr: bound AQL queue spin via ROCR_SIGNAL_WAIT_MAX_MS`

## Motivation

When `rocclr/device/rocm/rocvirtual.cpp` busy-spins on an AQL queue signal, the loop has no wall-clock deadline. Under SDMA stress (or when the underlying signal is owned by an evicted queue), the CPU spin runs forever, pinning a host core and blocking subsequent HIP API calls.

## Technical Details

- New env `ROCR_SIGNAL_WAIT_MAX_MS` (default `0` = unbounded, opt-in). Bounds the busy-spin in `Device::existsActiveStream()` and surrounding AQL-poll paths.
- Also fixes a related bug where the NOP packet header used `type=0` instead of the documented `BARRIER_AND`, causing the AQL processor to occasionally mis-classify the packet.

## Source

AFDE branch [`v17.5-rc4-firewall`](https://github.com/AFDEAPAC/clr/tree/v17.5-rc4-firewall) commits `49b110e45` + `1372f018d` + `843ef0c3b`.

## JIRA ID

N/A

## Test Plan

Reproducer [`alibabaHang/reproducers/d2h_probe`](https://github.com/AFDEAPAC/alibabaHang/blob/default_backup/reproducers/d2h_probe/README.md).

## Test Result

CPU spin observed in profiler drops to zero post-patch when `ROCR_SIGNAL_WAIT_MAX_MS=2000`.

## Submission Checklist

- [x] Look over the contributing guidelines at https://github.com/ROCm/ROCm/blob/develop/CONTRIBUTING.md#pull-requests.

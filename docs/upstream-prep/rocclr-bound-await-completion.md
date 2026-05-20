# P-CLR-1 — `rocclr: bound Event::awaitCompletion via HIP_AWAIT_FAIL_MS`

## Motivation

In long-running multi-stream HIP services (e.g. inference serving, RDMA peer-direct workloads), `Event::awaitCompletion()` and `Device::SyncAllStreams()` can park indefinitely when a downstream signal never fires (queue eviction, GPU reset in progress, RDMA dereg in flight, ...). The blocked thread holds locks that cascade into `hipFree`/`hipHostUnregister` stalls; we measured single-call `hipFree` latency of **264 seconds** under cgroup pressure. A bounded wall-clock deadline avoids the cascade.

## Technical Details

- New env `HIP_AWAIT_FAIL_MS` (default `0` = unbounded, opt-in). When >0, `Event::awaitCompletion()`, `Device::SyncAllStreams()` and their callers (`hipHostUnregister`, `hipFreeArray`, `hipIpcClose`, `hipDeviceSynchronize`) honor this deadline and propagate `hipErrorNotReady` to the caller.
- A per-device "await-degraded" latch auto-clears when a subsequent signal observes `CL_COMPLETE` again, so the runtime exits the bounded-wait state without a service restart.
- Split the CLR-layer signal env from the lower-level ROCr signal env (`ROCR_SIGNAL_WAIT_MAX_MS`) so callers can tune the two layers independently.

## Source

AFDE branch [`v17.5-rc4-firewall`](https://github.com/AFDEAPAC/clr/tree/v17.5-rc4-firewall) commits `b4ee33b51`, `10c778fa6`, `b4a9f8d42`, `abafb82f6`, `766c3156e`, `c3895b3c3`, `896757cfa`, `405d0c69f`, `fa6dae6ee`, `1bb799008`, `a65cc36e1`, `fceef8a8e`, `24637ed66`, `bab9a17dd` (14 iterative review-fixes; squashed for upstream).

## JIRA ID

N/A

## Test Plan

Reproducer [`alibabaHang/reproducers/customer_hang_repro`](https://github.com/AFDEAPAC/alibabaHang/blob/default_backup/reproducers/customer_hang_repro/README.md): 8-stream, 16 GiB cgroup. Before patch: `hipFree` parks for 264s+ under contention; after patch with `HIP_AWAIT_FAIL_MS=5000`: 5s deadline + `hipErrorNotReady` return.

## Test Result

- 30-min stress on MI300X: PASS, no `hipFree` longer than 6s, no permanent stuck threads.
- hip-tests unit/event passes.

## Submission Checklist

- [x] Look over the contributing guidelines at https://github.com/ROCm/ROCm/blob/develop/CONTRIBUTING.md#pull-requests.

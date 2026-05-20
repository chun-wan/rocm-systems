# P-CLR-2 — `rocclr: do not abort() on per-queue HSA exception`

## Motivation

`amd::roc::callbackQueue()` in `rocclr/device/rocm/rocdevice.cpp` hard-codes `abort()` for any non-success `hsa_status_t` raised by the per-queue exception handler. In multi-tenant serving (e.g. vLLM + RDMA + small HIP services on one GPU), a single bad request — a kernel reading freed RDMA memory, a queue eviction, a single SDMA timeout — sends `SIGABRT` to the whole process and kills every other in-flight request. Stock CLR offers no env / hook to attenuate this.

This PR replaces the bare `abort()` with a survivable path: log + invalidate the affected queue + return the failure as `hipErrorOperationFailed` to the next user-visible API call. A new env `HIP_HANG_RECOVERY_ENABLE` gates the whole new path (default OFF for upstream backward-compat). A custom SIGABRT handler keeps the rest of the process alive when ROCr itself raises a signal.

## Technical Details

- `amd::roc::callbackQueue` no longer calls `abort()` when `HIP_HANG_RECOVERY_ENABLE=1`. It marks the queue as `dead`, fan-outs `hipErrorOperationFailed` to all in-flight HIP events on the queue, and notifies the user via the existing `BackendErrorCallBack` machinery.
- New shader-side bypass on signal-wait timeout (the "v10" patch in our internal numbering): when the timeout fires, the dispatched compute kernel is forced into a `permanent-bypass` state for the rest of the queue's life, preventing further crashes on the same code path. The bypass auto-clears after a successful SDMA queue reset.
- `gpu_error_` and `g_hang_recovery_active_` flags are now separate. A VM fault on one queue no longer fails all subsequent API calls system-wide.
- Co-operative GPU system event handler keeps ROCr from raising the abort path on queue eviction events alone.

## Source

AFDE/chun-wan branch [`whchung/rocm-rel-6.3.0.1-ali-stability-perf`](https://github.com/chun-wan/clr/tree/whchung/rocm-rel-6.3.0.1-ali-stability-perf) HEAD `3e9979b57` (= rocclr v10 v9.2). Iteration history: v5 -> v10. See [PROJECT_TUMBLER.md](https://github.com/AFDEAPAC/alibabaHang/blob/main/docs/PROJECT_TUMBLER.md) section 2 clr.

## JIRA ID

N/A

## Test Plan

Reproducer [`alibabaHang/reproducers/customer_hang_repro`](https://github.com/AFDEAPAC/alibabaHang/blob/default_backup/reproducers/customer_hang_repro/README.md) + [`reproducers/multistream_combo`](https://github.com/AFDEAPAC/alibabaHang/blob/default_backup/reproducers/multistream_combo/README.md). Before: single bad request -> SIGABRT -> process dies, 7 other in-flight requests fail. After: bad request fails with `hipErrorOperationFailed`, the other 7 requests complete normally.

## Test Result

- Customer's MI300X torch_service workload: 11 hours stable under same load that previously crashed in 6 minutes.
- hip-tests unit/error pass with `HIP_HANG_RECOVERY_ENABLE=0` (default, no behavior change).
- hip-tests unit/error pass with `HIP_HANG_RECOVERY_ENABLE=1` (new behavior, returns error instead of abort).

## Submission Checklist

- [x] Look over the contributing guidelines at https://github.com/ROCm/ROCm/blob/develop/CONTRIBUTING.md#pull-requests.

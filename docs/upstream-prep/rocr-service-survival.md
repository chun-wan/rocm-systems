# P-ROCR-2 — `rocr: do not abort() VMFaultHandler / memory error handler in service-survival mode`

## Motivation

ROCr's default policy on VM fault / memory error is to fire `abort()` on the **process**, killing every in-flight request. For serving workloads this is unnecessarily aggressive — a single faulty kernel should not bring down 7 healthy peer requests.

## Technical Details

- New env `ROCR_SERVICE_SURVIVAL=1` (default `0` for backward-compat). When set:
  - `VMFaultHandler` logs + marks the offending queue dead and returns instead of calling `abort()`.
  - Memory-error event handler does the same.
  - The HSA queue's exception bit is still set so CLR's `callbackQueue()` (see PR P-CLR-2) can route the error to user.
- Default behavior unchanged when env is not set, so existing single-tenant users see no difference.

## Source

AFDE branch [`v17.5-rc4-firewall`](https://github.com/AFDEAPAC/rocr/tree/v17.5-rc4-firewall) commits `5680c9f` (FIX-P0-2) + `bb7258f` (FIX-P0-3).

## JIRA ID

N/A

## Test Plan

Reproducer [`alibabaHang/reproducers/customer_hang_repro`](https://github.com/AFDEAPAC/alibabaHang/blob/default_backup/reproducers/customer_hang_repro/README.md).

## Test Result

VM fault on one queue no longer kills the process. CLR PR P-CLR-2 then routes the error to user code as `hipErrorOperationFailed`. Stable for 11+ hours on customer workload.

## Submission Checklist

- [x] Look over the contributing guidelines at https://github.com/ROCm/ROCm/blob/develop/CONTRIBUTING.md#pull-requests.

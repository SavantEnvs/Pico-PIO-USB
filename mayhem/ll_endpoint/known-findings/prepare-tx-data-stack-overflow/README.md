# prepare_tx_data stack-buffer-overflow

`prepare_tx_data()` (src/pio_usb.c) copies `xact_len` bytes into `uint8_t buffer[PIO_USB_EP_SIZE + 4]`
(68 bytes) but `xact_len` is bounded only by the endpoint's `max_size`, which the host stack takes from
the attached device's endpoint descriptor. An endpoint with `wMaxPacketSize` above 64 and a transmit
transfer of that size overflows the stack buffer.

The `ll_endpoint` fuzz target rejects descriptors with `wMaxPacketSize` > `PIO_USB_EP_SIZE` (64) so this crash
does not end every run before the rest of the transfer state machine is exercised; the bug stays reproducible with
`ll_endpoint-oversize-standalone`, the same harness built with `-DLL_ENDPOINT_ALLOW_OVERSIZE`.

Reproduce: `/mayhem/ll_endpoint-oversize-standalone mayhem/ll_endpoint/known-findings/prepare-tx-data-stack-overflow/repro`
-> ASan `stack-buffer-overflow` WRITE of 256 bytes in `prepare_tx_data` via `pio_usb_ll_transfer_start`.

Not in `testsuite/` because every seed there must run clean.

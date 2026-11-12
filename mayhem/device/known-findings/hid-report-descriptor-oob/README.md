# HID report descriptor: host-controlled index and length

On `GET_DESCRIPTOR(HID report)` the device stack (src/pio_usb_device.c, `pio_usb_device_task`) does
`prepare_ep0_data(descriptor_buffers.hid_report[wIndex_lsb], wLength)` with both values taken
straight from the host's SETUP packet: no bound on the index into the `hid_report` array and no clamp of
the length to the real report descriptor size.

- `repro_bad_index`: wIndex = 0xfa -> NULL/garbage pointer is copied by `prepare_tx_data`
  (ASan SEGV in `__asan_memcpy` from `prepare_tx_data` <- `prepare_ep0_data`).
- `repro_long_length`: valid index 0, wLength = 64 for an 8-byte descriptor -> ASan
  `global-buffer-overflow` READ of 64 bytes.

Reproduce: `/mayhem/device-standalone mayhem/device/known-findings/hid-report-descriptor-oob/<file>`.

Not in `testsuite/` because every seed there must run clean.

# String descriptor: host-controlled index

On `GET_DESCRIPTOR(string)` the device stack (src/pio_usb_device.c, `pio_usb_device_task`, `DESC_TYPE_STRING`)
reads `descriptor_buffers.string[ep0_desc_request_idx]` with the index taken straight from the low byte of
`wValue` in the host's SETUP packet, then copies `str[0] & 0xff` bytes from it. Nothing checks the index against
the number of string descriptors the application supplied (the harness has 3).

- `repro`: `GET_DESCRIPTOR(string, index 3)` -> ASan `global-buffer-overflow` READ at `pio_usb_device.c:396`
  (the element just past the 3-entry array).

Reproduce: `/mayhem/device-standalone mayhem/device/known-findings/string-descriptor-index-oob/repro`
(or `/mayhem/device <file>`). Which neighbouring global ASan reports depends on the link layout, and an index far
past the array can land inside another valid global and read without a report, so use index 3.

Not in `testsuite/` because every seed there must run clean.

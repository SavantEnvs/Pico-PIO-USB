#include "mocksdk.h"
#include "pio_usb.h"
#include "pio_usb_ll.h"
#include <stdlib.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size == 0 || size > 255) return -1;
  uint8_t len = (uint8_t)size;
  size_t cap = (size_t)len * 2 * 7 / 6 + 2;
  uint8_t *out = (uint8_t *)malloc(cap);
  uint8_t n = pio_usb_ll_encode_tx_data(data, len, out);
  free(out);
  return n == 0 ? -1 : 0;
}

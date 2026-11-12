#include "mocksdk.h"
#include "usb_crc.h"

static uint8_t ref_crc5(uint16_t d) {
  uint8_t c = 0x1f;
  for (int i = 0; i < 11; i++) {
    int b = ((d >> i) & 1) ^ (c & 1);
    c >>= 1;
    if (b) c ^= 0x14;
  }
  return c ^ 0x1f;
}

static uint16_t ref_crc16(const uint8_t *p, size_t n) {
  uint16_t c = 0xffff;
  for (size_t i = 0; i < n; i++) {
    c ^= p[i];
    for (int k = 0; k < 8; k++) c = (c & 1) ? (uint16_t)((c >> 1) ^ 0xA001) : (uint16_t)(c >> 1);
  }
  return c ^ 0xffff;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size < 2) return -1;
  uint16_t tok = (uint16_t)(data[0] | (data[1] << 8)) & 0x7ff;
  if (calc_usb_crc5(tok) != ref_crc5(tok)) __builtin_trap();

  const uint8_t *p = data + 2;
  size_t n = size - 2;
  uint16_t crc = calc_usb_crc16(p, (uint16_t)n);
  if (crc != ref_crc16(p, n)) __builtin_trap();

  uint16_t run = 0xffff;
  for (size_t i = 0; i < n; i++) run = update_usb_crc16(run, p[i]);
  if ((uint16_t)(run ^ 0xffff) != crc) __builtin_trap();
  return 0;
}

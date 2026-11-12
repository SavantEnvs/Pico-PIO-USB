/* Known-answer probe: drives the real library (mock SDK underneath) and prints computed values. */
#include "mocksdk.h"
#include "pio_usb.h"
#include "pio_usb_ll.h"
#include "usb_crc.h"
#include <stdio.h>

static const uint8_t device_desc[18] = {18, 1, 0x00, 0x02, 0, 0, 0, 64, 0x34, 0x12, 0x78, 0x56, 0x00, 0x01, 1, 2, 0, 1};
static const uint8_t config_desc[34] = {
    9, 2, 34, 0, 1, 1, 0, 0xa0, 50,
    9, 4, 0, 0, 1, 3, 1, 1, 0,
    9, 0x21, 0x11, 0x01, 0, 1, 0x22, 8, 0,
    7, 5, 0x81, 3, 8, 0, 10};
static const uint8_t hid_report_desc[8] = {0x05, 0x01, 0x09, 0x06, 0xa1, 0x01, 0xc0, 0x00};
static const uint8_t *hid_reports[1] = {hid_report_desc};
static const string_descriptor_t strings[2] = {
    {4, 3, {0x09, 0x04}},
    {10, 3, {'A', 0, 'c', 0, 'm', 0, 'e', 0}},
};

static void hex(const char *tag, const uint8_t *p, size_t n) {
  printf("%s ", tag);
  for (size_t i = 0; i < n; i++) printf("%02x", p[i]);
  printf("\n");
}

static void setup(uint8_t b0, uint8_t b1, uint8_t v_lo, uint8_t v_hi, uint8_t idx, uint8_t len) {
  uint8_t req[8] = {b0, b1, v_lo, v_hi, idx, 0, len, 0};
  uint16_t dat = 0;
  uint8_t crc = calc_usb_crc5(dat);
  uint8_t tok[4] = {USB_SYNC, USB_PID_SETUP, 0, (uint8_t)(crc << 3)};
  uint8_t pkt[14] = {USB_SYNC, USB_PID_DATA0};
  memcpy(pkt + 2, req, 8);
  uint16_t c16 = calc_usb_crc16(req, 8);
  pkt[10] = c16 & 0xff;
  pkt[11] = c16 >> 8;
  mock_rx_flush();
  mock_rx_add_packet(tok, 4);
  mock_rx_add_packet(pkt, 12);
  mock_pio_hw[0].irq = 1u << IRQ_RX_START;
  mock_irq_handler();
  pio_usb_device_task();
}

static void show_ep1(const char *tag) {
  endpoint_t *ep = PIO_USB_ENDPOINT(1);
  printf("%s has_transfer=%d total_len=%u data_id=%u\n", tag, ep->has_transfer, ep->total_len, ep->data_id);
  hex("  encoded", ep->buffer, ep->encoded_data_len);
}

int main(void) {
  printf("crc16(123456789)=%04x\n", calc_usb_crc16((const uint8_t *)"123456789", 9));
  printf("crc5(addr=0x15,ep=0xe)=%02x\n", calc_usb_crc5(0x15 | (0xe << 7)));

  uint8_t ack[2] = {USB_SYNC, USB_PID_ACK};
  uint8_t enc[16] = {0};
  uint8_t n = pio_usb_ll_encode_tx_data(ack, 2, enc);
  hex("encode(sync,ack)", enc, n);

  mock_rx_reset();
  pio_usb_configuration_t cfg = PIO_USB_DEFAULT_CONFIG;
  usb_descriptor_buffers_t bufs = {device_desc, config_desc, hid_reports, strings};
  pio_usb_device_init(&cfg, &bufs);
  mock_se0_polls = 1001;
  pio_usb_device_task();
  printf("after reset: ep0.size=%u ep1.ep_num=%02x dev_addr=%u\n", PIO_USB_ENDPOINT(0)->size, PIO_USB_ENDPOINT(1)->ep_num, PIO_USB_ROOT_PORT(0)->dev_addr);

  setup(0x80, 0x06, 0x00, 0x01, 0x00, 0x12);
  show_ep1("get_device");
  PIO_USB_ENDPOINT(1)->has_transfer = false;
  setup(0x80, 0x06, 0x00, 0x02, 0x00, 0x09);
  show_ep1("get_config9");
  PIO_USB_ENDPOINT(1)->has_transfer = false;
  setup(0x80, 0x06, 0x01, 0x03, 0x00, 0xff);
  show_ep1("get_string1");
  return 0;
}

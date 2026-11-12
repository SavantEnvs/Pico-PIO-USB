#include "mocksdk.h"
#include "pio_usb.h"
#include "pio_usb_ll.h"
#include "usb_crc.h"

static const uint8_t device_desc[18] = {18, 1, 0x00, 0x02, 0, 0, 0, 64, 0x34, 0x12, 0x78, 0x56, 0x00, 0x01, 1, 2, 0, 1};
static const uint8_t config_desc[34] = {
    9, 2, 34, 0, 1, 1, 0, 0xa0, 50,
    9, 4, 0, 0, 1, 3, 1, 1, 0,
    9, 0x21, 0x11, 0x01, 0, 1, 0x22, 8, 0,
    7, 5, 0x81, 3, 8, 0, 10};
static const uint8_t hid_report_desc[8] = {0x05, 0x01, 0x09, 0x06, 0xa1, 0x01, 0xc0, 0x00};
static const uint8_t *hid_reports[1] = {hid_report_desc};
static const string_descriptor_t strings[3] = {
    {4, 3, {0x09, 0x04}},
    {10, 3, {'A', 0, 'c', 0, 'm', 0, 'e', 0}},
    {8, 3, {'K', 0, 'e', 0, 'y', 0}},
};

static void queue_token(uint8_t pid, uint8_t addr, uint8_t ep, bool bad_crc) {
  uint16_t dat = (uint16_t)((addr & 0x7f) | ((ep & 0xf) << 7));
  uint8_t crc = calc_usb_crc5(dat);
  uint8_t tok[4] = {USB_SYNC, pid, (uint8_t)(dat & 0xff), (uint8_t)((crc << 3) | ((dat >> 8) & 0x1f))};
  if (bad_crc) tok[3] ^= 0x80;
  mock_rx_add_packet(tok, sizeof(tok));
}

static void queue_data(uint8_t pid, const uint8_t *payload, size_t n, bool bad_crc) {
  uint8_t pkt[160];
  pkt[0] = USB_SYNC;
  pkt[1] = pid;
  memcpy(pkt + 2, payload, n);
  uint16_t crc = calc_usb_crc16(payload, (uint16_t)n);
  if (bad_crc) crc ^= 0x0101;
  pkt[2 + n] = crc & 0xff;
  pkt[3 + n] = crc >> 8;
  mock_rx_add_packet(pkt, n + 4);
}

static void bus_reset(unsigned extra) {
  mock_se0_polls = 1001 + extra;
  pio_usb_device_task();
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size < 3) return -1;

  mock_rx_reset();
  memset(pio_usb_ep_pool, 0, sizeof(pio_usb_ep_pool));
  memset(pio_port, 0, sizeof(pio_port));
  pio_usb_device_set_address(0);

  pio_usb_configuration_t cfg = PIO_USB_DEFAULT_CONFIG;
  usb_descriptor_buffers_t bufs = {device_desc, config_desc, hid_reports, strings};
  pio_usb_device_init(&cfg, &bufs);
  bus_reset(0);

  size_t i = 0;
  while (i + 3 <= size) {
    uint8_t ctl = data[i], epb = data[i + 1];
    size_t len = data[i + 2];
    i += 3;
    if (len > size - i) len = size - i;
    if (len > 120) len = 120;
    const uint8_t *payload = data + i;
    i += len;

    uint8_t kind = ctl & 7;
    uint8_t pid;
    switch (kind) {
      case 0: case 1: pid = USB_PID_SETUP; break;
      case 2: case 3: pid = USB_PID_OUT; break;
      case 4: pid = USB_PID_SOF; break;
      /* IN is skipped for a mock limit, not a library precondition: pio_usb_bus_start_receive() spins until
       * the write-1-to-clear PIO irq register reads 0, which plain-memory registers never do. */
      case 5: pid = epb; if (pid == USB_PID_IN) return -1; break;
      default: bus_reset(len); continue;
    }

    uint8_t addr = (ctl & 0x10) ? (uint8_t)(epb ^ 0x55) : PIO_USB_ROOT_PORT(0)->dev_addr;
    uint8_t ep = (ctl & 0x80) ? (uint8_t)(epb & 0xf) : 0;

    mock_rx_flush();
    queue_token(pid, addr, ep, (ctl & 0x08) != 0);
    if (pid == USB_PID_SETUP || pid == USB_PID_OUT) {
      queue_data((ctl & 0x40) ? USB_PID_DATA1 : USB_PID_DATA0, payload, len, (ctl & 0x20) != 0);
    }
    mock_pio_hw[0].irq = 1u << IRQ_RX_START;

    mock_irq_handler();
    pio_usb_device_task();
  }
  return 0;
}

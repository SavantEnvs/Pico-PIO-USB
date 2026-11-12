#include "mocksdk.h"
#include "usb_rx.pio.h"

pio_hw_t mock_pio_hw[2];
irq_handler_t mock_irq_handler;
unsigned mock_se0_polls;

#define MAX_PKTS 8
static uint8_t pkt_data[MAX_PKTS][160];
static size_t pkt_len[MAX_PKTS];
static unsigned pkt_cnt, pkt_cur;
static size_t pkt_pos;
static mock_timer_hw_t timer_hw;

void mock_rx_flush(void) {
  pkt_cnt = pkt_cur = 0;
  pkt_pos = 0;
}

void mock_rx_reset(void) {
  mock_rx_flush();
  mock_se0_polls = 0;
  memset(mock_pio_hw, 0, sizeof(mock_pio_hw));
}

void mock_rx_add_packet(const uint8_t *data, size_t len) {
  if (pkt_cnt >= MAX_PKTS) return;
  if (len > sizeof(pkt_data[0])) len = sizeof(pkt_data[0]);
  memcpy(pkt_data[pkt_cnt], data, len);
  pkt_len[pkt_cnt++] = len;
}

static size_t avail(void) {
  return pkt_cur < pkt_cnt ? pkt_len[pkt_cur] - pkt_pos : 0;
}

uint pio_sm_get_rx_fifo_level(PIO pio, uint sm) {
  (void)sm;
  size_t n = avail();
  if (n == 0) pio->irq |= (1u << IRQ_RX_EOP);
  return n > 8 ? 8 : (uint)n;
}

uint32_t pio_sm_get(PIO pio, uint sm) {
  (void)sm;
  if (avail() == 0) return 0;
  uint8_t b = pkt_data[pkt_cur][pkt_pos++];
  if (avail() == 0) pio->irq |= (1u << IRQ_RX_EOP);
  return (uint32_t)b << 24;
}

void pio_sm_clear_fifos(PIO pio, uint sm) {
  (void)pio; (void)sm;
  pkt_pos = pkt_cur < pkt_cnt ? pkt_len[pkt_cur] : 0;
}

void pio_sm_restart(PIO pio, uint sm) {
  (void)pio; (void)sm;
  if (pkt_cur < pkt_cnt) { pkt_cur++; pkt_pos = 0; }
}

mock_timer_hw_t *mock_timer(void) { timer_hw.timerawl++; return &timer_hw; }
uint32_t clock_get_hz(enum clock_index c) { (void)c; return 120000000u; }
void irq_set_exclusive_handler(uint num, irq_handler_t h) { (void)num; mock_irq_handler = h; }
void gpio_set_mask(uint32_t m) { (void)m; }
void gpio_clr_mask(uint32_t m) { (void)m; }

bool gpio_get(uint pin) {
  if (pin & 1) return true;
  if (mock_se0_polls) { mock_se0_polls--; return true; }
  return false;
}

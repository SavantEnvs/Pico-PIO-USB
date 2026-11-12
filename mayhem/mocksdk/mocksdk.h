/* Minimal host-side stand-in for the Raspberry Pi Pico SDK surface that Pico-PIO-USB uses.
 * PIO/DMA/GPIO registers are plain memory; the receive FIFO is fed by the harness. */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#ifndef PICO_SDK_VERSION_MAJOR
#define PICO_SDK_VERSION_MAJOR 2
#define PICO_SDK_VERSION_MINOR 1
#endif

typedef unsigned int uint;
typedef volatile uint32_t io_rw_32;
typedef const volatile uint32_t io_ro_32;

#define __not_in_flash(g)
#define __not_in_flash_func(f) f
#define __no_inline_not_in_flash_func(f) f
#define __time_critical_func(f) f
#define __force_inline inline __attribute__((always_inline))
#ifndef __always_inline
#define __always_inline inline __attribute__((always_inline))
#endif
#define __unused __attribute__((unused))
#define __compiler_memory_barrier() __asm__ volatile("" ::: "memory")

#define PIO_FDEBUG_TXSTALL_LSB 24
#define PIO_SM0_EXECCTRL_JMP_PIN_LSB 24
#define PIO_SM0_EXECCTRL_JMP_PIN_BITS (0x1fu << 24)

typedef struct {
  io_rw_32 clkdiv, execctrl, shiftctrl, addr, instr, pinctrl;
} pio_sm_hw_t;

typedef struct pio_hw {
  io_rw_32 ctrl, fstat, fdebug, flevel;
  io_rw_32 txf[4];
  io_rw_32 rxf[4];
  io_rw_32 irq;
  pio_sm_hw_t sm[4];
  io_rw_32 instr_mem[32];
} pio_hw_t;
typedef pio_hw_t *PIO;

struct pio_program {
  const uint16_t *instructions;
  uint8_t length;
  int8_t origin;
};
typedef struct pio_program pio_program_t;

typedef struct { uint32_t clkdiv, execctrl, shiftctrl, pinctrl; } pio_sm_config;

enum pio_fifo_join { PIO_FIFO_JOIN_NONE, PIO_FIFO_JOIN_TX, PIO_FIFO_JOIN_RX };
enum pio_src_dest { pio_pins, pio_x, pio_y, pio_null, pio_pindirs = 4, pio_exec_mov = 4, pio_status, pio_pc, pio_isr, pio_osr };
enum pio_interrupt_source { pis_interrupt0 = 8 };
enum gpio_override { GPIO_OVERRIDE_NORMAL, GPIO_OVERRIDE_INVERT, GPIO_OVERRIDE_LOW, GPIO_OVERRIDE_HIGH };
enum gpio_slew_rate { GPIO_SLEW_RATE_SLOW, GPIO_SLEW_RATE_FAST };
enum gpio_drive_strength { GPIO_DRIVE_STRENGTH_2MA, GPIO_DRIVE_STRENGTH_4MA, GPIO_DRIVE_STRENGTH_8MA, GPIO_DRIVE_STRENGTH_12MA };
enum dma_channel_transfer_size { DMA_SIZE_8, DMA_SIZE_16, DMA_SIZE_32 };
typedef struct { uint32_t ctrl; } dma_channel_config;
enum clock_index { clk_sys = 5 };

extern pio_hw_t mock_pio_hw[2];
#define PIO0_BASE 0u
#define PIO1_BASE (1u << 20)
#define PIO_INSTANCE(n) (&mock_pio_hw[(n)])
#define PIO_NUM(pio) ((pio) - mock_pio_hw)
#define NUM_PIO_IRQS 2u
#define PIO0_IRQ_0 7u
#define PIO_IRQ_NUM(pio, irqn) (PIO0_IRQ_0 + NUM_PIO_IRQS * PIO_NUM(pio) + (irqn))

static inline PIO pio_get_instance(uint n) { return &mock_pio_hw[n & 1]; }
static inline void pio_sm_claim(PIO pio, uint sm) { (void)pio; (void)sm; }
static inline void pio_sm_set_enabled(PIO pio, uint sm, bool en) { (void)pio; (void)sm; (void)en; }
static inline void pio_sm_exec(PIO pio, uint sm, uint instr) { (void)pio; (void)sm; (void)instr; }
static inline void pio_sm_init(PIO pio, uint sm, uint off, const pio_sm_config *c) { (void)pio; (void)sm; (void)off; (void)c; }
static inline void pio_sm_set_jmp_pin(PIO pio, uint sm, uint pin) { (void)pio; (void)sm; (void)pin; }
static inline void pio_sm_set_in_pins(PIO pio, uint sm, uint pin) { (void)pio; (void)sm; (void)pin; }
static inline void pio_sm_set_out_pins(PIO pio, uint sm, uint pin, uint n) { (void)pio; (void)sm; (void)pin; (void)n; }
static inline void pio_sm_set_set_pins(PIO pio, uint sm, uint pin, uint n) { (void)pio; (void)sm; (void)pin; (void)n; }
static inline void pio_sm_set_sideset_pins(PIO pio, uint sm, uint pin) { (void)pio; (void)sm; (void)pin; }
static inline void pio_sm_set_consecutive_pindirs(PIO pio, uint sm, uint pin, uint n, bool out) { (void)pio; (void)sm; (void)pin; (void)n; (void)out; }
static inline void pio_sm_set_pins_with_mask64(PIO pio, uint sm, uint64_t v, uint64_t m) { (void)pio; (void)sm; (void)v; (void)m; }
static inline void pio_sm_set_pindirs_with_mask64(PIO pio, uint sm, uint64_t v, uint64_t m) { (void)pio; (void)sm; (void)v; (void)m; }
static inline void pio_sm_set_clkdiv_int_frac(PIO pio, uint sm, uint16_t i, uint8_t f) { (void)pio; (void)sm; (void)i; (void)f; }
static inline void pio_gpio_init(PIO pio, uint pin) { (void)pio; (void)pin; }
static inline void pio_set_gpio_base(PIO pio, uint base) { (void)pio; (void)base; }
static inline void pio_set_irqn_source_enabled(PIO pio, uint n, uint src, bool en) { (void)pio; (void)n; (void)src; (void)en; }
static inline uint pio_get_dreq(PIO pio, uint sm, bool tx) { (void)pio; (void)sm; return tx ? 0 : 1; }
static inline uint pio_add_program(PIO pio, const pio_program_t *p) { (void)pio; (void)p; return 0; }
static inline void pio_add_program_at_offset(PIO pio, const pio_program_t *p, uint off) { (void)pio; (void)p; (void)off; }
static inline void pio_calculate_clkdiv_from_float(float div, uint16_t *i, uint8_t *f) {
  *i = (uint16_t)div;
  *f = (uint8_t)((div - (float)*i) * 256.0f);
}
void pio_sm_clear_fifos(PIO pio, uint sm);
void pio_sm_restart(PIO pio, uint sm);
uint pio_sm_get_rx_fifo_level(PIO pio, uint sm);
uint32_t pio_sm_get(PIO pio, uint sm);

static inline pio_sm_config pio_get_default_sm_config(void) { pio_sm_config c = {0, 0, 0, 0}; return c; }
static inline void sm_config_set_wrap(pio_sm_config *c, uint a, uint b) { (void)c; (void)a; (void)b; }
static inline void sm_config_set_sideset(pio_sm_config *c, uint n, bool o, bool p) { (void)c; (void)n; (void)o; (void)p; }
static inline void sm_config_set_sideset_pins(pio_sm_config *c, uint p) { (void)c; (void)p; }
static inline void sm_config_set_in_pins(pio_sm_config *c, uint p) { (void)c; (void)p; }
static inline void sm_config_set_jmp_pin(pio_sm_config *c, uint p) { (void)c; (void)p; }
static inline void sm_config_set_in_shift(pio_sm_config *c, bool d, bool a, uint t) { (void)c; (void)d; (void)a; (void)t; }
static inline void sm_config_set_out_shift(pio_sm_config *c, bool d, bool a, uint t) { (void)c; (void)d; (void)a; (void)t; }
static inline void sm_config_set_fifo_join(pio_sm_config *c, enum pio_fifo_join j) { (void)c; (void)j; }
static inline void sm_config_set_clkdiv(pio_sm_config *c, float d) { (void)c; (void)d; }

static inline uint pio_encode_jmp(uint a) { return a; }
static inline uint pio_encode_set(enum pio_src_dest d, uint v) { return ((uint)d << 5) | v; }
static inline uint pio_encode_sideset(uint n, uint v) { return v << (13 - n); }
static inline uint pio_encode_delay(uint c) { return c << 8; }
static inline uint pio_encode_irq_clear(bool rel, uint i) { return 0xc040u | i | (rel ? 0x10u : 0u); }
static inline uint pio_encode_mov_not(enum pio_src_dest d, enum pio_src_dest s) { return 0xa008u | ((uint)d << 5) | (uint)s; }

void gpio_set_mask(uint32_t m);
void gpio_clr_mask(uint32_t m);
bool gpio_get(uint pin);
static inline void gpio_pull_down(uint pin) { (void)pin; }
static inline void gpio_disable_pulls(uint pin) { (void)pin; }
static inline void gpio_set_inover(uint pin, uint v) { (void)pin; (void)v; }
static inline void gpio_set_outover(uint pin, uint v) { (void)pin; (void)v; }
static inline void gpio_set_oeover(uint pin, uint v) { (void)pin; (void)v; }
static inline void gpio_set_slew_rate(uint pin, enum gpio_slew_rate s) { (void)pin; (void)s; }
static inline void gpio_set_drive_strength(uint pin, enum gpio_drive_strength s) { (void)pin; (void)s; }

static inline void dma_claim_mask(uint32_t m) { (void)m; }
static inline dma_channel_config dma_channel_get_default_config(uint ch) { (void)ch; dma_channel_config c = {0}; return c; }
static inline void channel_config_set_read_increment(dma_channel_config *c, bool v) { (void)c; (void)v; }
static inline void channel_config_set_write_increment(dma_channel_config *c, bool v) { (void)c; (void)v; }
static inline void channel_config_set_transfer_data_size(dma_channel_config *c, enum dma_channel_transfer_size s) { (void)c; (void)s; }
static inline void channel_config_set_dreq(dma_channel_config *c, uint d) { (void)c; (void)d; }
static inline void dma_channel_set_config(uint ch, const dma_channel_config *c, bool t) { (void)ch; (void)c; (void)t; }
static inline void dma_channel_set_write_addr(uint ch, volatile void *a, bool t) { (void)ch; (void)a; (void)t; }
static inline void dma_channel_transfer_from_buffer_now(uint ch, const void *p, uint32_t n) { (void)ch; (void)p; (void)n; }
static inline bool dma_channel_is_busy(uint ch) { (void)ch; return false; }

typedef void (*irq_handler_t)(void);
void irq_set_exclusive_handler(uint num, irq_handler_t h);
static inline void irq_set_enabled(uint num, bool en) { (void)num; (void)en; }
static inline void irq_clear(uint num) { (void)num; }
extern irq_handler_t mock_irq_handler;

static inline uint32_t save_and_disable_interrupts(void) { return 0; }
static inline void restore_interrupts(uint32_t s) { (void)s; }

uint32_t clock_get_hz(enum clock_index c);
typedef struct { uint32_t timerawl; } mock_timer_hw_t;
mock_timer_hw_t *mock_timer(void);
#define PICO_DEFAULT_TIMER_INSTANCE() mock_timer()
static inline void busy_wait_us_32(uint32_t us) { (void)us; }
static inline void busy_wait_us(uint64_t us) { (void)us; }
static inline void busy_wait_ms(uint32_t ms) { (void)ms; }
static inline void busy_wait_1_us(void) {}
static inline void busy_wait_at_least_cycles(uint32_t c) { (void)c; }
static inline void tight_loop_contents(void) {}

typedef struct { int unused; } alarm_pool_t;
typedef struct { int unused; } repeating_timer_t;
typedef bool (*repeating_timer_callback_t)(repeating_timer_t *);

/* harness-facing model of the PIO receiver */
void mock_rx_reset(void);
void mock_rx_flush(void);
void mock_rx_add_packet(const uint8_t *data, size_t len);
extern unsigned mock_se0_polls;

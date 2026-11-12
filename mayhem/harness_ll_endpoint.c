#include "mocksdk.h"
#include "pio_usb.h"
#include "pio_usb_ll.h"
#include <stdlib.h>

/* Input: 7-byte endpoint descriptor body, then ops: [op][arg lo][arg hi]. */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size < 7 + 3) return -1;

  memset(pio_usb_ep_pool, 0, sizeof(pio_usb_ep_pool));
  memset(pio_usb_root_port, 0, sizeof(pio_usb_root_port));

  uint8_t desc[7] = {7, DESC_TYPE_ENDPOINT, 0, 0, 0, 0, 0};
  memcpy(desc + 2, data, 5);
#ifndef LL_ENDPOINT_ALLOW_OVERSIZE
  /* Library precondition: packets are at most PIO_USB_EP_SIZE (64) bytes (ep->buffer and prepare_tx_data's
   * stack buffer are sized for it; USB 2.0 FS/LS limit). Larger overflows: -DLL_ENDPOINT_ALLOW_OVERSIZE. */
  if ((desc[4] | (desc[5] << 8)) > PIO_USB_EP_SIZE) return -1;
#endif
  endpoint_t *ep = PIO_USB_ENDPOINT(1 + (data[0] & 0x0f));
  pio_usb_ll_configure_endpoint(ep, desc);
  ep->is_tx = (desc[2] & 0x80) != 0;

  uint8_t *app = NULL;
  size_t i = 7;
  for (; i + 3 <= size; i += 3) {
    uint16_t arg = (uint16_t)(data[i + 1] | (data[i + 2] << 8));
    switch (data[i] & 3) {
      case 0: {
        if (ep->has_transfer) break;
        free(app);
        uint16_t len = arg & 0x3ff;
        app = (uint8_t *)malloc(len ? len : 1);
        if (app == NULL) return 0;
        memset(app, data[i + 1], len);
        pio_usb_ll_transfer_start(ep, app, len);
        break;
      }
      case 1: {
        if (!ep->has_transfer) break;
        uint16_t xact = pio_usb_ll_get_transaction_len(ep);
        pio_usb_ll_transfer_continue(ep, (uint16_t)(arg % (xact + 1u)));
        break;
      }
      case 2:
        if (!ep->has_transfer) break;
        pio_usb_ll_transfer_complete(ep, (uint32_t[]){PIO_USB_INTS_ENDPOINT_COMPLETE_BITS, PIO_USB_INTS_ENDPOINT_ERROR_BITS, PIO_USB_INTS_ENDPOINT_STALLED_BITS, 0}[arg & 3]);
        break;
      default:
        ep->is_tx = !ep->is_tx;
        break;
    }
  }
  free(app);
  return 0;
}

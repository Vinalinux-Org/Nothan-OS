#ifndef _NOTHAN_USB_CPPI41_H
#define _NOTHAN_USB_CPPI41_H

/*
 * drivers/usb/cppi41.h - the USB subsystem's DMA engine, as much of it as
 * anything outside needs
 *
 * Deliberately small.  The engine has queues, a scheduler, a descriptor
 * region, teardown and completion interrupts, and none of that is any of the
 * host controller's business: what musb-hcd needs is somewhere to say "fill
 * this buffer from that endpoint" and a way to hear that it happened.  Keeping
 * the surface here narrow is what stops CPPI's vocabulary leaking into a file
 * that already knows too much.
 */

#include <nothan/types.h>

/*
 * The queues the camera's endpoint uses.
 *
 * Fixed by the silicon, not chosen: each USB endpoint has a hardwired pair of
 * queues and a hardwired channel number, listed in the vendor driver's
 * am335x_usb_queues_rx[] table.  USB1 endpoint 1 receive is channel 15,
 * submitting on queue 16 and completing on queue 141.  Picking different
 * numbers does not move the endpoint; it just addresses a queue nothing is
 * connected to.
 */
#define CPPI_CH_USB1_EP1_RX		15
#define CPPI_Q_USB1_EP1_RX_SUBMIT	16
#define CPPI_Q_USB1_EP1_RX_DONE		141

/*
 * Bring up the queue manager and prove it works.  @va is the translated USB
 * subsystem base — this file maps nothing, because a mapping is board
 * knowledge.  Returns 0 if a descriptor could be pushed onto a queue and taken
 * back again, which is the one question stage 1 exists to answer.
 */
int cppi_init(u32 va);

/* One descriptor out of the pool, by index, either way round. */
void *cppi_desc(unsigned int i);
u32 cppi_desc_phys(unsigned int i);

void cppi_push(unsigned int queue, u32 desc_phys);
u32  cppi_pop(unsigned int queue);

#endif /* _NOTHAN_USB_CPPI41_H */

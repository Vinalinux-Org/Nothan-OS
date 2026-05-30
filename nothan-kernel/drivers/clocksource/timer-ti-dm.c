#include <nothan/types.h>
#include <nothan/irq.h>
#include <nothan/mmio.h>

/*
 * DMTimer2 at PA 0x48040000 (L4_PER), VA 0xF0040000.
 * Clock: 24 MHz M_OSC crystal. IRQ: 68.
 */
#define DMTIMER2_BASE		0xF0040000
#define DMTIMER2_IRQ		68

#define TCLR			0x38
#define TCRR			0x3C
#define TLDR			0x40
#define TWPS			0x48
#define IRQENABLE_SET		0x2C
#define IRQSTATUS		0x28

#define TCLR_AR			(1 << 1)
#define TCLR_ST			(1 << 0)
#define IRQ_OVF_IT_FLAG		(1 << 1)
#define TWPS_W_PEND_TLDR	(1 << 2)
#define TWPS_W_PEND_TCRR	(1 << 1)

static volatile unsigned long jiffies;

static void timer_irq_handler(unsigned int irq)
{
	(void)irq;

	mmio_write32(DMTIMER2_BASE + IRQSTATUS, IRQ_OVF_IT_FLAG);
	jiffies++;
}

unsigned long get_jiffies(void)
{
	return jiffies;
}

/**
 * timer_init - initialise DMTimer2 as a 10 ms scheduler tick
 *
 * Sources the 24 MHz M_OSC crystal, auto-reloads every 10 ms
 * (240 000 cycles), and fires the overflow interrupt on IRQ 68.
 */
void timer_init(void)
{
	/* 10 ms @ 24 MHz: reload = 0xFFFFFFFF - (24000000/1000)*10 + 1 */
	u32 reload = 0xFFFFFFFF - 240000 + 1;

	mmio_write32(DMTIMER2_BASE + TLDR, reload);
	while (mmio_read32(DMTIMER2_BASE + TWPS) & TWPS_W_PEND_TLDR)
		;

	mmio_write32(DMTIMER2_BASE + TCRR, reload);
	while (mmio_read32(DMTIMER2_BASE + TWPS) & TWPS_W_PEND_TCRR)
		;

	mmio_write32(DMTIMER2_BASE + IRQENABLE_SET, IRQ_OVF_IT_FLAG);

	request_irq(DMTIMER2_IRQ, timer_irq_handler);
	intc_enable_irq(DMTIMER2_IRQ);

	mmio_write32(DMTIMER2_BASE + TCLR, TCLR_AR | TCLR_ST);
}

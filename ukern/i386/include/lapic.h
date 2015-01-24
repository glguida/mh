#ifndef _i386_lapic_h
#define _i386_lapic_h

#define APIC_DLVR_FIX   0
#define APIC_DLVR_PRIO  1
#define APIC_DLVR_SMI   2
#define APIC_DLVR_NMI 	4
#define APIC_DLVR_INIT  5
#define APIC_DLVR_START 6
#define APIC_DLVR_EINT 	7


/*
 * Local APIC.
 */

/* LAPIC registers.  */
#define L_IDREG		0x20
#define L_VER		0x30
#define L_TSKPRIO	0x80
#define L_ARBPRIO	0x90
#define L_PROCPRIO	0xa0
#define L_EOI		0xb0
#define L_LOGDEST	0xd0
#define L_DESTFMT	0xe0
#define L_MISC		0xf0 /* Spurious vector */
#define L_ISR		0x100 /* 256 bit */
#define L_TMR		0x180 /* 256 bit */
#define L_IRR		0x200 /* 256 bit */
#define L_ERR		0x280
#define L_ICR_LO	0x300
#define L_ICR_HI	0x310
#define L_LVT_TIMER	0x320
#define L_LVT_THERM	0x330
#define L_LVT_PFMCNT	0x340
#define L_LVT_LINT(x)	(0x350 + (x * 0x10))
#define L_LVT_ERR	0x370
#define L_TMR_IC	0x380
#define L_TMR_CC	0x390
#define L_TMR_DIV	0x3e0

#define LAPIC_SIZE      (1UL << 12)

extern void *lapic_base;
void lapic_init(paddr_t base);

static inline uint16_t
lapic_read(unsigned reg)
{

    return *((uint16_t *)lapic_base + reg);
}

#define lapic_getcurrent(void)  (lapic_read(L_IDREG) >> 24)

#endif

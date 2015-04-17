#ifndef __OMAP4_HWRNG_H__
#define __OMAP4_HWRNG_H__

#include <sys/param.h>
#include <sys/types.h>
#include <sys/rman.h>

#define OMAP4_HWRNG_OUTPUT_L	(0x00)
#define OMAP4_HWRNG_OUTPUT_H	(0x04)
#define OMAP4_HWRNG_STATUS	(0x08)
#define OMAP4_HWRNG_INTMASK	(0x0C)
#define OMAP4_HWRNG_INTACK	(0x10)
#define OMAP4_HWRNG_CONTROL	(0x14)
#define OMAP4_HWRNG_CONFIG	(0x18)
#define OMAP4_HWRNG_ALARMCNT	(0x1c)
#define OMAP4_HWRNG_FROENABLE	(0x20)
#define OMAP4_HWRNG_FRODETUNE	(0x24)
#define OMAP4_HWRNG_ALARMMASK	(0x28)
#define OMAP4_HWRNG_ALARMSTOP	(0x2C)
#define OMAP4_HWRNG_REV		(0x1FE0)
#define OMAP4_HWRNG_SYSCONFIG	(0x1FE4)

struct omap_hwrng_softc {
	device_t	sc_dev;
	struct mtx	sc_mtx;
	struct resource	sc_irq_res;
	struct resource sc_mem_res;
};

#endif

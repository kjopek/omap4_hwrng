#ifndef __OMAP4_HWRNG_H__
#define __OMAP4_HWRNG_H__

#include <sys/param.h>
#include <sys/types.h>
#include <sys/rman.h>

/* Registers */

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

/* Masks, important constants */
#define OMAP4_HWRNG_DATA_SIZE		(0x08)
#define OMAP4_HWRNG_STATUS_READY	(0x01)
#define OMAP4_HWRNG_CONTROL_ENABLE_TRNG	(0x01 << 10)
#define OMAP4_HWRNG_INTACK_READY	(0x01)
#define OMAP4_HWRNG_INTACK_SHUTDOWN_OFLO (0x02)

#define OMAP4_HWRNG_CONFIG_MIN_REFIL_CYCLES (0x21)
#define OMAP4_HWRNG_CONFIG_MAX_REFIL_CYCLES (0x22)

#define OMAP4_HWRNG_ALARM_THRESHOLD	(0xFF)
#define OMAP4_HWRNG_SHUTDOWN_THRESHOLD	(0x04)

#define OMAP4_HWRNG_CONTROL_STARTUP_CYCLES (0xff)

#define HWRNG_READ(_sc, reg)	bus_read_4((_sc)->sc_mem_res, reg)
#define HWRNG_WRITE(_sc, reg, val) \
	bus_write_4((_sc)->sc_mem_res, reg, val)

struct omap4_hwrng_softc {
	device_t	sc_dev;
	struct mtx	sc_mtx;
	struct resource	*sc_irq_res;
	struct resource *sc_mem_res;
	void		*sc_intr_handler;
};

#endif

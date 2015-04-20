#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/types.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/random.h>
#include <sys/selinfo.h>
#include <sys/systm.h>
#include <sys/random.h>

#include <dev/fdt/fdt_common.h>
#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_bus.h>
#include <dev/ofw/ofw_bus_subr.h>

#include <machine/bus.h>
#include <machine/fdt.h>

#include "omap4_hwrng.h"

// TODO: Add RANDOM_PURE_OMAP4 to /sys/random.h

static struct omap4_hwrng_softc *softc = NULL;
static devclass_t omap4_hwrng_devclass;

static int inline
omap4_hwrng_data_ready(struct omap4_hwrng_softc *sc)
{
	int ret;

	ret = HWRNG_READ(sc, OMAP4_HWRNG_STATUS) & OMAP4_HWRNG_STATUS_READY;
	return (ret);
}

static uint64_t inline
omap4_hwrng_get_data(struct omap4_hwrng_softc *sc)
{
	uint64_t ret;
	uint32_t reg0, reg1;

	reg0 = HWRNG_READ(sc, OMAP4_HWRNG_OUTPUT_H);
	reg1 = HWRNG_READ(sc, OMAP4_HWRNG_OUTPUT_L);

	ret = ((uint64_t)reg0 << 32) | reg1;

	mtx_lock(&(sc->sc_mtx));
	HWRNG_WRITE(sc, OMAP4_HWRNG_INTACK, OMAP4_HWRNG_INTACK_READY);
	mtx_unlock(&(sc->sc_mtx));

	return (ret);
}

static int
omap4_hwrng_init(struct omap4_hwrng_softc *sc)
{
	uint32_t config, threshold, control;

	if (HWRNG_READ(sc, OMAP4_HWRNG_CONTROL) &
	    OMAP4_HWRNG_CONTROL_ENABLE_TRNG)
		return (0);
	
	config = OMAP4_HWRNG_CONFIG_MIN_REFIL_CYCLES;
	config |= OMAP4_HWRNG_CONFIG_MAX_REFIL_CYCLES << 16;

	threshold = OMAP4_HWRNG_ALARM_THRESHOLD;
	threshold |= OMAP4_HWRNG_SHUTDOWN_THRESHOLD << 16;

	control = OMAP4_HWRNG_CONTROL_STARTUP_CYCLES << 16;
	control |= OMAP4_HWRNG_CONTROL_ENABLE_TRNG;

	mtx_lock(&(sc->sc_mtx));
	HWRNG_WRITE(sc, OMAP4_HWRNG_CONFIG, config);
	HWRNG_WRITE(sc, OMAP4_HWRNG_FRODETUNE, 0);
	HWRNG_WRITE(sc, OMAP4_HWRNG_FROENABLE, 0xffffff);
	HWRNG_WRITE(sc, OMAP4_HWRNG_ALARMCNT, threshold);
	HWRNG_WRITE(sc, OMAP4_HWRNG_CONTROL, control);
	mtx_unlock(&(sc->sc_mtx));

	return (0);
}

static void
omap4_hwrng_stop(struct omap4_hwrng_softc *sc)
{
	uint32_t val;

	val = HWRNG_READ(sc, OMAP4_HWRNG_CONTROL);
	val &= ~OMAP4_HWRNG_CONTROL_ENABLE_TRNG;
	mtx_lock(&(sc->sc_mtx));
	/* Driver for Linux writes value to CONFIG register
	 * maybe it is broken? Needs tests */
	HWRNG_WRITE(sc, OMAP4_HWRNG_CONTROL, val);
	mtx_unlock(&(sc->sc_mtx));
}

static void
omap4_hwrng_intr(void *arg)
{
	uint32_t tmp;

	tmp = ~HWRNG_READ(softc, OMAP4_HWRNG_FROENABLE) & 0xffffff;
	tmp |= HWRNG_READ(softc, OMAP4_HWRNG_FRODETUNE);

	mtx_lock(&(softc->sc_mtx));
	HWRNG_WRITE(softc, OMAP4_HWRNG_ALARMMASK, 0);
	HWRNG_WRITE(softc, OMAP4_HWRNG_ALARMSTOP, 0);

	HWRNG_WRITE(softc, OMAP4_HWRNG_FRODETUNE, tmp);
	HWRNG_WRITE(softc, OMAP4_HWRNG_FROENABLE, 0xffffff);

	HWRNG_WRITE(softc, OMAP4_HWRNG_INTACK, OMAP4_HWRNG_INTACK_SHUTDOWN_OFLO);
	mtx_unlock(&(softc->sc_mtx));
}

static void
omap4_hwrng_harvest(void *arg)
{
	struct omap4_hwrng_softc *sc;
	uint64_t tmp;

	sc = arg;
	while (!omap4_hwrng_data_ready(sc)) {
		DELAY(10);
	}

	tmp = omap4_hwrng_get_data(sc);
	random_harvest(&tmp, sizeof(tmp), sizeof(tmp)*8, RANDOM_PURE_OMAP4);
	callout_reset(&sc->sc_callout, hz * 5, omap4_hwrng_harvest, sc);
}

static int
omap4_hwrng_probe(device_t dev)
{
	if (!ofw_bus_status_okay(dev))
		return (ENXIO);

	if (!ofw_bus_is_compatible(dev, "ti,omap4-rng"))
		return (ENXIO);

	device_set_desc(dev, "OMAP4 Hardware RNG");

	return (BUS_PROBE_DEFAULT);
}

static int
omap4_hwrng_attach(device_t dev)
{
	int rid;
	struct omap4_hwrng_softc *sc;

	if (softc != NULL) {
		return (ENXIO);
	}

	sc = softc = device_get_softc(dev);
	sc->sc_dev = dev;
	rid = 0;
	sc->sc_mem_res = bus_alloc_resource_any(dev, SYS_RES_MEMORY, &rid,
	    RF_ACTIVE);
	if (!sc->sc_mem_res) {
		device_printf(dev, "cannot allocate memory resources\n");
		return (ENXIO);
	}

	rid = 0;
	sc->sc_irq_res = bus_alloc_resource_any(dev, SYS_RES_IRQ, &rid,
	    RF_ACTIVE);
	if (!sc->sc_irq_res) {
		device_printf(dev, "cannot allocate interrupt resources\n");
		bus_release_resource(dev, SYS_RES_MEMORY, 0, sc->sc_mem_res);
		return (ENXIO);
	}

	if (bus_setup_intr(dev, sc->sc_irq_res, INTR_MPSAFE, NULL,
		   omap4_hwrng_intr, sc, &sc->sc_intr_handler) != 0) {
		bus_release_resource(dev, SYS_RES_IRQ, 0, sc->sc_irq_res);
		bus_release_resource(dev, SYS_RES_MEMORY, 0, sc->sc_irq_res);
		device_printf(dev, "cannot setup intr handler");
		return (ENXIO);
	}

	mtx_init(&(sc->sc_mtx), device_get_nameunit(dev), "omap4_hwrng",
		    MTX_DEF);

	omap4_hwrng_init(sc);
	callout_init(&sc->sc_callout, CALLOUT_MPSAFE);
	callout_reset(&sc->sc_callout, hz*5, omap4_hwrng_harvest, sc);
	return (0);
}

static int
omap4_hwrng_detach(device_t dev)
{
	struct omap4_hwrng_softc *sc;

	sc = device_get_softc(dev);

	callout_drain(&sc->sc_callout);

	omap4_hwrng_stop(sc);
	mtx_destroy(&(sc->sc_mtx));

	if (sc->sc_intr_handler)
		bus_teardown_intr(dev, sc->sc_irq_res, sc->sc_intr_handler);
	if (sc->sc_irq_res)
		bus_release_resource(dev, SYS_RES_IRQ, 0, sc->sc_irq_res);
	if (sc->sc_mem_res)
		bus_release_resource(dev, SYS_RES_MEMORY, 0, sc->sc_mem_res);

	return (bus_generic_detach(dev));
}

static int
omap4_hwrng_suspend(device_t dev)
{
	struct omap4_hwrng_softc *sc;

	sc = device_get_softc(dev);
	omap4_hwrng_stop(sc);
	return (0);
}

static int
omap4_hwrng_resume(device_t dev)
{
	struct omap4_hwrng_softc *sc;

	sc = device_get_softc(dev);
	return (omap4_hwrng_init(sc));
}


static device_method_t omap4_hwrng_methods[] = {
	DEVMETHOD(device_probe,		omap4_hwrng_probe),
	DEVMETHOD(device_attach,	omap4_hwrng_attach),
	DEVMETHOD(device_detach,	omap4_hwrng_detach),
	DEVMETHOD(device_suspend,	omap4_hwrng_suspend),
	DEVMETHOD(device_resume,	omap4_hwrng_resume),
	{0, 0}
};

static driver_t omap4_hwrng_driver = {
	"omap4_hwrng",
	omap4_hwrng_methods,
	sizeof(struct omap4_hwrng_softc)
};

DRIVER_MODULE(omap4_hwrng, simplebus, omap4_hwrng_driver, omap4_hwrng_devclass, 0, 0);
MODULE_VERSION(omap4_hwrng, 1);
MODULE_DEPEND(omap4_hwrng, simplebus, 1, 1, 1);
MODULE_DEPEND(omap4_hwrng, random, 1, 1, 1);

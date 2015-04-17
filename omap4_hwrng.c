#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/random.h>
#include <sys/selinfo.h>
#include <sys/systm.h>

#include <dev/random/randomdev.h>
#include <dev/random/randomdev_soft.h>
#include <dev/random/random_harvestq.h>
#include <dev/random/live_entropy_sources.h>
#include <dev/random/random_adaptors.h>

#include <dev/fdt/fdt_common.h>
#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_bus.h>
#include <dev/ofw/ofw_bus_subr.h>

#include <machine/bus.h>
#include <machine/fdt.h>

#include "omap4_hwrng.h"

// TODO: Add RANDOM_PURE_OMAP4 to /sys/random.h

static struct random_hardware_source random_omap4_rng = {
	.ident = "OMAP4 Hardware RNG",
	.source = RANDOM_PURE_OMAP4,
	.read = omap4_hwrng_read
}; 

static struct omap4_hwrng_softc *softc = NULL;
static devclass_t omap4_hwrng_devclass;

static int
omap4_hwrng_init(struct omap4_hwrng_softc *sc)
{

	return (0);	
}

static void
omap4_hwrng_stop(struct omap4_hwrng_softc *sc)
{

}

static void
omap4_hwrng_intr(void *arg)
{
	struct omap4_hwrng_softc *sc;

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

	if (bus_setup_intr(dev, sc->sc_irq_res, INTR_TYPE_MPSAFE, NULL,
			   omap4_hwrng_intr, sc, &sc->sc_intr_handler) != 0) {
		bus_release_resource(dev, SYS_RES_IRQ, 0, sc->sc_irq_res);
		bus_release_resource(dev, SYS_RES_MEMORY, 0, sc->sc_irq_res);
		device_printf(dev, "cannot setup intr handler");
		return (ENXIO);
	}

	omap4_hwrng_init(sc);

	return (0);
}

static int
omap4_hwrng_detach(device_t dev)
{
	struct omap4_hwrng_softc *sc;

	sc = device_get_softc(dev);
	omap4_hwrng_stop(sc);

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

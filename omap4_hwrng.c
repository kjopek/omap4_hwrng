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

static int
omap4_hwrng_init(void)
{

	return (0);	
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

	

}

static int
omap4_hwrng_modevent(module_t mod, int type, void *unused)
{
	int error = 0;
	switch (type) {
	case MOD_LOAD:
		error = omap4_hwrng_probe();
		if (!error) {
			live_entropy_source_register(&random_omap4_rng);
			omap4_hwrng_init();
		}
		break;

	case MOD_UNLOAD:
		live_entropy_source_deregister(&random_omap4_rng);
		break;

	default:
		break;
	}

	return (error);
}

LIVE_ENTROPY_SRC_MODULE(omap4_hwrng, omap4_hwrng_modevent, 1);

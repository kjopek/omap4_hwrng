KMOD = omap4_hwrng
SRCS = omap4_hwrng.c omap4_hwrng.h device_if.h bus_if.h ofw_bus_if.h

.include <bsd.kmod.mk>

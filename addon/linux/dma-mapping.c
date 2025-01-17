#include <linux/dma-mapping.h>
#include <linux/envdefs.h>

#define BUS_ADDRESS(addr)	(((addr) & ~0xC0000000) | GPU_MEM_BASE)

void *dmam_alloc_coherent (struct device *dev, size_t size, dma_addr_t *dma_handle, gfp_t gfp)
{
	if (dev->dma_mem.end-dev->dma_mem.start+1 < size)
	{
		return 0;
	}

	*dma_handle = BUS_ADDRESS (dev->dma_mem.start);		// physical to bus address

	return (void *) dev->dma_mem.start;
}

//
// vchiqdevice.cpp
//
#include <vc4/vchiq/vchiqdevice.h>
#include <circle/bcm2835.h>
#include <circle/bcm2835int.h>
#include <circle/sysconfig.h>

#include <linux/linuxemu.h>
#include <circle/util.h>
#include <assert.h>

int vchiq_probe (struct platform_device *pdev);

static inline void CVCHIQDevice_AddResource (
    CVCHIQDevice *_this, u32 nStart, u32 nEnd, unsigned nFlags)
{
    assert (nStart <= nEnd);
    assert (nFlags != 0);

    unsigned i = _this->m_PlatformDevice.num_resources++;
    assert (i < PLATFORM_DEVICE_MAX_RESOURCES);

    _this->m_PlatformDevice.resource[i].start = nStart;
    _this->m_PlatformDevice.resource[i].end = nEnd;
    _this->m_PlatformDevice.resource[i].flags = nFlags;
}

static inline void CVCHIQDevice_SetDMAMemory (
    CVCHIQDevice *_this, u32 nStart, u32 nEnd)
{
    assert (nStart <= nEnd);

    _this->m_PlatformDevice.dev.dma_mem.start = nStart;
    _this->m_PlatformDevice.dev.dma_mem.end = nEnd;
    _this->m_PlatformDevice.dev.dma_mem.flags = IORESOURCE_DMA;
}

#define COHERENT_SLOT_VCHIQ_START       (MEGABYTE / PAGE_SIZE / 2)
#define COHERENT_SLOT_VCHIQ_END         (MEGABYTE / PAGE_SIZE - 1)

u32 CMemorySystem_GetCoherentPage (unsigned nSlot)
{
    u32 nPageAddress = MEM_COHERENT_REGION;
    nPageAddress += nSlot * PAGE_SIZE;
    return nPageAddress;
}

boolean CVCHIQDevice_Initialize (CVCHIQDevice *_this)
{
    // CVCHIQDevice::CVCHIQDevice()
    memset (&_this->m_PlatformDevice, 0, sizeof _this->m_PlatformDevice);

    _this->m_PlatformDevice.num_resources = 0;

    CVCHIQDevice_AddResource (_this, ARM_VCHIQ_BASE, ARM_VCHIQ_END, IORESOURCE_MEM);
    CVCHIQDevice_AddResource (_this, ARM_IRQ_ARM_DOORBELL_0, ARM_IRQ_ARM_DOORBELL_0, IORESOURCE_IRQ);

    CVCHIQDevice_SetDMAMemory (_this,
        CMemorySystem_GetCoherentPage (COHERENT_SLOT_VCHIQ_START),
        CMemorySystem_GetCoherentPage (COHERENT_SLOT_VCHIQ_END) + PAGE_SIZE-1);

    // CVCHIQDevice::Initialize()
    if (linuxemu_init () != 0)
    {
        return FALSE;
    }

    return vchiq_probe (&_this->m_PlatformDevice) == 0 ? TRUE : FALSE;
}

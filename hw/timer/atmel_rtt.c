/*
 * AT91 Real-time Timer
 *
 * Copyright (c) 2009 Filip Navara
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to 
eal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
*
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
ROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * RT Timer
 *
 * Copyright (c) 2008 OK Labs
 * Copyright (c) 2011 NICTA Pty Ltd
 * Originally written by Hans Jiang
 * Updated by Peter Chubb
 *
 * This code is licensed under GPL version 2 or later.  See
 * the COPYING file in the top-level directory.
 *
 */

#include "hw/hw.h"
#include "qemu/timer.h"
#include "hw/ptimer.h"
#include "hw/sysbus.h"
//#include "hw/rtt.h"
//#include "hw/arm/imx.h"

//#define DEBUG_TIMER 1
#ifdef DEBUG_TIMER
#  define DPRINTF(fmt, args...) \
      do { printf("rtt_timer: " fmt , ##args); } while (0)
#else
#  define DPRINTF(fmt, args...) do {} while (0)
#endif

/*
 * Define to 1 for messages about attempts to
 * access unimplemented registers or similar.
 */
#define DEBUG_IMPLEMENTATION 1
#if DEBUG_IMPLEMENTATION
#  define IPRINTF(fmt, args...)                                         \
    do  { fprintf(stderr, "rtt_timer: " fmt, ##args); } while (0)
#else
#  define IPRINTF(fmt, args...) do {} while (0)
#endif

#define RTT_MR          0x00 
#define RTT_AR          0x04 
#define RTT_VR          0x08
#define RTT_SR          0x0c

#define MR_ALMIEN       0x10000
#define MR_RTTINCIEN    0x20000
#define MR_RTTRST       0x40000

#define SR_ALMS         0x01
#define SR_RTTINC       0x02

typedef struct {
  SysBusDevice busdev;
  MemoryRegion iomem;
  uint32_t	 mr; 	// Control Register
  uint32_t	 ar; 	// Mode Register
  uint32_t	 vr; 	// Receive Data Register
  uint32_t	 sr; 	// Status Register
  ptimer_state * timer;
  qemu_irq irq;
} rtt_state;

static const unsigned char rtt_id[8] =
  { 0x22, 0x10, 0x04, 0x00, 0x0d, 0xf0, 0x05, 0xb1 };

static void rtt_update(rtt_state *s)
{
  if(s->mr & MR_RTTRST){
    s->vr = 0;
    //s->mr &= ~MR_RTTRST;
  }
  if(s->vr == s->ar)
    s->sr |= SR_ALMS;
  
  if ((s->sr & SR_ALMS) || (s->sr & SR_RTTINC)) 
    qemu_irq_raise(s->irq);
  else 
    qemu_irq_lower(s->irq);
  
  //qemu_set_irq(s->irq, (s->sr & ((s->mr & 0x30000) >> 16)) != 0);
}

static void rtt_tick(void *opaque)
{
    rtt_state *s = opaque;

    s->vr++;
    s->sr |= SR_RTTINC;
    rtt_update(s);
}

static uint64_t rtt_read(void *opaque, hwaddr offset,
                           unsigned size)
{
    rtt_state *s = (rtt_state *)opaque;
    int val;

    if (offset >= 0xfe0 && offset < 0x1000) {
        return rtt_id[(offset - 0xfe0) >> 2];
    }
    switch (offset) {
     
    case 0x00: // MR 
      return s->mr;
    case 0x04: // AR 
      return s->ar;
    case 0x08: // VR 
      return s->vr;
    case 0x0c: // SR 
      val = s->sr;
      s->sr = 0;
      return val;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "rtt_read: Bad offset %x\n", (int)offset);
        return 0;
    }
}

static void rtt_write(void *opaque, hwaddr offset,
                        uint64_t value, unsigned size)
{
    rtt_state *s = (rtt_state *)opaque;

    switch (offset) {
    case 0x00: // MR
        s->mr = value;
	rtt_update(s);
        break;
    case 0x04: // AR
        s->ar = value;
	rtt_update(s);
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "rtt_write: Bad offset %x\n", (int)offset);
    }
}

static void rtt_reset(rtt_state *s)
{
  s->mr = 0x8000;
  s->ar = 0xffffffff;
  s->vr = 0;
  s->sr = 0;
}

static const MemoryRegionOps rtt_ops = {
    .read = rtt_read,
    .write = rtt_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const VMStateDescription vmstate_rtt = {
    .name = "rtt_ssp",
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields      = (VMStateField[]) {
        VMSTATE_UINT32(mr, rtt_state),
        VMSTATE_UINT32(ar, rtt_state),
        VMSTATE_UINT32(vr, rtt_state),
        VMSTATE_UINT32(sr, rtt_state),
        VMSTATE_PTIMER(timer,rtt_state),
        VMSTATE_END_OF_LIST()
    }
};

static int rtt_init(SysBusDevice *dev)
{
    rtt_state *s = FROM_SYSBUS(rtt_state, dev);
    QEMUBH *bh;

    memory_region_init_io(&s->iomem, OBJECT(s), &rtt_ops, s, "rtt", 0x20);
    sysbus_init_mmio(dev, &s->iomem);
    sysbus_init_irq(dev, &s->irq);
    //s->ssi = ssi_create_bus(&dev->qdev, "ssi");
    //rtt_reset(s);
    //vmstate_register(&dev->qdev, -1, &vmstate_rtt, s);
    bh = qemu_bh_new(rtt_tick, s);
    s->timer = ptimer_init(bh);

    ptimer_set_freq(s->timer, 10);
    ptimer_set_limit(s->timer, 1, 1);
    ptimer_run(s->timer, 0);

    return 0;

}

static void rtt_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc  = DEVICE_CLASS(klass);
    SysBusDeviceClass *sdc = SYS_BUS_DEVICE_CLASS(klass);

    sdc->init = rtt_init;
    dc->vmsd = &vmstate_rtt;
    dc->reset = rtt_reset;
    dc->desc = "rtt timer";
}

static const TypeInfo rtt_info = {
    .name          = "rtt",
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(rtt_state),
    .class_init    = rtt_class_init,
};

static void rtt_register_types(void)
{
    type_register_static(&rtt_info);
}

type_init(rtt_register_types)

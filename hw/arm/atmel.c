/*
 * Luminary Micro Atmel peripherals
 *
 * Copyright (c) 2006 CodeSourcery.
 * Written by Paul Brook
 *
 * This code is licensed under the GPL.
 */

#include "hw/sysbus.h"
#include "hw/ssi.h"
#include "hw/arm/arm.h"
#include "hw/devices.h"
#include "qemu/timer.h"
#include "hw/spi.h"
#include "net/net.h"
#include "hw/boards.h"
#include "exec/address-spaces.h"
#include "hw/ptimer.h"
//#include "qemu/qemu-timer.h"

#define GPIO_A 0
#define GPIO_B 1
#define GPIO_C 2

typedef const struct {
    const char *name;
  uint32_t chipid_cidr;
  uint32_t chipid_exid;
} atmel_board_info;


/* System controller.  */

typedef struct {
    MemoryRegion iomem;
  uint32_t actlr;
  uint32_t cpuid;
  uint32_t icsr;
  uint32_t vtor;
  uint32_t aircr;
  uint32_t scr;
  uint32_t ccr;
  uint32_t shcrs;
  qemu_irq irq;
  atmel_board_info *board;
} ssys_state;
/*
static void ssys_update(ssys_state *s)
{
  qemu_set_irq(s->irq, 1);//(s->int_status & s->int_mask) != 0);
}
*/
static const MemoryRegionOps ssys_ops = {
  //  .read = ssys_read,
  //  .write = ssys_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};
/*
static void ssys_reset(void *opaque)
{
    ssys_state *s = (ssys_state *)opaque;

    s->pborctl = 0x7ffd;
    s->rcc = 0x078e3ac0;

    if (ssys_board_class(s) == DID0_CLASS_SANDSTORM) {
        s->rcc2 = 0;
    } else {
        s->rcc2 = 0x07802810;
    }
    s->rcgc[0] = 1;
    s->scgc[0] = 1;
    s->dcgc[0] = 1;
    ssys_calculate_system_clock(s);
}
*/
static const VMStateDescription vmstate_atmel_sys = {
    .name = "atmel_sys",
    .version_id = 2,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields      = (VMStateField[]) {
        VMSTATE_UINT32(actlr, ssys_state),
        VMSTATE_UINT32(cpuid, ssys_state),
        VMSTATE_UINT32(icsr, ssys_state),
        VMSTATE_UINT32(vtor, ssys_state),
        VMSTATE_UINT32(aircr, ssys_state),
        VMSTATE_UINT32(scr, ssys_state),
        VMSTATE_UINT32(ccr, ssys_state),
        VMSTATE_UINT32(shcrs, ssys_state),
        VMSTATE_END_OF_LIST()
    }
};

static int atmel_sys_init(uint32_t base, qemu_irq irq,
                              atmel_board_info * board,
                              uint8_t *macaddr)
{
    ssys_state *s;

    s = (ssys_state *)g_malloc0(sizeof(ssys_state));
    s->irq = irq;
    s->board = board;

    memory_region_init_io(&s->iomem, OBJECT(s),&ssys_ops, s, "ssys", 0x00001000);
    memory_region_add_subregion(get_system_memory(), base, &s->iomem);
    //ssys_reset(s);
    vmstate_register(NULL, -1, &vmstate_atmel_sys, s);
    return 0;
}


/* Board init.  */
static atmel_board_info atmel_boards[] = {
  { "AT91SAM3S8",
    0x28AB0A60,
    0
  },
  { "AT91SAM3SD8",
    0x29AB0A60,
    0
  }
};

static void atmel_init(const char *kernel_filename, const char *cpu_model,
                           atmel_board_info *board)
{
  static const int uart_irq[] = {8, 9};
    static const int rtt_irq = 3;
    static const uint32_t gpio_addr[7] =
      { 0x400e0e00, 0x400e1000, 0x400e1200};
    static const int gpio_irq[3] = {11, 12, 13};

    MemoryRegion *address_space_mem = get_system_memory();
    qemu_irq *pic;
    DeviceState *gpio_dev[3];
    qemu_irq gpio_in[3][8];
    qemu_irq gpio_out[3][8];
    int sram_size;
    int flash_size;
    //spi_bus *spi;
    //i2c_bus *i2c;
    DeviceState *dev;
    int i;
    int j;

    flash_size = 512 * 1024;
    sram_size = 64;
    pic = armv7m_init(address_space_mem,
                      flash_size, sram_size, kernel_filename, cpu_model);

    atmel_sys_init(0x400e0740, pic[28], board, nd_table[0].macaddr.a);
    //GPIO
    for (i = 0; i < 3; i++) {
      gpio_dev[i] = sysbus_create_simple("pl061_luminary", gpio_addr[i],
					 pic[gpio_irq[i]]);
      for (j = 0; j < 8; j++) {
	gpio_in[i][j] = qdev_get_gpio_in(gpio_dev[i], j);
	gpio_out[i][j] = NULL;
      }
    }
    //UART
    for (i = 0; i < 2; i++) {
            sysbus_create_simple("pl011_luminary", 0x400e0600 + i * 0x200,
                                 pic[uart_irq[i]]);
    }
    
    //SPI
    dev = sysbus_create_simple("spi", 0x40008000, pic[21]);
    void *bus;
    DeviceState *sddev;
    DeviceState *ssddev;    
    bus = qdev_get_child_bus(dev, "ssi");    
    sddev = ssi_create_slave(bus, "ssi-sd");
    ssddev = ssi_create_slave(bus, "ssd0323");
    
    // Real Time Timer
    sysbus_create_simple("rtt", 0x400e1430, pic[rtt_irq]);

    // Power management controller
    sysbus_create_simple("pmc", 0x400e0400, pic[5]);

}

/* FIXME: Figure out how to generate these from atmel_boards.  */
static void at91sam3s8_init(QEMUMachineInitArgs *args)
{
    const char *cpu_model = args->cpu_model;
    const char *kernel_filename = args->kernel_filename;
    atmel_init(kernel_filename, cpu_model, &atmel_boards[0]);
}

static void at91sam3sd8_init(QEMUMachineInitArgs *args)
{
    const char *cpu_model = args->cpu_model;
    const char *kernel_filename = args->kernel_filename;
    atmel_init(kernel_filename, cpu_model, &atmel_boards[1]);
}

static QEMUMachine at91sam3s8_machine = {
    .name = "at91sam3s8",
    .desc = "Atmel AT91SAM3S8",
    .init = at91sam3s8_init,
    .max_cpus=2,
    DEFAULT_MACHINE_OPTIONS,
};

static QEMUMachine at91sam3sd8_machine = {
    .name = "at91sam3sd8",
    .desc = "Atmel AT91SAM3SD8",
    .init = at91sam3sd8_init,
    .max_cpus = 2,
    DEFAULT_MACHINE_OPTIONS,
};

static void atmel_machine_init(void)
{
    qemu_register_machine(&at91sam3s8_machine);
    qemu_register_machine(&at91sam3sd8_machine);
}

machine_init(atmel_machine_init);

static void atmel_register_types(void)
{
  //type_register_static(&atmel_i2c_info);
  //type_register_static(&atmel_gptm_info);
  //type_register_static(&atmel_adc_info);
  //type_register_static(&atmelRTT_info);
  //type_register_static(&at91_rtt_info);
}

type_init(atmel_register_types)

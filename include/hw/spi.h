/*
 * SPI interface.
 *
 * Copyright (C) 2007-2010 Nokia Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 or
 * (at your option) any later version of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#ifndef QEMU_SPI_H
#define QEMU_SPI_H

#include "hw/hw.h"
#include "hw/qdev.h"

/* pass to spi_set_cs to deslect all devices on bus */

#define SPI_BUS_NO_CS 0xFF

/* state of a SPI device,
 * SPI_NO_CS -> the CS pin in de-asserted -> device is tristated
 * SPI_IDLE -> CS is asserted and device ready to recv
 * SPI_DATA_PENDING -> CS is asserted and the device has pushed data to master
 */

typedef enum {
    SPI_NO_CS,
    SPI_IDLE,
    SPI_DATA_PENDING
} SpiSlaveState;

typedef struct SPISlave {
    DeviceState qdev;
    uint8_t cs;
} SPISlave;

#define TYPE_SPI_SLAVE "spi-slave"
#define SPI_SLAVE(obj) \
     OBJECT_CHECK(SPISlave, (obj), TYPE_SPI_SLAVE)
#define SPI_SLAVE_CLASS(klass) \
     OBJECT_CLASS_CHECK(SPISlaveClass, (klass), TYPE_SPI_SLAVE)
#define SPI_SLAVE_GET_CLASS(obj) \
     OBJECT_GET_CLASS(SPISlaveClass, (obj), TYPE_SPI_SLAVE)

typedef struct SPISlaveClass {
    DeviceClass parent_class;

    /* Callbacks provided by the device.  */
    int (*init)(SPISlave *s);

    /* change the cs pin state */
    void (*cs)(SPISlave *s, uint8_t select);

    /* Master to slave.  */
    SpiSlaveState (*send)(SPISlave *s, uint32_t data, int len);

    /* Slave to master.  */
    SpiSlaveState (*recv)(SPISlave *s, uint32_t *data);

    /* poll the spi device state */
    SpiSlaveState (*get_state)(SPISlave *s);
} SPISlaveClass;

#define SPI_SLAVE_FROM_QDEV(dev) DO_UPCAST(SPISlave, qdev, dev)
#define FROM_SPI_SLAVE(type, dev) DO_UPCAST(type, spi, dev)

extern const VMStateDescription vmstate_spi_slave;

#define VMSTATE_SPI_SLAVE(_field, _state) {                          \
    .name       = (stringify(_field)),                               \
    .size       = sizeof(SPISlave),                                  \
    .vmsd       = &vmstate_spi_slave,                                \
    .flags      = VMS_STRUCT,                                        \
    .offset     = vmstate_offset_value(_state, _field, SPISlave),    \
}

typedef struct spi_bus {
    BusState qbus;
    SPISlave **slaves;
    uint8_t num_slaves;
    uint8_t cur_slave;
} spi_bus;

/* create a new spi bus */
spi_bus *spi_init_bus(DeviceState *parent, int num_slaves, const char *name);
int spi_attach_slave(spi_bus *bus, SPISlave *s, int cs);

DeviceState *spi_create_slave(spi_bus *bus, const char *name, uint8_t addr);

/* change the chip select. Return 1 on failure. */
int spi_set_cs(spi_bus *bus, int cs);
int spi_get_cs(spi_bus *bus);
SpiSlaveState spi_get_state(spi_bus *bus);

SpiSlaveState spi_send(spi_bus *bus, uint32_t data, int len);
SpiSlaveState spi_recv(spi_bus *bus, uint32_t *data);
//void spi_end_transfer(spi_bus *bus);

#endif

/*
#ifndef HW_SPI_H__
#define HW_SPI_H__
#include "hw/hw.h"
#include "hw/qdev.h"

typedef struct spi_device_s SPIDevice;
typedef struct spi_bus_s SPIBus;
typedef int (*spi_device_initfn)(SPIDevice *dev);
typedef uint32_t (*spi_txrx_cb)(SPIDevice *dev, uint32_t, int);

typedef struct {
  // DeviceState qdev;
    DeviceInfo qdev;
    spi_device_initfn init;
    spi_txrx_cb txrx;
} SPIDeviceInfo;

struct spi_device_s {
    DeviceState qdev;
    SPIDeviceInfo *info;
    // internal fields used by SPI code 
    uint8_t channel;
};

SPIBus *spi_init_bus(DeviceState *parent, const char *name, int num_channels);
uint32_t spi_txrx(SPIBus *bus, int channel, uint32_t data, int len);

#define SPI_DEVICE_FROM_QDEV(dev) DO_UPCAST(SPIDevice, qdev, dev)
#define FROM_SPI_DEVICE(type, dev) DO_UPCAST(type, spi, dev)

void spi_register_device(SPIDeviceInfo *info);
DeviceState *spi_create_device(SPIBus *bus, const char *name, int ch);
DeviceState *spi_create_device_noinit(SPIBus *bus, const char *name, int ch);

#endif
*/

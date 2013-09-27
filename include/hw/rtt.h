/*
 * i.MX31 emulation
 *
 * Copyright (C) 2012 Peter Chubb
 * NICTA
 *
 * This code is released under the GPL, version 2.0 or later
 * See the file `../COPYING' for details.
 */

#ifndef RTT_H
#define RTT_H
#include "hw/arm/imx.h"
void rtt_serial_create(int uart, const hwaddr addr, qemu_irq irq);
/*
typedef enum  {
    NOCLK,
    MCU,
    HSP,
    IPG,
    CLK_32k
} RTTClk;
*/
typedef IMXClk RTTClk;
uint32_t rtt_clock_frequency(DeviceState *s, RTTClk clock);

void rtt_timerp_create(const hwaddr addr,
                      qemu_irq irq,
                      DeviceState *ccm);

#endif 

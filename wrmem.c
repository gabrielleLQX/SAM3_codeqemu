/*
 * QEMU System Emulator
 *
 * Copyright (c) 2003-2008 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
/*
 * splitted out wrmem related stuffs from vl.c.
 */

#include "exec/wrmem.h"
#include "trace.h"
#include "exec/memory.h"

/***********************************************************/
/* IO Port */

//#define DEBUG_UNUSED_WRMEM
//#define DEBUG_WRMEM

#ifdef DEBUG_UNUSED_WRMEM
#  define LOG_UNUSED_WRMEM(fmt, ...) fprintf(stderr, fmt, ## __VA_ARGS__)
#else
#  define LOG_UNUSED_WRMEM(fmt, ...) do{ } while (0)
#endif

#ifdef DEBUG_WRMEM
#  define LOG_WRMEM(...) qemu_log_mask(CPU_LOG_WRMEM, ## __VA_ARGS__)
#else
#  define LOG_WRMEM(...) do { } while (0)
#endif

/* XXX: use a two level table to limit memory usage */

static void *mr;
/*
static WrmemReadFunc *wrmem_read_table[3][MAX_WRMEMS];
static WrmemWriteFunc *wrmem_write_table[3][MAX_WRMEMS];
//static CPUDestructor *wrmem_destructor_table[MAX_WRMEMS];

static WrmemReadFunc default_wrmem_readb, default_wrmem_readw, default_wrmem_readl;
static WrmemWriteFunc default_wrmem_writeb, default_wrmem_writew, default_wrmem_writel;
*/
void writedbg_mem(uint32_t addr, uint32_t val)
{  
  MemoryRegion *memr=mr;
  LOG_WRMEM("write in: %x"=" %x" "\n", addr, val);
  if (memr->ops->write) {
    memr->ops->write(memr,addr-memr->addr,val,4);
    return;
  }
}

uint32_t readdbg_mem(uint32_t addr)
{  
  MemoryRegion *memr=mr;
  uint32_t val;
  if (memr->ops->read_dbg) {
    val = memr->ops->read_dbg(memr,addr-memr->addr,4);
    LOG_WRMEM("read from: %x" = " %x" "\n", addr, val);
    return val;
  }
}
/*
uint32_t wrmem_read(int index, uint32_t addr)
{
    static WrmemReadFunc * const default_func[3] = {
        default_wrmem_readb,
        default_wrmem_readw,
        default_wrmem_readl
    };
    WrmemReadFunc *func = wrmem_read_table[index][addr];
    if (!func)
        func = default_func[index];
    return func(wrmem_opaque[addr], addr);
}

void wrmem_write(int index, uint32_t addr, uint32_t val)
{
    LOG_WRMEM("outb: %04"FMT_pioaddr" %02"PRIx8"\n", addr, val);
    static WrmemWriteFunc * const default_func[3] = {
        default_wrmem_writeb,
        default_wrmem_writew,
        default_wrmem_writel
    };
    WrmemWriteFunc *func = wrmem_write_table[index][addr];
    if (!func)
        func = default_func[index];
    func(wrmem_opaque[addr], addr, val);
}

*/

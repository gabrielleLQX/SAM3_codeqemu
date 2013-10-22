/*
 * Arm SPI
 *
 * Copyright (c) 2007 CodeSourcery.
 * Written by Paul Brook
 *
 * This code is licensed under the GPL.
 */

#include "hw/sysbus.h"
#include "hw/ssi.h"

//#define DEBUG_PL022 1

#ifdef DEBUG_SPI
#define DPRINTF(fmt, ...) \
do { printf("spi: " fmt , ## __VA_ARGS__); } while (0)
#define BADF(fmt, ...) \
do { fprintf(stderr, "spi: error: " fmt , ## __VA_ARGS__); exit(1);} while (0)
#else
#define DPRINTF(fmt, ...) do {} while(0)
#define BADF(fmt, ...) \
do { fprintf(stderr, "spi: error: " fmt , ## __VA_ARGS__);} while (0)
#endif

// -------- SPI_CR : (SPI Offset: 0x0) SPI Control Register -------- 
#define  SPI_SPIEN       ((unsigned int) 0x1 <<  0) // (SPI) SPI Enable
#define  SPI_SPIDIS      ((unsigned int) 0x1 <<  1) // (SPI) SPI Disable
#define  SPI_SWRST       ((unsigned int) 0x1 <<  7) // (SPI) SPI Software reset
#define  SPI_LASTXFER    ((unsigned int) 0x1 << 24) // (SPI) SPI Last Transfer
// -------- SPI_MR : (SPI Offset: 0x4) SPI Mode Register -------- 
#define  SPI_MSTR        ((unsigned int) 0x1 <<  0) // (SPI) Master/Slave Mode
#define  SPI_PS          ((unsigned int) 0x1 <<  1) // (SPI) Peripheral Select
#define  SPI_PS_FIXED    ((unsigned int) 0x0 <<  1) // (SPI) Fixed Peripheral Select
#define  SPI_PS_VARIABLE ((unsigned int) 0x1 <<  1) // (SPI) Variable Peripheral Select
#define  SPI_PCSDEC      ((unsigned int) 0x1 <<  2) // (SPI) Chip Select Decode
#define  SPI_FDIV        ((unsigned int) 0x1 <<  3) // (SPI) Clock Selection
#define  SPI_MODFDIS     ((unsigned int) 0x1 <<  4) // (SPI) Mode Fault Detection
#define  SPI_LLB         ((unsigned int) 0x1 <<  7) // (SPI) Clock Selection
#define  SPI_PCS         ((unsigned int) 0xF << 16) // (SPI) Peripheral Chip Select
#define  SPI_DLYBCS      ((unsigned int) 0xFF << 24) // (SPI) Delay Between Chip Selects
// -------- SPI_RDR : (SPI Offset: 0x8) Receive Data Register -------- 
#define  SPI_RD          ((unsigned int) 0xFFFF <<  0) // (SPI) Receive Data
#define  SPI_RPCS        ((unsigned int) 0xF << 16) // (SPI) Peripheral Chip Select Status
// -------- SPI_TDR : (SPI Offset: 0xc) Transmit Data Register -------- 
#define  SPI_TD          ((unsigned int) 0xFFFF <<  0) // (SPI) Transmit Data
#define  SPI_TPCS        ((unsigned int) 0xF << 16) // (SPI) Peripheral Chip Select Status
// -------- SPI_SR : (SPI Offset: 0x10) Status Register -------- 
#define  SPI_RDRF        ((unsigned int) 0x1 <<  0) // (SPI) Receive Data Register Full
#define  SPI_TDRE        ((unsigned int) 0x1 <<  1) // (SPI) Transmit Data Register Empty
#define  SPI_MODF        ((unsigned int) 0x1 <<  2) // (SPI) Mode Fault Error
#define  SPI_OVRES       ((unsigned int) 0x1 <<  3) // (SPI) Overrun Error Status
#define  SPI_ENDRX       ((unsigned int) 0x1 <<  4) // (SPI) End of Receiver Transfer
#define  SPI_ENDTX       ((unsigned int) 0x1 <<  5) // (SPI) End of Transfer Transfer
#define  SPI_RXBUFF      ((unsigned int) 0x1 <<  6) // (SPI) RXBUFF 
#define  SPI_TXBUFE      ((unsigned int) 0x1 <<  7) // (SPI) TXBUFE 
#define  SPI_NSSR        ((unsigned int) 0x1 <<  8) // (SPI) NSSR 
#define  SPI_TXEMPTY     ((unsigned int) 0x1 <<  9) // (SPI) TXEMPTY 
#define  SPI_SPIENS      ((unsigned int) 0x1 << 16) // (SPI) Enable Status
// -------- SPI_IER : (SPI Offset: 0x14) Interrupt Enable Register -------- 
// -------- SPI_IDR : (SPI Offset: 0x18) Interrupt Disable Register -------- 
// -------- SPI_IMR : (SPI Offset: 0x1c) Interrupt Mask Register -------- 
// -------- SPI_CSR : (SPI Offset: 0x30) Chip Select Register -------- 
#define  SPI_CPOL        ((unsigned int) 0x1 <<  0) // (SPI) Clock Polarity
#define  SPI_NCPHA       ((unsigned int) 0x1 <<  1) // (SPI) Clock Phase
#define  SPI_CSAAT       ((unsigned int) 0x0 <<  3) // (SPI) Chip Select Active After Transfer
#define  SPI_BITS        ((unsigned int) 0xF <<  4) // (SPI) Bits Per Transfer
#define  SPI_BITS_8      ((unsigned int) 0x0 <<  4) // (SPI) 8 Bits Per transfer
#define  SPI_BITS_9      ((unsigned int) 0x1 <<  4) // (SPI) 9 Bits Per transfer
#define  SPI_BITS_10     ((unsigned int) 0x2 <<  4) // (SPI) 10 Bits Per transfer
#define  SPI_BITS_11     ((unsigned int) 0x3 <<  4) // (SPI) 11 Bits Per transfer
#define  SPI_BITS_12     ((unsigned int) 0x4 <<  4) // (SPI) 12 Bits Per transfer
#define  SPI_BITS_13     ((unsigned int) 0x5 <<  4) // (SPI) 13 Bits Per transfer
#define  SPI_BITS_14     ((unsigned int) 0x6 <<  4) // (SPI) 14 Bits Per transfer
#define  SPI_BITS_15     ((unsigned int) 0x7 <<  4) // (SPI) 15 Bits Per transfer
#define  SPI_BITS_16     ((unsigned int) 0x8 <<  4) // (SPI) 16 Bits Per transfer
#define  SPI_SCBR        ((unsigned int) 0xFF <<  8) // (SPI) Serial Clock Baud Rate
#define  SPI_DLYBS       ((unsigned int) 0xFF << 16) // (SPI) Delay Before SPCK
#define  SPI_DLYBCT      ((unsigned int) 0xFF << 24) // (SPI) Delay Between Consecutive Transfers

typedef struct {
  SysBusDevice busdev;
  MemoryRegion iomem;
  uint32_t	 cr; 	// Control Register
  uint32_t	 mr; 	// Mode Register
  uint32_t	 rdr; 	// Receive Data Register
  uint32_t	 tdr; 	// Transmit Data Register
  uint32_t	 sr; 	// Status Register
  uint32_t	 ier; 	// Interrupt Enable Register
  uint32_t	 idr; 	// Interrupt Disable Register
  uint32_t	 imr; 	// Interrupt Mask Register
  uint32_t	 csr[4]; 	// Chip Select Register
  //for at91sam3sd8
  uint32_t         wpmr;      //Write Protection Control Register
  uint32_t         wpsr;      //Write Protection Status Register
  
  // The FIFO head points to the next empty entry.  
  int tx_fifo_head;
  int rx_fifo_head;
  int tx_fifo_len;
  int rx_fifo_len;
  uint16_t tx_fifo[8];
  uint16_t rx_fifo[8];
  
  /*************************************************************
    For PDC_SPI
   *************************************************************/
  /*
  uint32_t	 rpr; 	// Receive Pointer Register
  uint32_t	 rcr; 	// Receive Counter Register
  uint32_t	 tpr; 	// Transmit Pointer Register
  uint32_t	 tcr; 	// Transmit Counter Register
  uint32_t	 rnpr; 	// Receive Next Pointer Register
  uint32_t	 rncr; 	// Receive Next Counter Register
  uint32_t	 tnpr; 	// Transmit Next Pointer Register
  uint32_t	 tncr; 	// Transmit Next Counter Register
  uint32_t	 ptcr; 	// PDC Transfer Control Register
  uint32_t	 ptsr; 	// PDC Transfer Status Register
  */
  bool dbg_s;
    qemu_irq irq;
    SSIBus *ssi;
} spi_state;

static const unsigned char spi_id[8] =
  { 0x22, 0x10, 0x04, 0x00, 0x0d, 0xf0, 0x05, 0xb1 };

static void spi_update(spi_state *s)
{
    fprintf(stderr,"\rin function : %s \r\n",__FUNCTION__);
    if (s->tx_fifo_len == 0)
      s->sr |= SPI_TXBUFE;
    else
      s->sr &= ~SPI_TXBUFE;

    if (s->rx_fifo_len == 8)
      s->sr |= SPI_RXBUFF;
    else
      s->sr &= ~SPI_RXBUFF;

    if (((s->cr & 0x1) == 1) && ((s->cr & 0x2) == 0))
      s->sr |= SPI_SPIENS;

    s->ier = 0;
    if (s->rx_fifo_len >= 4)
        s->ier |= SPI_RXBUFF;
    if (s->tx_fifo_len <= 4)
        s->ier |= SPI_TXBUFE;

    qemu_set_irq(s->irq, ((~(s->idr)) & s->ier & s->imr) != 0);
}


static void spi_transmit(spi_state *s, uint64_t value)
{  
    fprintf(stderr,"\rin function : %s \r\n",__FUNCTION__);
    int i;
    int o;
    int val;

    if (s->tx_fifo_len < 8) {
      DPRINTF("TX %02x\n", (unsigned)value);
      s->tx_fifo[s->tx_fifo_head] = value;
      s->sr &= ~SPI_TXEMPTY;
      s->sr &= ~SPI_TDRE;
      s->tx_fifo_head = (s->tx_fifo_head + 1) & 7;
      s->tx_fifo_len++;	    
      spi_update(s);
    }
    
    if ((s->cr & SPI_SPIEN) == 0) {
        spi_update(s);
        DPRINTF("Disabled\n");
        return;
    }
    DPRINTF("Maybe transmit %d/8\n", s->tx_fifo_len);
    i = (s->tx_fifo_head - s->tx_fifo_len) & 7;
    o = s->rx_fifo_len;
    if (s->tx_fifo_len && s->rx_fifo_len < 8) {
        DPRINTF("xfer\n");
        val = s->tx_fifo[i];
        
	  if (s->mr & SPI_LLB) {
	  // Loopback mode.
	    s->rdr = value;
	  } else {
	  val = ssi_transfer(s->ssi, val);
	  }
	
        i = (i + 1) & 7;
        s->tx_fifo_len--;
	s->rx_fifo[o] = val & 0xffff;
	s->sr |= SPI_RDRF;
        o = (o + 1) & 7;
        s->rx_fifo_len++;
    }
    s->rx_fifo_head = o;
    s->sr |= SPI_TXEMPTY;
    s->sr |= SPI_TDRE;
    spi_update(s);
}
static uint16_t spi_receive(spi_state *s)
{  
  fprintf(stderr,"\rin function : %s \r\n",__FUNCTION__);
  uint16_t val;
  if (s->mr & SPI_LLB) {
    // Loopback mode.
    val = s->rdr;
    s->sr &= ~SPI_RDRF;
    spi_update(s);
    s->tdr = 0x0;
    s->rdr = 0x0;
    return val;
  } else {
    if (s->rx_fifo_len > 0) {
      val = s->rx_fifo[(s->rx_fifo_head - s->rx_fifo_len) & 7];
      DPRINTF("RX %02x\n", val);
      s->rx_fifo_len--;
    } else {
      val = 0x0;
    }
    s->sr &= ~SPI_RDRF;
    spi_update(s);
    //s->tdr = 0x0;
    return val;
  }
} 
static void spi_xfer(spi_state *s,uint16_t value)
{  
  fprintf(stderr,"\rin function : %s \r\n",__FUNCTION__);
  s->sr &= ~SPI_TXEMPTY;
  s->sr &= ~SPI_TDRE;
  s->sr |= SPI_RDRF;
  s->rx_fifo[0] = value;
}

static void spi_dbgUpdate(void *opaque, hwaddr offset,
                           unsigned size)
{
  fprintf(stderr,"\rin function : %s \r\n",__FUNCTION__);
  spi_state *s = (spi_state *)opaque;
  if(s->dbg_s){
    s->dbg_s = 0;
  }
}

static uint32_t spi_read(void *opaque, hwaddr offset,
                           unsigned size)
{
    spi_state *s = (spi_state *)opaque;
    //int val;
    fprintf(stderr,"\rin function : %s \r\n",__FUNCTION__);
    if (offset >= 0xfe0 && offset < 0x1000) {
        return spi_id[(offset - 0xfe0) >> 2];
    }
    switch (offset) {
      
      //case 0x00: // CR 
      //return s->cr;
    case 0x04: // MR 
      return s->mr;
    case 0x08: // RDR 
      if((s->wpmr & 0x2) != 0){//mode debug     	  
	return s->rdr;
      }
      else
	return spi_receive(s);
      /******************************************
          Easy Mode when LLB is set
          and the write read is at the same speed
      ******************************************/
      /*
	if((s->mr & SPI_LLB)==0)
	return spi_receive(s);
	else
	{
	s->sr &= ~SPI_RDRF;
	s->sr |= SPI_TXEMPTY;
	s->sr |= SPI_TDRE;
	return val = s->rx_fifo[0];
	}
      */
    case 0x0c: //TDR
      if((s->wpmr & 0x2) != 0){
	return s->tdr;
      }
      else
	return 0xffffffff;
    case 0x10: // SR 
      return s->sr;
    case 0x1c: // IMR 
      return s->imr;
    case 0x30: // CSR0 
      return s->csr[0];
    case 0x34: // CSR1 
      return s->csr[1];
    case 0x38: // CSR2 
      return s->csr[2];
    case 0x3C: // CSR3 
      return s->csr[3];
    case 0xE4: // WPMR 
      return s->wpmr;
    case 0xE8: // WPSR 
      return s->wpsr;
    default:
      qemu_log_mask(LOG_GUEST_ERROR,
		    "spi_read: Bad offset %x\n", (int)offset);
      return 0;
    }
}

static uint32_t spi_read_dbg(void *opaque, hwaddr offset,
                           unsigned size)
{
  fprintf(stderr,"\rin function : %s \r\n",__FUNCTION__);
    spi_state *s = (spi_state *)opaque;
    //int val;

    if (offset >= 0xfe0 && offset < 0x1000) {
        return spi_id[(offset - 0xfe0) >> 2];
    }
    switch (offset) {
      
    case 0x00: // CR 
      return s->cr;
    case 0x04: // MR 
      return s->mr;
    case 0x08: // RDR   	  
	return s->rdr;
    case 0x0c: //TDR
	return s->tdr;
    case 0x10: // SR 
      return s->sr;
    case 0x14: // IER
      return s->ier; 
    case 0x18: // IDR
      return s->idr;  
    case 0x1c: // IMR 
      return s->imr;
    case 0x30: // CSR0 
      return s->csr[0];
    case 0x34: // CSR1 
      return s->csr[1];
    case 0x38: // CSR2 
      return s->csr[2];
    case 0x3C: // CSR3 
      return s->csr[3];
    case 0xE4: // WPMR 
      return s->wpmr;
    case 0xE8: // WPSR 
      return s->wpsr;
    default:
      qemu_log_mask(LOG_GUEST_ERROR,
		    "spi_read: Bad offset %x\n", (int)offset);
      return 0;
    }
}

static void spi_write(void *opaque, hwaddr offset,
                        uint64_t value, unsigned size)
{
  fprintf(stderr,"\rin function : %s \r\n",__FUNCTION__);
    spi_state *s = (spi_state *)opaque;

    switch (offset) {
    case 0x00: // CR
        s->cr = value;
	spi_update(s);
        break;
    case 0x04: // MR
        s->mr = value;
        if ((s->mr & SPI_MSTR)
                   == (~SPI_MSTR)) {
            BADF("SPI slave mode not implemented\n");
        }
        break;
    case 0x0c: // TDR 
	s->tdr = value;
      if((s->wpmr & 0x2) != 0x2)//tmp:(test)if not in mode debug
	if((s->sr & SPI_SPIENS) != 0)
	  spi_transmit(s,value);
	/******************************
          Easy Mode when LLB is set
          and the write read is at the same speed
	 ******************************/
	/*
	  if((s->mr & SPI_LLB)==0)
	  spi_transmit(s,value);
	  else
	  {
	  s->tx_fifo[0] = value;
	  spi_xfer(s,value);
	  }
	*/
      break;
    case 0x14: // IER 
        s->ier = value;
        spi_update(s);
        break;
    case 0x18: // IDR 
        s->idr = value;
        spi_update(s);
        break;
    case 0x30: // CSR0 
        s->csr[0] = value;
        break;
    case 0x34: // CSR1 
        s->csr[1] = value;
        break;
    case 0x38: // CSR2 
        s->csr[2] = value;
        break;
    case 0x3c: // CSR3 
        s->csr[3] = value;
        break;
    case 0xE4: // WPMR 
        s->wpmr = value;
	//if((value & 0x2) == 0)//not in mode debug
	  
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "spi_write: Bad offset %x\n", (int)offset);
    }
}

static void spi_write_dbg(void *opaque, hwaddr offset,
                        uint64_t value, unsigned size)
{
  fprintf(stderr,"\rin function : %s \r\n",__FUNCTION__);
    spi_state *s = (spi_state *)opaque;
    s->dbg_s = 1;
    switch (offset) {
    case 0x00: // CR
        s->cr = value;
	//spi_update(s);
        break;
    case 0x04: // MR
        s->mr = value;
        break;
    case 0x08: // RDR 
      s->rdr = value;
      break;
    case 0x0c: // TDR 
      s->tdr = value;
      break;
    case 0x14: // IER 
        s->ier = value;
        //spi_update(s);
        break;
    case 0x18: // IDR 
        s->idr = value;
        //spi_update(s);
        break;
    case 0x1c: // IMR 
        s->imr = value;
    case 0x30: // CSR0 
        s->csr[0] = value;
        break;
    case 0x34: // CSR1 
        s->csr[1] = value;
        break;
    case 0x38: // CSR2 
        s->csr[2] = value;
        break;
    case 0x3c: // CSR3 
        s->csr[3] = value;
        break;
    case 0xE4: // WPMR 
        s->wpmr = value;
	//if((value & 0x2) == 0)//not in mode debug
	  
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "spi_write: Bad offset %x\n", (int)offset);
    }
}

static void spi_reset(spi_state *s)
{
  fprintf(stderr,"\rin function : %s \r\n",__FUNCTION__);
    s->rx_fifo_len = 0;
    s->tx_fifo_len = 0;
    s->imr = 0x7ff;
    s->ier = 0;
    s->sr |= SPI_TXEMPTY | SPI_TXBUFE | SPI_RXBUFF;// | SPI_ENDTX | SPI_ENDRX;
    s->cr = 0;
}

static const MemoryRegionOps spi_ops = {
    .read = spi_read,
    .read_dbg = spi_read_dbg,
    .write = spi_write,
    .write_dbg = spi_write_dbg,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const VMStateDescription vmstate_spi = {
    .name = "spi_ssp",
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields      = (VMStateField[]) {
        VMSTATE_UINT32(cr, spi_state),
        VMSTATE_UINT32(mr, spi_state),
        VMSTATE_UINT32(rdr, spi_state),
        VMSTATE_UINT32(tdr, spi_state),
        VMSTATE_UINT32(sr, spi_state),
        VMSTATE_UINT32(ier, spi_state),
        VMSTATE_UINT32(idr, spi_state),
        VMSTATE_UINT32(imr, spi_state),
        VMSTATE_UINT32(csr[0], spi_state),
        VMSTATE_UINT32(csr[1], spi_state),
        VMSTATE_UINT32(csr[2], spi_state),
        VMSTATE_UINT32(csr[3], spi_state),
        VMSTATE_UINT32(wpmr, spi_state),
        VMSTATE_UINT32(wpsr, spi_state),
        VMSTATE_INT32(tx_fifo_head, spi_state),
        VMSTATE_INT32(rx_fifo_head, spi_state),
        VMSTATE_INT32(tx_fifo_len, spi_state),
        VMSTATE_INT32(rx_fifo_len, spi_state),
        VMSTATE_UINT16(tx_fifo[0], spi_state),
        VMSTATE_UINT16(rx_fifo[0], spi_state),
        VMSTATE_UINT16(tx_fifo[1], spi_state),
        VMSTATE_UINT16(rx_fifo[1], spi_state),
        VMSTATE_UINT16(tx_fifo[2], spi_state),
        VMSTATE_UINT16(rx_fifo[2], spi_state),
        VMSTATE_UINT16(tx_fifo[3], spi_state),
        VMSTATE_UINT16(rx_fifo[3], spi_state),
        VMSTATE_UINT16(tx_fifo[4], spi_state),
        VMSTATE_UINT16(rx_fifo[4], spi_state),
        VMSTATE_UINT16(tx_fifo[5], spi_state),
        VMSTATE_UINT16(rx_fifo[5], spi_state),
        VMSTATE_UINT16(tx_fifo[6], spi_state),
        VMSTATE_UINT16(rx_fifo[6], spi_state),
        VMSTATE_UINT16(tx_fifo[7], spi_state),
        VMSTATE_UINT16(rx_fifo[7], spi_state),
        VMSTATE_END_OF_LIST()
    }
};

static int spi_init(SysBusDevice *dev)
{
  fprintf(stderr,"\rin function : %s \r\n",__FUNCTION__);
    DeviceState *device = DEVICE(dev);
    spi_state *s = FROM_SYSBUS(spi_state, dev);

    memory_region_init_io(&s->iomem,OBJECT(s), &spi_ops, s, "spi", 0x1000);
    sysbus_init_mmio(dev, &s->iomem);
    sysbus_init_irq(dev, &s->irq);
    s->ssi = ssi_create_bus(device, "ssi");
    spi_reset(s);
    vmstate_register(device, -1, &vmstate_spi, s);
    return 0;
}

static void spi_class_init(ObjectClass *klass, void *data)
{
  fprintf(stderr,"\rin function : %s \r\n",__FUNCTION__);
    SysBusDeviceClass *sdc = SYS_BUS_DEVICE_CLASS(klass);

    sdc->init = spi_init;
}

static const TypeInfo spi_info = {
    .name          = "spi",
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(spi_state),
    .class_init    = spi_class_init,
};

static void spi_register_types(void)
{
  fprintf(stderr,"\rin function : %s \r\n",__FUNCTION__);
    type_register_static(&spi_info);
}

type_init(spi_register_types)

/*
 * Arm PMC
 *
 * Copyright (c) 2007 CodeSourcery.
 * Written by Paul Brook
 *
 * This code is licensed under the GPL.
 */

#include "hw/sysbus.h"
#include "hw/ptimer.h"
#include "qemu/timer.h"
#include "hw/hw.h"

//#define DEBUG_PL022 1

#ifdef DEBUG_PMC
#define DPRINTF(fmt, ...) \
do { printf("pmc: " fmt , ## __VA_ARGS__); } while (0)
#define BADF(fmt, ...) \
do { fprintf(stderr, "pmc: error: " fmt , ## __VA_ARGS__); exit(1);} while (0)
#else
#define DPRINTF(fmt, ...) do {} while(0)
#define BADF(fmt, ...) \
do { fprintf(stderr, "pmc: error: " fmt , ## __VA_ARGS__);} while (0)
#endif

#define PLLA_CHANGE 0x1
#define PLLB_CHANGE 0x2
#define CSS_CHANGE  0x4
#define PRES_CHANGE 0x8

// -------- PMC_SCER : (PMC Offset: 0x0) System Clock Enable Register -------- 
#define  PMC_SCUDP         ((unsigned int) 0x1 <<  7) // (PMC) USB Device Port Clock
#define  PMC_SCPCK0        ((unsigned int) 0x1 <<  8) // (PMC) Programmable Clock Output
#define  PMC_SCPCK1        ((unsigned int) 0x1 <<  9) // (PMC) Programmable Clock Output
#define  PMC_SCPCK2        ((unsigned int) 0x1 << 10) // (PMC) Programmable Clock Output
// -------- PMC_SCDR : (PMC Offset: 0x4) System Clock Disable Register -------- 
// -------- PMC_SCSR : (PMC Offset: 0x8) System Clock Status Register -------- 
// -------- CKGR_MCFR : (PMC Offset: 0x24) Main Clock Frequency Register -------- 
// -------- CKGR_PLLAR : (PMC Offset: 0x28) PLLA  Register -------- 
// -------- CKGR_PLLBR : (PMC Offset: 0x2c) PLLB  Register --------
// -------- CKGR_MOR : (CKGR Offset: 0x20) Main Oscillator Register -------- 
#define  CKGR_MOSCXTEN     ((unsigned int) 0x1 <<  0) // (CKGR) Main Oscillator Enable
#define  CKGR_MOSCXTBY     ((unsigned int) 0x1 <<  1) // (CKGR) Main Oscillator Bypass
#define  CKGR_MOSCXTST     ((unsigned int) 0xff <<  8) // (CKGR) Main Oscillator Start-up Time
#define  CKGR_MOSCRCEN     ((unsigned int) 0x1 <<  3) // (CKGR) RC Oscillator Enable
#define  CKGR_MOSCRCF_4M   ((unsigned int) 0x0 <<  4) // (CKGR)
#define  CKGR_MOSCRCF_8M   ((unsigned int) 0x1 <<  4) // (CKGR)
#define  CKGR_MOSCRCF_12M   ((unsigned int) 0x2 <<  4) // (CKGR)
// -------- CKGR_MCFR : (CKGR Offset: 0x24) Main Clock Frequency Register -------- 
#define  CKGR_MAINF      ((unsigned int) 0xFFFF <<  0) // (CKGR) Main Clock Frequency
#define  CKGR_MAINFRDY    ((unsigned int) 0x1 << 16) // (CKGR) Main Clock Ready
// -------- CKGR_PLLAR : (CKGR Offset: 0x28) PLLA Register -------- 
#define  CKGR_DIVA        ((unsigned int) 0xFF <<  0) // (CKGR) Divider Selected
#define  CKGR_DIVA_0                    ((unsigned int) 0x0) // (CKGR) Divider output is 0
#define  CKGR_DIVA_BYPASS               ((unsigned int) 0x1) // (CKGR) Divider is bypassed
#define  CKGR_PLLACOUNT   ((unsigned int) 0x3F <<  8) // (CKGR) PLLA Counter
#define  CKGR_OUTA        ((unsigned int) 0x3 << 14) // (CKGR) PLLA Output Frequency Range
#define  CKGR_OUTA_0                    ((unsigned int) 0x0 << 14) // (CKGR) Please refer to the PLLA datasheet
#define  CKGR_OUTA_1                    ((unsigned int) 0x1 << 14) // (CKGR) Please refer to the PLLA datasheet
#define  CKGR_OUTA_2                    ((unsigned int) 0x2 << 14) // (CKGR) Please refer to the PLLA datasheet
#define  CKGR_OUTA_3                    ((unsigned int) 0x3 << 14) // (CKGR) Please refer to the PLLA datasheet
#define  CKGR_MULA        ((unsigned int) 0x7FF << 16) // (CKGR) PLLA Multiplier
#define  CKGR_ONE                    ((unsigned int) 0x1 << 29) // (CKGR) Set to 1 when configuring

// -------- CKGR_PLLBR : (CKGR Offset: 0x2c) PLL B Register -------- 
#define  CKGR_DIVB        ((unsigned int) 0xFF <<  0) // (CKGR) Divider Selected
#define  CKGR_DIVB_0                    ((unsigned int) 0x0) // (CKGR) Divider output is 0
#define  CKGR_DIVB_BYPASS               ((unsigned int) 0x1) // (CKGR) Divider is bypassed
#define  CKGR_PLLBCOUNT   ((unsigned int) 0x3F <<  8) // (CKGR) PLLB Counter
#define  CKGR_OUTB        ((unsigned int) 0x3 << 14) // (CKGR) PLLB Output Frequency Range
#define  CKGR_OUTB_0                    ((unsigned int) 0x0 << 14) // (CKGR) Please refer to the PLLB datasheet
#define  CKGR_OUTB_1                    ((unsigned int) 0x1 << 14) // (CKGR) Please refer to the PLLB datasheet
#define  CKGR_OUTB_2                    ((unsigned int) 0x2 << 14) // (CKGR) Please refer to the PLLB datasheet
#define  CKGR_OUTB_3                    ((unsigned int) 0x3 << 14) // (CKGR) Please refer to the PLLB datasheet
#define  CKGR_MULB        ((unsigned int) 0x7FF << 16) // (CKGR) PLL Multiplier

// -------- PMC_MCKR : (PMC Offset: 0x30) Master Clock Register -------- 
#define  PMC_CSS         ((unsigned int) 0x3 <<  0) // (PMC) Programmable Clock Selection
#define  PMC_CSS_SLOW_CLK             ((unsigned int) 0x0) // (PMC) Slow Clock is selected
#define  PMC_CSS_MAIN_CLK             ((unsigned int) 0x1) // (PMC) Main Clock is selected
#define  PMC_CSS_PLLA_CLK              ((unsigned int) 0x2) // (PMC) Clock from PLLA is selected
#define  PMC_CSS_PLLB_CLK              ((unsigned int) 0x3) // (PMC) Clock from PLLB is selected

#define  PMC_PRES_CLK_3  ((unsigned int) 0x7 <<  4) // (PMC) Selected clock divided by 3
#define  PMC_PRES        ((unsigned int) 0x7 <<  4) // (PMC) Selected clock
#define  PMC_PRES_CLK_1  ((unsigned int) 0x0 <<  4) // (PMC) Selected clock
#define  PMC_PRES_CLK_2  ((unsigned int) 0x1 <<  4) // (PMC) Selected clock divided by 2
#define  PMC_PRES_CLK_4  ((unsigned int) 0x2 <<  4) // (PMC) Selected clock divided by 4
#define  PMC_PRES_CLK_8  ((unsigned int) 0x3 <<  4) // (PMC) Selected clock divided by 8
#define  PMC_PRES_CLK_16 ((unsigned int) 0x4 <<  4) // (PMC) Selected clock divided by 16
#define  PMC_PRES_CLK_32               ((unsigned int) 0x5 <<  4) // (PMC) Selected clock divided by 32
#define  PMC_PRES_CLK_64               ((unsigned int) 0x6 <<  4) // (PMC) Selected clock divided by 64
// -------- PMC_PCKR : (PMC Offset: 0x40) Programmable Clock Register -------- 
// -------- PMC_IER : (PMC Offset: 0x60) PMC Interrupt Enable Register -------- 
#define  PMC_MOSCXTS     ((unsigned int) 0x1 <<  0) // (PMC) MOSC Status/Enable/Disable/Mask
#define  PMC_LOCKA       ((unsigned int) 0x1 <<  1) // (PMC) PLLA Status/Enable/Disable/Mask
#define  PMC_LOCKB       ((unsigned int) 0x1 <<  2) // (PMC) PLLB Status/Enable/Disable/Mask
#define  PMC_MCKRDY      ((unsigned int) 0x1 <<  3) // (PMC) MCK_RDY Status/Enable/Disable/Mask
#define  PMC_PCK0RDY     ((unsigned int) 0x1 <<  8) // (PMC) PCK0_RDY Status/Enable/Disable/Mask
#define  PMC_PCK1RDY     ((unsigned int) 0x1 <<  9) // (PMC) PCK1_RDY Status/Enable/Disable/Mask
#define  PMC_PCK2RDY     ((unsigned int) 0x1 << 10) // (PMC) PCK2_RDY Status/Enable/Disable/Mask
#define  PMC_MOSCSELS    ((unsigned int) 0x1 << 16) // (PMC) MOSCSELS_RDY Status/Enable/Disable/Mask
#define  PMC_MOSCRCS     ((unsigned int) 0x1 << 17) // (PMC) MOSCRCS_RDY Status/Enable/Disable/Mask
#define  PMC_CFDEV       ((unsigned int) 0x1 << 18) // (PMC) CFDEV_RDY Status/Enable/Disable/Mask
// -------- PMC_IDR : (PMC Offset: 0x64) PMC Interrupt Disable Register -------- 
// -------- PMC_SR : (PMC Offset: 0x68) PMC Status Register -------- 
#define  PMC_CFDS       ((unsigned int) 0x1 << 19)
#define  PMC_FOS        ((unsigned int) 0x1 << 20)
// -------- PMC_IMR : (PMC Offset: 0x6c) PMC Interrupt Mask Register -------- 


typedef struct {
  SysBusDevice busdev;
  MemoryRegion iomem;
  uint32_t	 scer; 	// System Clock Enable Register
  uint32_t	 scdr; 	// System Clock Disable Register
  uint32_t	 scsr; 	// System Clock Status Register
  //	uint32_t	 Reserved0[1]; 	// 
  uint32_t	 pcer[2]; 	// Peripheral Clock Enable Register
  uint32_t	 pcdr[2]; 	// Peripheral Clock Disable Register
  uint32_t	 pcsr[2]; 	// Peripheral Clock Status Register
  //	uint32_t	 Reserved1[1]; 	// 
  uint32_t	 ckgr_mor; 	// Main Oscillator Register
  uint32_t	 ckgr_mcfr; 	// Main Clock  Frequency Register
  uint32_t	 ckgr_pllar; 	// PLLA Register 
  uint32_t	 ckgr_pllbr; 	// PLLB Register
  uint32_t	 mckr; 	// Master Clock Register
  //	uint32_t	 Reserved3[1]; 	// 
  uint32_t	 usb; 	// USB Clock Register
  //	uint32_t	 Reserved4[1]; 	// 
  uint32_t	 pck[3]; 	// Programmable Clock Register
  //	uint32_t	 Reserved5[5]; 	// 
  uint32_t	 ier; 	// Interrupt Enable Register
  uint32_t	 idr; 	// Interrupt Disable Register
  uint32_t	 sr; 	// Status Register
  uint32_t	 imr; 	// Interrupt Mask Register
  uint32_t	 fsmr; 	// Fast Startup Mode Register
  uint32_t	 fspr; 	// Fast Startup Polarity Register
  uint32_t	 focr; 	// Fault Output Clear Register
  //	uint32_t	 Reserved6[25];	//
  uint32_t	 wpmr; 	// Write Protect Mode Register
  uint32_t	 wpsr; 	// Write Protect Status Register
  //	uint32_t	 Reserved7[5];	//
  //	uint32_t	 Reserved8[1]; 	// 
  uint32_t	 ocr; 	// Oscillator Calibration Register 
  uint32_t       indice;
  uint32_t startup;
  uint32_t plla_count;
  uint32_t pllb_count;
  qemu_irq irq;
  ptimer_state *timer;
  //SSIBus *ssi;
} pmc_state;

static const unsigned char pmc_id[8] =
  { 0x22, 0x10, 0x04, 0x00, 0x0d, 0xf0, 0x05, 0xb1 };

static int pmc_WrProtectTest(pmc_state *s)
{  
  if(s->wpmr == 0x504D4301){//write protected
    DPRINTF("Write Protected ! \n");
    return 0;
  }
  else
    return 1;
}

static void pmc_update(pmc_state *s)
{
  s->pcsr[0] = (~s->pcdr[0]) & s->pcer[0];
  s->pcsr[1] = (~s->pcdr[1]) & s->pcer[1];
  
  if((s->ckgr_mor & CKGR_MOSCXTBY) == CKGR_MOSCXTBY)
    s->sr |= PMC_MOSCXTS; 
  if((s->ckgr_mcfr & CKGR_MAINF) != 0)
    s->ckgr_mcfr |= CKGR_MAINFRDY;

  if(((s->indice & CSS_CHANGE) != 0) || ((s->indice & PRES_CHANGE) != 0))
    s->sr |= PMC_MCKRDY;

  s->sr = (s->sr & ~(PMC_SCPCK0 | PMC_SCPCK1 | PMC_SCPCK2)) | (s->scsr & (PMC_SCPCK0 | PMC_SCPCK1 | PMC_SCPCK2));
  switch(s->mckr & PMC_CSS)
    {
    case PMC_CSS_PLLA_CLK:
      if((s->sr & PMC_LOCKA) == 0)
	s->mckr = (s->mckr & (~PMC_CSS)) | PMC_CSS_SLOW_CLK;
      break;
    case PMC_CSS_PLLB_CLK:
      if((s->sr & PMC_LOCKB) == 0)
	s->mckr = (s->mckr & (~PMC_CSS)) | PMC_CSS_SLOW_CLK;
      break;
    default:
      break;
    }

  if(s->startup ==((s->ckgr_mor & CKGR_MOSCXTST) >> 5))
    s->sr |= PMC_MOSCXTS;

  if((s->indice & PLLA_CHANGE) != 0){
    if(s->plla_count == ((s->ckgr_pllar & CKGR_PLLACOUNT) >> 8))
      s->sr |= PMC_LOCKA;
  }

  if((s->indice & PLLB_CHANGE) != 0){
    if(s->pllb_count == ((s->ckgr_pllbr & CKGR_PLLBCOUNT) >> 8))
      s->sr |= PMC_LOCKB;
  }

  qemu_set_irq(s->irq, ((~(s->idr)) & s->ier & s->imr) != 0);
}

static void pmc_tick(void *opaque)
{
  pmc_state *s = opaque;
  if(((s->ckgr_mor & CKGR_MOSCXTEN) == CKGR_MOSCXTEN) && (s->startup < ((s->ckgr_mor & CKGR_MOSCXTST) >> 5)))
    s->startup++;

  if(s->plla_count < ((s->ckgr_pllar & CKGR_PLLACOUNT) >> 8))
    s->plla_count++;
  
  if(s->pllb_count < ((s->ckgr_pllbr & CKGR_PLLBCOUNT) >> 8))
    s->pllb_count++;
}

static uint64_t pmc_read(void *opaque, hwaddr offset,
                           unsigned size)
{
    pmc_state *s = (pmc_state *)opaque;
    //int val;
    /*
    if (offset >= 0xfe0 && offset < 0x1000) {
        return pmc_id[(offset - 0xfe0) >> 2];
    }
    */
    switch (offset) {
     
    case 0x08: // SCSR 
      return s->scsr;
    case 0x18: // PCSR0 
        return s->pcsr[0];
    case 0x20: // CKGR_MOR 
        return s->ckgr_mor;
    case 0x24: // CKGR_MCFR 
      if((s->ckgr_mcfr & CKGR_MAINFRDY) != 0)
        return s->ckgr_mcfr;
      else
	DPRINTF("MAINF not ready ! \n");
    case 0x28: // CKGR_PLLAR 
        return s->ckgr_pllar;
    case 0x2c: // CKGR_PLLBR 
        return s->ckgr_pllbr;
    case 0x30: // MCKR 
        return s->mckr;
    case 0x38: // USB
        return s->usb;
    case 0x40: // PCK0
        return s->pck[0];
    case 0x44: // PCK1 
        return s->pck[1];
    case 0x48: // pck2 
        return s->pck[2];
    case 0x68: // SR
        return s->sr;
    case 0x6c: // IMR 
        return s->imr;
    case 0x70: // FSMR
        return s->fsmr;
    case 0x74: // FSPR 
        return s->fspr;
    case 0xE4: // WPMR 
        return s->wpmr;
    case 0xE8: // WPSR 
        return s->wpsr;
    case 0x108: // PCSR1 
        return s->pcsr[1];
    case 0x110: // ocr 
        return s->ocr;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "pmc_read: Bad offset %x\n", (int)offset);
        return 0;
    }
}

static void pmc_write(void *opaque, hwaddr offset,
                        uint64_t value, unsigned size)
{
    pmc_state *s = (pmc_state *)opaque;
    s->indice = 0;
    if(pmc_WrProtectTest(s)==1){
      switch (offset) {
      case 0x00: // SCER
	s->scer = value;
	//pmc_update(s);
	break;
      case 0x04: // SCDR
	s->scdr = value;
	break;
      case 0x10: // PCER0 
	s->pcer[0] = value;
	pmc_update(s);
	break;
      case 0x14: // PCDR0 
	s->pcdr[0] = value;
	pmc_update(s);
	break;
      case 0x20: // CKGR_MOR 
	if((value & 0xff0000) == 0x370000)
	  s->ckgr_mor = value;
	else
	  DPRINTF("Invalid write ! \n");
	pmc_update(s);
	break;
      case 0x28: // CKGR_PLLAR 
	if((s->sr & PMC_LOCKA) == 0){
	  if(value & CKGR_ONE){
	    s->ckgr_pllar = value;
	    s->indice |= PLLA_CHANGE;
	    s->plla_count = 0;
	  }
	  else
	    DPRINTF("Not Possible to Configure PLLA!\n");
	}
	else
	  DPRINTF("Already Locked PLLA! \n");
	break;
      case 0x2c: // CKGR_PLLBR 
	if((s->sr & PMC_LOCKB) == 0){
	  s->ckgr_pllbr = value;
	  s->indice |= PLLB_CHANGE;
	  s->pllb_count = 0;
	}
	else{
	  DPRINTF("Already Locked PLLB! \n");
	}
	break;
      case 0x30: // MCKR 
	if((value & PMC_CSS) != (s->mckr & PMC_CSS))
	  s->indice |= CSS_CHANGE;
	if((value & PMC_PRES) != (s->mckr & PMC_PRES))
	  s->indice |= PRES_CHANGE;	
	s->mckr = value;
	pmc_update(s);
	break;
      case 0x38: // USB 
	s->usb = value;
	break;
      case 0x40: // PCK0
	if((s->scsr & PMC_SCPCK0) == 0)
	  s->pck[0] = value;
	else
	  DPRINTF("Cannot Configure PCK0 when it is enabled!\n");
	break;
      case 0x44: // PCK1 
	if((s->scsr & PMC_SCPCK1) == 0)
	  s->pck[1] = value;
	else
	  DPRINTF("Cannot Configure PCK1 when it is enabled!\n");
	break;
      case 0x48: // PCK2 
	if((s->scsr & PMC_SCPCK2) == 0)
	  s->pck[2] = value;
	else
	  DPRINTF("Cannot Configure PCK2 when it is enabled!\n");
	break;
      case 0x60: // IER 
	s->ier = value;
	pmc_update(s);
	break;
      case 0x64: // IDR 
	s->idr = value;
	pmc_update(s);
	break;
      case 0x70: // FSMR 
	s->fsmr = value;
	break;
      case 0x74: // FSPR 
	s->fspr = value;
	break;
      case 0x78: // FSOR 
	s->focr = value;
	break;
      case 0xE4: // WPMR 
	s->wpmr = value;
	break;
      case 0x100: // PCER1 
	s->pcer[1] = value;
	pmc_update(s);
	break;
      case 0x104: // PCDR1
	s->pcdr[1] = value;
	pmc_update(s);
	break;
      case 0x110: // OCR
	s->ocr = value;
	break;
      default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "pmc_write: Bad offset %x\n", (int)offset);
      }
    }
}

static void pmc_reset(pmc_state *s)
{
  s->scsr = 0x00000001;//?
  s->pcer[0] = 0;
  s->pcer[1] = 0;
  s->pcdr[0] = 0;
  s->pcdr[1] = 0;
  s->pcsr[0] = 0;
  s->pcsr[1] = 0;
  s->ckgr_mor = 0x00000001;
  s->ckgr_mcfr = 0;
  s->ckgr_pllar = 0x00003f00;
  s->ckgr_pllbr = 0x00003f00;
  s->mckr = 0x00000001;
  s->usb = 0;
  s->pck[0] = 0;
  s->pck[1] = 0;
  s->pck[2] = 0;
  s->sr = 0x00010008;
  s->imr = 0;
  s->fsmr = 0;
  s->fspr = 0;
  s->wpmr = 0;
  s->wpsr = 0;
  s->ocr = 0x00404040;
  s->indice = 0;
  s->startup = 0;
  s->plla_count = 0;
  s->pllb_count = 0;
}

static const MemoryRegionOps pmc_ops = {
    .read = pmc_read,
    .write = pmc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const VMStateDescription vmstate_pmc = {
    .name = "pmc_ssp",
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields      = (VMStateField[]) {
        VMSTATE_UINT32(scer, pmc_state),
        VMSTATE_UINT32(scdr, pmc_state),
        VMSTATE_UINT32(scsr, pmc_state),
        VMSTATE_UINT32(pcer[0], pmc_state),
        VMSTATE_UINT32(pcdr[0], pmc_state),
        VMSTATE_UINT32(pcsr[0], pmc_state),
        VMSTATE_UINT32(ckgr_mor, pmc_state),
        VMSTATE_UINT32(ckgr_mcfr, pmc_state),
        VMSTATE_UINT32(ckgr_pllar, pmc_state),
        VMSTATE_UINT32(ckgr_pllbr, pmc_state),
        VMSTATE_UINT32(mckr, pmc_state),
        VMSTATE_UINT32(usb, pmc_state),
        VMSTATE_UINT32(pck[0], pmc_state),
        VMSTATE_UINT32(pck[1], pmc_state),
        VMSTATE_UINT32(pck[2], pmc_state),
        VMSTATE_UINT32(ier, pmc_state),
        VMSTATE_UINT32(idr, pmc_state),
        VMSTATE_UINT32(sr, pmc_state),
        VMSTATE_UINT32(imr, pmc_state),
        VMSTATE_UINT32(fsmr, pmc_state),
        VMSTATE_UINT32(fspr, pmc_state),
        VMSTATE_UINT32(focr, pmc_state),
        VMSTATE_UINT32(wpmr, pmc_state),
        VMSTATE_UINT32(wpsr, pmc_state),
        VMSTATE_UINT32(pcer[1], pmc_state),
        VMSTATE_UINT32(pcdr[1], pmc_state),
        VMSTATE_UINT32(pcsr[1], pmc_state),
        VMSTATE_UINT32(ocr, pmc_state),
        VMSTATE_PTIMER(timer,pmc_state),
        VMSTATE_END_OF_LIST()
    }
};

static int pmc_init(SysBusDevice *dev)
{    
    DeviceState *device = DEVICE(dev);
    pmc_state *s = FROM_SYSBUS(pmc_state, dev);
    QEMUBH *bh;
    
    memory_region_init_io(&s->iomem, OBJECT(s), &pmc_ops, s, "pmc", 0x200);
    sysbus_init_mmio(dev, &s->iomem);
    sysbus_init_irq(dev, &s->irq);
    //s->ssi = ssi_create_bus(&dev->qdev, "ssi");
    pmc_reset(s);
    bh = qemu_bh_new(pmc_tick, s);
    s->timer = ptimer_init(bh);

    ptimer_set_freq(s->timer, 10);
    ptimer_set_limit(s->timer, 1, 1);
    ptimer_run(s->timer, 0);
    vmstate_register(device, -1, &vmstate_pmc, s);
    return 0;
}

static void pmc_class_init(ObjectClass *klass, void *data)
{
    SysBusDeviceClass *sdc = SYS_BUS_DEVICE_CLASS(klass);

    sdc->init = pmc_init;
}

static const TypeInfo pmc_info = {
    .name          = "pmc",
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(pmc_state),
    .class_init    = pmc_class_init,
};

static void pmc_register_types(void)
{
    type_register_static(&pmc_info);
}

type_init(pmc_register_types)

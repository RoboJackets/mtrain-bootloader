#include "dram.h"

#include "ch.h"
#include "hal.h"
#include <stdint.h>

// DRAM begins at 0xC000_0000
// 256Mbits = 32MBytes
// Ends at 0xC1FF_FFFF
//
// 16 bit Access Width
// Row Bits: 13
// Column Bits: 9
// Bank Count: 4

// FMC_Bank5_6

void dram_init(void) {
    // DAMM EMABLE FMC CLOCK!!!!!
    RCC->AHB3ENR |= 0x1;


    // 1. Program memory device feature into FMC_SDCRx
    FMC_Bank5_6->SDCR[0] = 0
        |FMC_SDCR1_NC_0     // 9 Bit Column Addr
        |FMC_SDCR1_NR_1     // 13 Row Bits
        |FMC_SDCR1_MWID_0   // 16-bit width
        |FMC_SDCR1_NB       // 4 Banks
        |FMC_SDCR1_CAS      // 3 CAS Cycle
        |FMC_SDCR1_SDCLK    // SDCLK = HCLK/3
        |FMC_SDCR1_RPIPE_0  // 1 HCLK Delay before an read
        ;

    // 2.Program Timing into FMC_SDTRx
    FMC_Bank5_6->SDTR[0] = 0
        | 1 >> FMC_SDTR2_TMRD_Pos   // tMRD: 2 tCK
        | 6 << FMC_SDTR1_TXSR_Pos   // tXSR: 7 tCK
        | 4 << FMC_SDTR1_TRAS_Pos   // tRAS: 5 tCK
        | 5 << FMC_SDTR1_TRC_Pos    // tRC: 6 tCK
        | 1 << FMC_SDTR1_TWR_Pos    // tWR: 2 tCK
        | 1 << FMC_SDTR1_TRP_Pos    // tRP: 2 tCK
        | 1 << FMC_SDTR1_TRCD_Pos   // tRCD: 2 tCK
        ;

    // 3. Derive Clock Start
    FMC_Bank5_6->SDCMR = 0
        |0b001 << FMC_SDCMR_MODE_Pos  // Clock Configuration Mode
        |FMC_SDCMR_CTB1     // Bank 1 enable
        ;

    chThdSleepMilliseconds(1);

    // 5. Precharge All
    FMC_Bank5_6->SDCMR = 0
        |0b010 << FMC_SDCMR_MODE_Pos   // Pre-Charge
        |FMC_SDCMR_CTB1     // Bank 1 enable
        ;

    // 6. Auto-Refresh
    FMC_Bank5_6->SDCMR = 0
        |0b011 << FMC_SDCMR_MODE_Pos    // Auto Refresh
        | 7 << FMC_SDCMR_NRFS_Pos       // Issue 8 Auto Refresh
        |FMC_SDCMR_CTB1                 // Bank 1 enable
        ;

    // 7. Configure MODE Register
    // Register value:
    // 12 .............................. 0
    // 0 0 0 | 1 | 0 0 | 0 1 1 | 0 | 0 0 0
    //  RES  | WB| OPM |  CAS  |BT | BL=1
    FMC_Bank5_6->SDCMR = 0
        |0b100 << FMC_SDCMR_MRD_Pos    // Issue Mode Register Set
        | 0b0001000110000 << FMC_SDCMR_MODE_Pos       //
        |FMC_SDCMR_CTB1                 // Bank 1 enable
        ;

    FMC_Bank5_6->SDRTR = 400 << FMC_SDRTR_COUNT_Pos // LMAO It's well in the safe range
        |FMC_SDRTR_CRE;
}

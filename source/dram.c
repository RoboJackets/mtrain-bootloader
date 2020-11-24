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
        |1 << FMC_SDCR1_NC_Pos     // 9 Bit Column Addr
        |2 << FMC_SDCR1_NR_Pos     // 13 Row Bits
        |1 << FMC_SDCR1_MWID_Pos   // 16-bit width
        |1 << FMC_SDCR1_NB_Pos       // 4 Banks
        |3 << FMC_SDCR1_CAS_Pos      // 3 CAS Cycle
        |3 << FMC_SDCR1_SDCLK_Pos    // SDCLK = HCLK/3
        |1 << FMC_SDCR1_RBURST_Pos
        ;

    // 2.Program Timing into FMC_SDTRx
    FMC_Bank5_6->SDTR[0] = 0
        | 2 << FMC_SDTR2_TMRD_Pos   // tMRD: 2 tCK
        | 7 << FMC_SDTR1_TXSR_Pos   // tXSR: 7 tCK
        | 4 << FMC_SDTR1_TRAS_Pos   // tRAS: 5 tCK
        | 7 << FMC_SDTR1_TRC_Pos    // tRC:  tCK
        | 2 << FMC_SDTR1_TWR_Pos    // tWR: 2 tCK
        | 2 << FMC_SDTR1_TRP_Pos    // tRP: 2 tCK
        | 2 << FMC_SDTR1_TRCD_Pos   // tRCD: 2 tCK
        ;

    // 3. Derive Clock Start
    while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);
    FMC_Bank5_6->SDCMR = 0
        |0b001 << FMC_SDCMR_MODE_Pos  // Clock Configuration Mode
        |FMC_SDCMR_CTB1     // Bank 1 enable
        ;

    chThdSleepMilliseconds(1);

    while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);
    // 5. Precharge All
    FMC_Bank5_6->SDCMR = 0
        |0b010 << FMC_SDCMR_MODE_Pos   // Pre-Charge
        |FMC_SDCMR_CTB1     // Bank 1 enable
        ;

    while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);
    // 6. Auto-Refresh
    FMC_Bank5_6->SDCMR = 0
        |0b011 << FMC_SDCMR_MODE_Pos    // Auto Refresh
        | 7 << FMC_SDCMR_NRFS_Pos       // Issue 8 Auto Refresh
        |FMC_SDCMR_CTB1                 // Bank 1 enable
        ;

    while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);
    // 7. Configure MODE Register
    // Register value:
    // 12 .............................. 0
    // 0 0 0 | 0 | 0 0 | 0 1 0 | 0 | 0 0 0
    //  RES  | WB| OPM |  CAS  |BT | BL=1
    FMC_Bank5_6->SDCMR = 0
        |0b100 << FMC_SDCMR_MODE_Pos     // Issue Mode Register Set
        |0b110000 << FMC_SDCMR_MRD_Pos //
        |FMC_SDCMR_CTB1                 // Bank 1 enable
        ;
    while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);

    FMC_Bank5_6->SDRTR = 400 << FMC_SDRTR_COUNT_Pos // LMAO It's well in the safe range
        |FMC_SDRTR_CRE;
}

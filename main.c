/**
 * @defgroup   MAIN main
 *
 * @brief      Program main for mTrain bootloader
 *
 * @author     Codetector
 * @date       2020
 */
#include "ch.h"
#include "hal.h"
#include "dram.h"
#include "usbdfu.h"
#include "firmware_update.h"

/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  // Init DRAM as early as possible
  dram_init();


  usbStart(&USBD2, &usbcfg);
  usbDisconnectBus(&USBD2);
  for (int i = 0; i < 15; ++i)
  {
    chThdSleepMilliseconds(100);
    palToggleLine(LINE_LED_USB);
  }
  usbConnectBus(&USBD2);
  palSetLine(LINE_LED_USB);

  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state.
   */
  while (true) {
    chThdSleepMilliseconds(500);
  }
}

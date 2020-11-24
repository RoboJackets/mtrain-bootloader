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
 * This is a periodic thread that does absolutely nothing except flashing
 * a LED.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    if (dfu_currentState != STATE_DFU_IDLE) {
      palSetLine(LINE_LED1);
      chThdSleepMilliseconds(100);
      palClearLine(LINE_LED1);
      chThdSleepMilliseconds(100);
    } else {
      palClearLine(LINE_LED1);
    }
  }
}

char memTest(void) {
  uint32_t num_DWORD = 1024 * 1024 * 8;
  #define step 1024
  #define offset 0
  for (uint32_t i = offset; i < num_DWORD; i += step)
  {
    *(((uint32_t*)firmware_buffer) + i) = (uint32_t) (i);
    SCB_CleanDCache();
    if (*(((uint32_t*)firmware_buffer) + i) != (uint32_t) i)
      return 1;
  }

  for (uint32_t i = offset; i < num_DWORD; i += step)
  {
    if (*(((uint32_t*)firmware_buffer) + i) != (uint32_t) (i))
      return 1;
  }
  #undef step
  #undef offset

  return 0;
}

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

  // DRAM TEST
  palSetLine(LINE_LED1);
  if (memTest()) {
    palSetLine(LINE_LED4);
  } else {
    palClearLine(LINE_LED4);
  }
  palClearLine(LINE_LED1);
  while(1){}

  usbStart(&USBD2, &usbcfg);
  usbDisconnectBus(&USBD2);
  chThdSleepMilliseconds(1500);
  usbConnectBus(&USBD2);

  /*
   * Creates the example thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state.
   */
  while (true) {
    chThdSleepMilliseconds(500);
  }
}

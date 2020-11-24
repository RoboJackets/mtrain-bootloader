/**
 * @defgroup   USBDFU usbdfu
 *
 * @brief      This file implements usbdfu protocol.
 *
 * @author     Codetector
 * @date       2020
 */


#include "hal.h"
#include "usbdfu.h"
#include "firmware_update.h"


/*
 * USB Device Descriptor.
 */
static const uint8_t dfu_device_descriptor_data[18] = {
  USB_DESC_DEVICE       (0x0200,        /* bcdUSB (1.1).                    */
                         0x00,          /* bDeviceClass                     */
                         0x00,          /* bDeviceSubClass.                 */
                         0x00,          /* bDeviceProtocol.                 */
                         64,            /* bMaxPacketSize.                  */
                         0xfeed,        /* idVendor.                        */
                         0x6969,        /* idProduct.                       */
                         0x0200,        /* bcdDevice.                       */
                         1,             /* iManufacturer.                   */
                         2,             /* iProduct.                        */
                         3,             /* iSerialNumber.                   */
                         1)             /* bNumConfigurations.              */
};

static const USBDescriptor dfu_device_descriptor = {
  sizeof dfu_device_descriptor_data,
  dfu_device_descriptor_data
};


/* Configuration Descriptor tree for a DFU.*/
static const uint8_t dfu_configuration_descriptor_data[27] = {
  /* Configuration Descriptor.*/
  USB_DESC_CONFIGURATION(27,            /* wTotalLength.                    */
                         0x01,          /* bNumInterfaces.                  */
                         0x01,          /* bConfigurationValue.             */
                         0,             /* iConfiguration.                  */
                         0xC0,          /* bmAttributes (self powered).     */
                         50),           /* bMaxPower (100mA).               */
  /* Interface Descriptor.*/
  USB_DESC_INTERFACE    (0x00,          /* bInterfaceNumber.                */
                         0x00,          /* bAlternateSetting.               */
                         0x00,          /* bNumEndpoints.                   */
                         0xFE,          /* bInterfaceClass (DFU Class
                                                                            */
                         0x01,          /* bInterfaceSubClass               */
                         0x02,          /* bInterfaceProtocol               */
                         0),            /* iInterface.                      */

  USB_DESC_BYTE         (9),            /* bLength.                         */
  USB_DESC_BYTE         (0x21),         /* bDescriptorType (DFU_FCUNTION).  */
  USB_DESC_BYTE         (0b1011),       /* bmAttributes (DETACH | DOWNLOAD) */
  USB_DESC_WORD         (   500),       /* Timeout.                         */
  USB_DESC_WORD         (512  ),        /* wTransferSize                    */
  USB_DESC_BCD          (0x0110)        /* bcdUSBDFUVersion: 1.1            */
};

static const USBDescriptor dfu_configuration_descriptor = {
  sizeof dfu_configuration_descriptor_data,
  dfu_configuration_descriptor_data
};

static const uint8_t dfu_string0[] = {
  USB_DESC_BYTE(4),                     /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  USB_DESC_WORD(0x0409)                 /* wLANGID (U.S. English).          */
};

static const uint8_t dfu_string1[] = {
  USB_DESC_BYTE(24),                    /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  'R', 0, 'o', 0, 'b', 0, 'o', 0, 'J', 0, 'a', 0, 'c', 0, 'k', 0,
  'e', 0, 't', 0, 's', 0
};

/*
 * Device Description string.
 */
static const uint8_t dfu_string2[] = {
  USB_DESC_BYTE(22),                    /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  'm', 0, 'T', 0, 'r', 0, 'a', 0, 'i', 0, 'n', 0, ' ', 0,
  'D', 0, 'F', 0, 'U', 0
};

/*
 * Serial Number string.
 */
static const uint8_t dfu_string3[] = {
  USB_DESC_BYTE(8),                     /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  '0' + CH_KERNEL_MAJOR, 0,
  '0' + CH_KERNEL_MINOR, 0,
  '0' + CH_KERNEL_PATCH, 0
};

/*
 * Strings wrappers array.
 */
static const USBDescriptor dfu_strings[] = {
  {sizeof dfu_string0, dfu_string0},
  {sizeof dfu_string1, dfu_string1},
  {sizeof dfu_string2, dfu_string2},
  {sizeof dfu_string3, dfu_string3}
};


/*
 * Handles the GET_DESCRIPTOR callback. All required descriptors must be
 * handled here.
 */
static const USBDescriptor *get_descriptor(USBDriver *usbp,
                                           uint8_t dtype,
                                           uint8_t dindex,
                                           uint16_t lang) {

  (void)usbp;
  (void)lang;
  switch (dtype) {
  case USB_DESCRIPTOR_DEVICE:
    return &dfu_device_descriptor;
  case USB_DESCRIPTOR_CONFIGURATION:
    return &dfu_configuration_descriptor;
  case USB_DESCRIPTOR_STRING:
    if (dindex < 4)
      return &dfu_strings[dindex];
  }
  return NULL;
}

static void usb_event(USBDriver *usbp, usbevent_t event) {
}

size_t currentBufferOffset = 0;
enum dfu_state dfu_currentState = STATE_DFU_IDLE;
static enum {
  LASTOP_IDLE,
  LASTOP_DNLOAD,
  LASTOP_UPLOAD
} lastOperation;

static uint8_t status_response_buffer[6] = {
    DFU_STATUS_OK, // Status (0)
    1, 0x00, 0x00,
    0x00, // Next State (4)
    0
};

static bool request_handler(USBDriver *usbp) {
    if((usbp->setup[0] & USB_RTYPE_TYPE_MASK) == USB_RTYPE_TYPE_CLASS) {
        uint16_t transfer_size = (((uint16_t)usbp->setup[7]) << 8) | usbp->setup[6];
        uint16_t block_cnt = (((uint16_t)usbp->setup[4]) << 8) | usbp->setup[3];
        switch (usbp->setup[1]) {
            case DFU_GETSTATUS:
                if (dfu_currentState == STATE_DFU_DNLOAD_SYNC) {
                  dfu_currentState = STATE_DFU_DNBUSY;
                }
                status_response_buffer[4] = dfu_currentState;
                usbSetupTransfer(usbp, (uint8_t *)status_response_buffer, 6, NULL);
                return true;
            case DFU_GETSTATE:
                usbSetupTransfer(usbp, (uint8_t *)&dfu_currentState, 1, NULL);
                return true;
            case DFU_UPLOAD: // Upload current firmware buffer to HOST
                if (lastOperation != LASTOP_UPLOAD) {
                  lastOperation = LASTOP_UPLOAD;
                  currentBufferOffset = 0;
                }
                if ((currentBufferOffset + transfer_size) > firmware_size) {
                    transfer_size = firmware_size - currentBufferOffset;
                    usbSetupTransfer(
                      usbp,
                      &firmware_buffer[currentBufferOffset],
                      transfer_size,
                      NULL
                    );
                    lastOperation = LASTOP_IDLE;
                    dfu_currentState = STATE_DFU_IDLE;
                } else {
                    usbSetupTransfer(
                      usbp,
                      &firmware_buffer[currentBufferOffset],
                      transfer_size,
                      NULL
                    );
                    currentBufferOffset += transfer_size;
                    dfu_currentState = STATE_DFU_UPLOAD_IDLE;
                }
                return true;
            case DFU_ABORT:
              lastOperation = LASTOP_IDLE;
              dfu_currentState = STATE_DFU_IDLE;
              return true;
            case DFU_DNLOAD:
              if (lastOperation != LASTOP_DNLOAD) {
                lastOperation = LASTOP_DNLOAD;
                currentBufferOffset = 0;
              }
              if (transfer_size) {
                usbSetupTransfer(usbp, &firmware_buffer[currentBufferOffset], transfer_size, NULL);
                currentBufferOffset += transfer_size;
                dfu_currentState = STATE_DFU_DNLOAD_IDLE;
              } else {
                firmware_size = currentBufferOffset;
                dfu_currentState = STATE_DFU_MANIFEST_WAIT_RESET;
                lastOperation = LASTOP_IDLE;
              }
              return true;
        }
    }
    return false;
}

/*
 * USB driver configuration.
 */
const USBConfig usbcfg = {
  usb_event,
  get_descriptor,
  request_handler,
  NULL
};

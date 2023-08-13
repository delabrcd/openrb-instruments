/* Copyright (C) 2012 Kristian Lauszus, TKJ Electronics. All rights reserved.

 This software may be distributed and modified under the terms of the GNU
 General Public License version 2 (GPL2) as published by the Free Software
 Foundation and appearing in the file GPL2.TXT included in the packaging of
 this file. Please note that GPL2 Section 2[b] requires that all works based
 on this software must also be made publicly available under the terms of
 the GPL2 ("Copyleft").

 Contact information
 -------------------

 Kristian Lauszus, TKJ Electronics
 Web      :  http://www.tkjelectronics.com
 e-mail   :  kristianl@tkjelectronics.com
 */

#ifndef _xboxusb_h_
#define _xboxusb_h_

#include "Usb.h"
#include "usbhid.h"
#include "xboxEnums.h"

/* Data Xbox 360 taken from descriptors */
#define EP_MAXPKTSIZE 32  // max size for data via USB

/* Names we give to the 3 Xbox360 pipes */
#define XBOX_CONTROL_PIPE 0
#define XBOX_INPUT_PIPE 1
#define XBOX_OUTPUT_PIPE 2

#define SANJAY_VID 0x1209
#define SANJAY_GUITAR_PID 0x2882

#define XBOX_REPORT_BUFFER_SIZE 14  // Size of the input report buffer
#define XBOX_MAX_ENDPOINTS 3

struct xb_three_gh_input_pkt_t {
    uint8_t : 8;

    uint8_t command;

    uint8_t : 2;
    uint8_t selectButton : 1;
    uint8_t startButton : 1;

    uint8_t right : 1;
    uint8_t left : 1;
    uint8_t strumDown : 1;
    uint8_t strumUp : 1;

    uint8_t yellowButton : 1;
    uint8_t blueButton : 1;
    uint8_t redButton : 1;
    uint8_t greenButton : 1;

    uint8_t : 3;
    uint8_t orangeButton : 1;

    uint8_t unused[6];

    uint16_t whammy;
    uint16_t tilt;
} __attribute__((packed));

enum xb_three_command {

};

static_assert(XBOX_REPORT_BUFFER_SIZE == sizeof(xb_three_gh_input_pkt_t),
              "Incorrect XBOX report size");

/** This class implements support for a Xbox wired controller via USB. */
class ARDWIINO : public USBDeviceConfig {
public:
    /**
     * Constructor for the ARDWIINO class.
     * @param  pUsb   Pointer to USB class instance.
     */
    ARDWIINO(USB *pUsb);

    /** @name USBDeviceConfig implementation */
    /**
     * Initialize the Xbox Controller.
     * @param  parent   Hub number.
     * @param  port     Port number on the hub.
     * @param  lowspeed Speed of the device.
     * @return          0 on success.
     */
    uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
    /**
     * Release the USB device.
     * @return 0 on success.
     */
    uint8_t Release();
    /**
     * Poll the USB Input endpoins and run the state machines.
     * @return 0 on success.
     */
    uint8_t Poll();

    /**
     * Get the device address.
     * @return The device address.
     */
    virtual uint8_t GetAddress() {
        return bAddress;
    };

    /**
     * Used to check if the controller has been initialized.
     * @return True if it's ready.
     */
    virtual bool isReady() {
        return bPollEnable;
    };

    /**
     * Used by the USB core to check what this driver support.
     * @param  vid The device's VID.
     * @param  pid The device's PID.
     * @return     Returns true if the device's VID and PID matches this driver.
     */
    virtual bool VIDPIDOK(uint16_t vid, uint16_t pid) {
        return ((vid == SANJAY_VID) && (pid == SANJAY_GUITAR_PID));
    };
    /**@}*/

    /** Turn off all the LEDs on the controller. */
    void setAllOff() {
        setLedRaw(0);
    };

    /**
     * Set LED value. Without using the ::LEDEnum or ::LEDModeEnum.
     * @param value      See:
     * setLedOff(), setLedOn(LEDEnum l),
     * setLedBlink(LEDEnum l), and setLedMode(LEDModeEnum lm).
     */
    void setLedRaw(uint8_t value);

    /** Turn all LEDs off the controller. */
    void setLedOff() {
        setLedRaw(0);
    };
    /**
     * Turn on a LED by using ::LEDEnum.
     * @param l          ::OFF, ::LED1, ::LED2, ::LED3 and ::LED4 is supported by the Xbox
     * controller.
     */
    void setLedOn(LEDEnum l);
    /**
     * Turn on a LED by using ::LEDEnum.
     * @param l          ::ALL, ::LED1, ::LED2, ::LED3 and ::LED4 is supported by the Xbox
     * controller.
     */
    void setLedBlink(LEDEnum l);
    /**
     * Used to set special LED modes supported by the Xbox controller.
     * @param lm         See ::LEDModeEnum.
     */
    void setLedMode(LEDModeEnum lm);

    /**@}*/

    /** True if a Xbox 360 controller is connected. */
    bool Xbox360Connected;

    inline const xb_three_gh_input_pkt_t *getInputPacket() {
        return &pkt;
    }

protected:
    /** Pointer to USB class instance. */
    USB *pUsb;
    /** Device address. */
    uint8_t bAddress;
    /** Endpoint info structure. */
    EpInfo epInfo[XBOX_MAX_ENDPOINTS];

private:
    void onInit();

    bool bPollEnable;

    union {
        uint8_t                 readBuf[EP_MAXPKTSIZE];  // General purpose buffer for input data
        xb_three_gh_input_pkt_t pkt;
    };

    uint8_t writeBuf[8];  // General purpose buffer for output data

    void readReport();    // read incoming data
    void printReport();   // print incoming date - Uncomment for debugging

    /* Private commands */
    void XboxCommand(uint8_t *data, uint16_t nbytes);
};

#endif  // _xboxusb_h_
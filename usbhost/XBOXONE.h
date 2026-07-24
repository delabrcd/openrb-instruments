/* Copyright (C) 2012 Kristian Lauszus, TKJ Electronics. All rights reserved.
   Copyright (C) 2015 guruthree

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

 guruthree
 Web      :  https://github.com/guruthree/
 */

#ifndef _xboxone_h_
#define _xboxone_h_

#include "Usb.h"
#include "xboxEnums.h"

/* Xbox One data taken from descriptors */
#define XBOX_ONE_EP_MAXPKTSIZE 64  // Max size for data via USB

/* Names we give to the 3 XboxONE pipes */
#define XBOX_ONE_CONTROL_PIPE 0
#define XBOX_ONE_OUTPUT_PIPE 1
#define XBOX_ONE_INPUT_PIPE 2

#define XBOX_ONE_MAX_ENDPOINTS 3

// The controllers this driver binds to, as exact {VID, PID} pairs.
//
// To add support for another controller add one XBOX_DEVICE(vid, pid, name) line
// below; to drop one, delete its line. Nothing else needs to change - the lookup
// table in XBOXONE.cpp is generated from this list. The name is documentation
// only and costs no flash.
//
// Reference for known devices:
// https://github.com/torvalds/linux/blob/master/drivers/input/joystick/xpad.c
#define XBOX_SUPPORTED_DEVICE_LIST(XBOX_DEVICE)                             \
    /* Official controllers */                                              \
    XBOX_DEVICE(0x045E, 0x02D1, "Microsoft X-Box One pad")                  \
    XBOX_DEVICE(0x045E, 0x02DD, "Microsoft X-Box One pad (Firmware 2015)")  \
    XBOX_DEVICE(0x045E, 0x02E3, "Microsoft X-Box One Elite pad")            \
    XBOX_DEVICE(0x045E, 0x02EA, "Microsoft X-Box One S pad")                \
    XBOX_DEVICE(0x045E, 0x0B0A, "Microsoft X-Box One Adaptive Controller")  \
    XBOX_DEVICE(0x045E, 0x0B12, "Microsoft X-Box Core Controller")          \
                                                                            \
    /* Unofficial controllers */                                            \
    /* NOTE: the FightStick TE 2 may need a different trigger mapping */    \
    XBOX_DEVICE(0x0738, 0x4A01, "Mad Catz FightStick TE 2")                 \
    XBOX_DEVICE(0x0E6F, 0x0139, "Afterglow Prismatic Wired Controller")     \
    XBOX_DEVICE(0x0E6F, 0x0146, "Rock Candy Wired Controller for Xbox One") \
    XBOX_DEVICE(0x0F0D, 0x0067, "HORIPAD ONE")                              \
    XBOX_DEVICE(0x1532, 0x0A03, "Razer Wildcat")                            \
    XBOX_DEVICE(0x24C6, 0x541A, "PowerA Xbox One Mini Wired Controller")    \
    XBOX_DEVICE(0x24C6, 0x542A, "Xbox ONE spectra")                         \
    XBOX_DEVICE(0x24C6, 0x543A, "PowerA Xbox One wired controller")         \
    XBOX_DEVICE(0x20D6, 0x2801, "BDA Xbox ONE Enhanced Controller")

/** This class implements support for a Xbox ONE controller connected via USB. */
class XBOXONE : public USBDeviceConfig, public UsbConfigXtracter {
public:
    /**
     * Constructor for the XBONEAUTH class.
     * @param  pUsb   Pointer to USB class instance.
     */
    XBOXONE(USB *pUsb, void (*data_cb)(const uint8_t *data, const uint8_t &ndata));

    /** @name USBDeviceConfig implementation */
    /**
     * Initialize the Xbox Controller.
     * @param  parent   Hub number.
     * @param  port     Port number on the hub.
     * @param  lowspeed Speed of the device.
     * @return          0 on success.
     */
    virtual uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
    /**
     * Release the USB device.
     * @return 0 on success.
     */
    virtual uint8_t Release();
    /**
     * Poll the USB Input endpoins and run the state machines.
     * @return 0 on success.
     */
    virtual uint8_t Poll();

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
     * Read the poll interval taken from the endpoint descriptors.
     * @return The poll interval in ms.
     */
    uint8_t readPollInterval() {
        return pollInterval;
    };

    /**
     * Used by the USB core to check what this driver support.
     * @param  vid The device's VID.
     * @param  pid The device's PID.
     * @return     Returns true if the device's VID and PID matches this driver.
     */
    virtual bool VIDPIDOK(uint16_t vid, uint16_t pid);
    /**@}*/

    /** @name Xbox Controller functions */
    /**
     * getButtonPress(ButtonEnum b) will return true as long as the button is held down.
     *
     * While getButtonClick(ButtonEnum b) will only return it once.
     *
     * So you instance if you need to increase a variable once you would use
     * getButtonClick(ButtonEnum b), but if you need to drive a robot forward you would use
     * getButtonPress(ButtonEnum b).
     * @param  b          ::ButtonEnum to read.
     * @return            getButtonClick(ButtonEnum b) will return a bool, while
     * getButtonPress(ButtonEnum b) will return a word if reading ::L2 or ::R2.
     */
    uint16_t getButtonPress(ButtonEnum b);
    bool     getButtonClick(ButtonEnum b);

    /**
     * Return the analog value from the joysticks on the controller.
     * @param  a          Either ::LeftHatX, ::LeftHatY, ::RightHatX or ::RightHatY.
     * @return            Returns a signed 16-bit integer.
     */
    int16_t getAnalogHat(AnalogHatEnum a);

    /**
     * Used to call your own function when the controller is successfully initialized.
     * @param funcOnInit Function to call.
     */
    void attachOnInit(void (*funcOnInit)(void)) {
        pFuncOnInit = funcOnInit;
    };

    /** Used to set the rumble off. */
    void setRumbleOff();

    /**
     * Used to turn on rumble continuously.
     * @param leftTrigger  Left trigger force.
     * @param rightTrigger Right trigger force.
     * @param leftMotor    Left motor force.
     * @param rightMotor   Right motor force.
     */
    void setRumbleOn(uint8_t leftTrigger, uint8_t rightTrigger, uint8_t leftMotor,
                     uint8_t rightMotor);
    /**@}*/

    /** True if a Xbox ONE controller is connected. */
    bool XboxOneConnected;

    uint8_t XboxCommand(uint8_t *data, uint16_t nbytes);

protected:
    bool needreset;
    USB *pUsb;

    void (*dataCallback)(const uint8_t *data, const uint8_t &ndata);

    /** Pointer to USB class instance. */
    /** Device address. */
    uint8_t bAddress;
    /** Endpoint info structure. */
    EpInfo epInfo[XBOX_ONE_MAX_ENDPOINTS];

    /** Configuration number. */
    uint8_t bConfNum;
    /** Total number of endpoints in the configuration. */
    uint8_t bNumEP;
    /** Next poll time based on poll interval taken from the USB descriptor. */
    uint32_t qNextPollTime;

    /** @name UsbConfigXtracter implementation */
    /**
     * UsbConfigXtracter implementation, used to extract endpoint information.
     * @param conf  Configuration value.
     * @param iface Interface number.
     * @param alt   Alternate setting.
     * @param proto Interface Protocol.
     * @param ep    Endpoint Descriptor.
     */
    void EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto,
                        const USB_ENDPOINT_DESCRIPTOR *ep);
    /**@}*/

    /**
     * Used to print the USB Endpoint Descriptor.
     * @param ep_ptr Pointer to USB Endpoint Descriptor.
     */
    void PrintEndpointDescriptor(const USB_ENDPOINT_DESCRIPTOR *ep_ptr);

private:
    /**
     * Called when the controller is successfully initialized.
     * Use attachOnInit(void (*funcOnInit)(void)) to call your own function.
     */
    void onInit();
    void (*pFuncOnInit)(void);  // Pointer to function called in onInit()

    uint8_t pollInterval;
    bool    bPollEnable;

    /* Variables to store the buttons */
    uint16_t ButtonState;
    uint16_t OldButtonState;
    uint16_t ButtonClickState;
    int16_t  hatValue[4];
    uint16_t triggerValue[2];
    uint16_t triggerValueOld[2];

    bool L2Clicked;  // These buttons are analog, so we use we use these bools to check if they
                     // where clicked or not
    bool R2Clicked;

    bool sharePressed;  // This button doesn't fit in the bitfield
    bool shareClicked;

    uint8_t readBuf[XBOX_ONE_EP_MAXPKTSIZE];  // General purpose buffer for input data
    uint8_t cmdCounter;

    void readReport();  // Used to read the incoming data

    /* Private commands */
};
#endif

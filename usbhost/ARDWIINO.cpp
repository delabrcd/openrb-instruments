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

#include "ARDWIINO.h"
// To enable serial debugging see "settings.h"
// #define EXTRADEBUG // Uncomment to get even more debugging data
// #define PRINTREPORT  // Uncomment to print the report send by the Xbox 360 Controller

ARDWIINO::ARDWIINO(USB *p)
    : pUsb(p),              // pointer to USB class instance - mandatory
      bAddress(0),          // device address - mandatory
      bPollEnable(false) {  // don't start polling before dongle is connected
    for (uint8_t i = 0; i < XBOX_MAX_ENDPOINTS; i++) {
        epInfo[i].epAddr      = 0;
        epInfo[i].maxPktSize  = (i) ? 0 : 8;
        epInfo[i].bmSndToggle = 0;
        epInfo[i].bmRcvToggle = 0;
        epInfo[i].bmNakPower  = (i) ? USB_NAK_NOWAIT : USB_NAK_MAX_POWER;
    }

    if (pUsb)                             // register in USB subsystem
        pUsb->RegisterDeviceClass(this);  // set devConfig[] entry
}

uint8_t ARDWIINO::Init(uint8_t parent, uint8_t port, bool lowspeed) {
    uint8_t                buf[sizeof(USB_DEVICE_DESCRIPTOR)];
    USB_DEVICE_DESCRIPTOR *udd = reinterpret_cast<USB_DEVICE_DESCRIPTOR *>(buf);
    uint8_t                rcode;
    UsbDevice             *p         = NULL;
    EpInfo                *oldep_ptr = NULL;
    uint16_t               PID;
    uint16_t               VID;

    // get memory address of USB device address pool
    AddressPool &addrPool = pUsb->GetAddressPool();
#ifdef EXTRADEBUG
    Notify(PSTR("\r\nXBOXUSB Init"), 0x80);
#endif
    // check if address has already been assigned to an instance
    if (bAddress) {
#ifdef DEBUG_USB_HOST
        Notify(PSTR("\r\nAddress in use"), 0x80);
#endif
        return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;
    }

    // Get pointer to pseudo device with address 0 assigned
    p = addrPool.GetUsbDevicePtr(0);

    if (!p) {
#ifdef DEBUG_USB_HOST
        Notify(PSTR("\r\nAddress not found"), 0x80);
#endif
        return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
    }

    if (!p->epinfo) {
#ifdef DEBUG_USB_HOST
        Notify(PSTR("\r\nepinfo is null"), 0x80);
#endif
        return USB_ERROR_EPINFO_IS_NULL;
    }

    // Save old pointer to EP_RECORD of address 0
    oldep_ptr = p->epinfo;

    // Temporary assign new pointer to epInfo to p->epinfo in order to avoid toggle inconsistence
    p->epinfo = epInfo;

    p->lowspeed = lowspeed;

    // Get device descriptor
    rcode = pUsb->getDevDescr(0, 0, sizeof(USB_DEVICE_DESCRIPTOR),
                              (uint8_t *)buf);  // Get device descriptor - addr, ep, nbytes, data
    // Restore p->epinfo
    p->epinfo = oldep_ptr;

    if (rcode)
        goto FailGetDevDescr;

    VID = udd->idVendor;
    PID = udd->idProduct;

    if (VID != SANJAY_VID)              // Check VID
        goto FailUnknownDevice;
    else if (PID != SANJAY_GUITAR_PID)  // Check PID
        goto FailUnknownDevice;

    // Allocate new address according to device class
    bAddress = addrPool.AllocAddress(parent, false, port);

    if (!bAddress)
        return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;

    // Extract Max Packet Size from device descriptor
    epInfo[0].maxPktSize = udd->bMaxPacketSize0;

    // Assign new address to the device
    rcode = pUsb->setAddr(0, 0, bAddress);
    if (rcode) {
        p->lowspeed = false;
        addrPool.FreeAddress(bAddress);
        bAddress = 0;
#ifdef DEBUG_USB_HOST
        Notify(PSTR("\r\nsetAddr: "), 0x80);
        D_PrintHex<uint8_t>(rcode, 0x80);
#endif
        return rcode;
    }
#ifdef EXTRADEBUG
    Notify(PSTR("\r\nAddr: "), 0x80);
    D_PrintHex<uint8_t>(bAddress, 0x80);
#endif
    // delay(300); // Spec says you should wait at least 200ms

    p->lowspeed = false;

    // get pointer to assigned address record
    p = addrPool.GetUsbDevicePtr(bAddress);
    if (!p)
        return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

    p->lowspeed = lowspeed;

    // Assign epInfo to epinfo pointer - only EP0 is known
    rcode = pUsb->setEpInfoEntry(bAddress, 1, epInfo);
    if (rcode)
        goto FailSetDevTblEntry;

    // TODO CDD - look into why these are different and determine if we need to be dynamically
    // finding them
    /* Initialize data structures for endpoints of device */
    epInfo[XBOX_INPUT_PIPE].epAddr      = 0x02;            // XBOX 360 report endpoint
    epInfo[XBOX_INPUT_PIPE].epAttribs   = USB_TRANSFER_TYPE_INTERRUPT;
    epInfo[XBOX_INPUT_PIPE].bmNakPower  = USB_NAK_NOWAIT;  // Only poll once for interrupt endpoints
    epInfo[XBOX_INPUT_PIPE].maxPktSize  = EP_MAXPKTSIZE;
    epInfo[XBOX_INPUT_PIPE].bmSndToggle = 0;
    epInfo[XBOX_INPUT_PIPE].bmRcvToggle = 0;
    epInfo[XBOX_OUTPUT_PIPE].epAddr     = 0x81;            // XBOX 360 output endpoint (??? maybe)
    epInfo[XBOX_OUTPUT_PIPE].epAttribs  = USB_TRANSFER_TYPE_INTERRUPT;
    epInfo[XBOX_OUTPUT_PIPE].bmNakPower = USB_NAK_NOWAIT;  // Only poll once for interrupt endpoints
    epInfo[XBOX_OUTPUT_PIPE].maxPktSize = EP_MAXPKTSIZE;
    epInfo[XBOX_OUTPUT_PIPE].bmSndToggle = 0;
    epInfo[XBOX_OUTPUT_PIPE].bmRcvToggle = 0;

    rcode = pUsb->setEpInfoEntry(bAddress, 3, epInfo);
    if (rcode)
        goto FailSetDevTblEntry;

    delay(200);  // Give time for address change

    rcode = pUsb->setConf(bAddress, epInfo[XBOX_CONTROL_PIPE].epAddr, 1);
    if (rcode)
        goto FailSetConfDescr;

#ifdef DEBUG_USB_HOST
    Notify(PSTR("\r\nXbox 360 Controller Connected\r\n"), 0x80);
#endif
    onInit();
    Xbox360Connected = true;
    bPollEnable      = true;
    return 0;  // Successful configuration

               /* Diagnostic messages */
FailGetDevDescr:
#ifdef DEBUG_USB_HOST
    NotifyFailGetDevDescr();
    goto Fail;
#endif

FailSetDevTblEntry:
#ifdef DEBUG_USB_HOST
    NotifyFailSetDevTblEntry();
    goto Fail;
#endif

FailSetConfDescr:
#ifdef DEBUG_USB_HOST
    NotifyFailSetConfDescr();
#endif
    goto Fail;

FailUnknownDevice:
#ifdef DEBUG_USB_HOST
    NotifyFailUnknownDevice(VID, PID);
#endif
    rcode = USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;

Fail:
#ifdef DEBUG_USB_HOST
    Notify(PSTR("\r\nXbox 360 Init Failed, error code: "), 0x80);
    NotifyFail(rcode);
#endif
    Release();
    return rcode;
}

/* Performs a cleanup after failed Init() attempt */
uint8_t ARDWIINO::Release() {
    Xbox360Connected = false;
    pUsb->GetAddressPool().FreeAddress(bAddress);
    bAddress    = 0;
    bPollEnable = false;
    return 0;
}

uint8_t ARDWIINO::Poll() {
    if (!bPollEnable)
        return 0;
    uint16_t BUFFER_SIZE = EP_MAXPKTSIZE;
    if (pUsb->inTransfer(bAddress, epInfo[XBOX_INPUT_PIPE].epAddr, &BUFFER_SIZE, readBuf))
        return 0;
    updateGuitarState();
#ifdef PRINTREPORT
    readReport();
    printReport();  // Uncomment "#define PRINTREPORT" to print the report send by the Xbox 360
                    // Controller
#endif
    return 0;
}

#ifdef PRINTREPORT
static bool isEmpty(uint8_t *buf, size_t nbuf) {
    for (size_t i = 0; i < nbuf; i++) {
        if (buf[i])
            return false;
    }
    return true;
}
#endif

void ARDWIINO::readReport() {
#ifdef PRINTREPORT
    if (isEmpty(readBuf, XBOX_REPORT_BUFFER_SIZE))
        return;

    bool newline = false;
    if (pkt.blueButton) {
        Notify(PSTR("BLUE "), 0x80);
        newline = true;
    }
    if (pkt.greenButton) {
        Notify(PSTR("GREEN "), 0x80);
        newline = true;
    }
    if (pkt.orangeButton) {
        Notify(PSTR("ORANGE "), 0x80);
        newline = true;
    }
    if (pkt.yellowButton) {
        Notify(PSTR("YELLOW "), 0x80);
        newline = true;
    }
    if (pkt.redButton) {
        Notify(PSTR("RED "), 0x80);
        newline = true;
    }
    if (pkt.strumDown) {
        Notify(PSTR("STRUM DOWN "), 0x80);
        newline = true;
    }
    if (pkt.strumUp) {
        Notify(PSTR("STRUM UP "), 0x80);
        newline = true;
    }
    if (pkt.startButton) {
        Notify(PSTR("START "), 0x80);
        newline = true;
    }
    if (pkt.tilt >= (UINT16_MAX / 2)) {
        Notify(PSTR("STAR POWER "), 0x80);
        newline = true;
    }
    if (newline) {
        Notify(PSTR("\r\n "), 0x80);
    }
#endif
    return;
}

void ARDWIINO::printReport() {  // Uncomment "#define PRINTREPORT" to print the report send by the
// Xbox 360 Controller
#ifdef PRINTREPORT
    if (readBuf == NULL)
        return;
    for (uint8_t i = 0; i < XBOX_REPORT_BUFFER_SIZE; i++) {
        D_PrintHex<uint8_t>(readBuf[i], 0x80);
        Notify(PSTR(" "), 0x80);
    }
    Notify(PSTR("\r\n"), 0x80);
#endif
}

/* Xbox Controller commands */
void ARDWIINO::XboxCommand(uint8_t *data, uint16_t nbytes) {
    // bmRequest = Host to device (0x00) | Class (0x20) | Interface (0x01) = 0x21, bRequest = Set
    // Report (0x09), Report ID (0x00), Report Type (Output 0x02), interface (0x00), datalength,
    // datalength, data)
    pUsb->ctrlReq(bAddress, epInfo[XBOX_CONTROL_PIPE].epAddr, bmREQ_HID_OUT, HID_REQUEST_SET_REPORT,
                  0x00, 0x02, 0x00, nbytes, nbytes, data, NULL);
}

void ARDWIINO::setLedRaw(uint8_t value) {
    writeBuf[0] = 0x01;
    writeBuf[1] = 0x03;
    writeBuf[2] = value;

    XboxCommand(writeBuf, 3);
}

void ARDWIINO::setLedOn(LEDEnum led) {
    if (led == OFF)
        setLedRaw(0);
    else if (led != ALL)  // All LEDs can't be on a the same time
        setLedRaw(pgm_read_byte(&XBOX_LEDS[(uint8_t)led]) + 4);
}

void ARDWIINO::setLedBlink(LEDEnum led) {
    setLedRaw(pgm_read_byte(&XBOX_LEDS[(uint8_t)led]));
}

void ARDWIINO::setLedMode(LEDModeEnum ledMode) {  // This function is used to do some special LED
                                                  // stuff the controller supports
    setLedRaw((uint8_t)ledMode);
}

void ARDWIINO::onInit() {
    setLedOn(static_cast<LEDEnum>(LED1));
}

void ARDWIINO::updateGuitarState() {
    if (pkt.command != 0x14)
        return;

    unsigned long current_time = millis();
    if ((current_time - last_update_time) > 10) {
        guitar_state_flags |= changed_flag;
        last_update_time = current_time;
    }
}

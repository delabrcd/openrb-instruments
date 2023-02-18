#include "usbhost.h"
#include "Arduino.h"
uint8_t MAX3421e::vbusState = 0;

/* constructor */
MAX3421e::MAX3421e() {
    setupNewDevices = 0;

    spi_init();
    pinMode(MAX_INT, INPUT);
    pinMode(MAX_GPX, INPUT);

    // activate pullups on INT and GPX
    digitalWrite(MAX_INT, HIGH);
    digitalWrite(MAX_GPX, HIGH);

    pinMode(MAX_SS, OUTPUT);
    digitalWrite(MAX_SS, HIGH);

    // release MAX3421E from reset
    pinMode(MAX_RESET, OUTPUT);
    digitalWrite(MAX_RESET, HIGH);
};

void MAX3421e::regWr(uint8_t reg, uint8_t data) {
    digitalWrite(MAX_SS, LOW);

    SPI_SendByte(reg | 0x02);

    SPI_SendByte(data);
    digitalWrite(MAX_SS, HIGH);
    return;
};
/* multiple-byte write                            */

uint8_t *MAX3421e::bytesWr(uint8_t reg, uint8_t nbytes, uint8_t *data_p) {
    digitalWrite(MAX_SS, LOW);
    SPI_SendByte(reg | 0x02);
    while (nbytes--) {
        // send next data byte
        SPI_SendByte(*data_p);
        data_p++;
    }

    digitalWrite(MAX_SS, HIGH);
    return data_p;
}
/* GPIO write                                           */
/*GPIO byte is split between 2 registers, so two writes are needed to write one byte */
void MAX3421e::gpioWr(uint8_t data) {
    regWr(rIOPINS1, data);
    data >>= 4;
    regWr(rIOPINS2, data);
    return;
}

uint8_t MAX3421e::regRd(uint8_t reg) {
    uint8_t tmp;
    digitalWrite(MAX_SS, LOW);
    SPI_SendByte(reg);
    tmp = SPI_ReceiveByte();
    digitalWrite(MAX_SS, HIGH);

    return tmp;
}
/* multiple-byte register read  */

/* returns a pointer to a memory position after last read   */
uint8_t *MAX3421e::bytesRd(uint8_t reg, uint8_t nbytes, uint8_t *data_p) {
    digitalWrite(MAX_SS, LOW);
    SPI_SendByte(reg);
    while (nbytes) {
        *data_p = SPI_ReceiveByte();
        data_p++;
        nbytes--;
    }

    digitalWrite(MAX_SS, HIGH);
    return data_p;
}
/* GPIO read. See gpioWr for explanation */

/** @brief  Reads the current GPI input values
 *   @retval uint8_t Bitwise value of all 8 GPI inputs
 */
/* GPIN pins are in high nibbles of IOPINS1, IOPINS2    */
uint8_t MAX3421e::gpioRd() {
    uint8_t gpin = 0;
    gpin         = regRd(rIOPINS2);  // pins 4-7
    gpin &= 0xf0;                    // clean lower nibble
    gpin |= (regRd(rIOPINS1) >> 4);  // shift low bits and OR with upper from previous operation.
    return (gpin);
}

/** @brief  Reads the current GPI output values
 *   @retval uint8_t Bitwise value of all 8 GPI outputs
 */
/* GPOUT pins are in low nibbles of IOPINS1, IOPINS2    */
uint8_t MAX3421e::gpioRdOutput() {
    uint8_t gpout = 0;
    gpout         = regRd(rIOPINS1);  // pins 0-3
    gpout &= 0x0f;                    // clean upper nibble
    gpout |= (regRd(rIOPINS2) << 4);  // shift high bits and OR with lower from previous operation.
    return (gpout);
}

/* reset MAX3421E. Returns number of cycles it took for PLL to stabilize after reset
  or zero if PLL haven't stabilized in 65535 cycles */
uint16_t MAX3421e::reset() {
    uint16_t i = 0;
    regWr(rUSBCTL, bmCHIPRES);
    regWr(rUSBCTL, 0x00);
    while (++i) {
        if ((regRd(rUSBIRQ) & bmOSCOKIRQ)) {
            break;
        }
    }
    return (i);
}

/* initialize MAX3421E. Set Host mode, pullups, and stuff. Returns 0 if success, -1 if not */

int8_t MAX3421e::Init() {
    /* Configure full-duplex SPI, interrupt pulse   */
    regWr(rPINCTL, (bmFDUPSPI + bmINTLEVEL + bmGPXB));  // Full-duplex SPI, level interrupt, GPX

    // stop/start the oscillator
    reset();

    /* configure host operation */
    regWr(rMODE, bmDPPULLDN | bmDMPULLDN | bmHOST |
                     bmSEPIRQ);            // set pull-downs, Host, Separate GPIN IRQ on GPX
    regWr(rHIEN, bmCONDETIE | bmFRAMEIE);  // connection detection

    /* check if device is connected */
    regWr(rHCTL, bmSAMPLEBUS);  // sample USB bus
    while (!(regRd(rHCTL) & bmSAMPLEBUS))
        ;                       // wait for sample operation to finish
    busprobe();                 // check if anything is connected
    regWr(rHIRQ, bmCONDETIRQ);  // clear connection detect interrupt
    regWr(rCPUCTL, 0x01);       // enable interrupt pin
    return (0);
}

/* initialize MAX3421E. Set Host mode, pullups, and stuff. Returns 0 if success, -1 if not */
int8_t MAX3421e::Init(int mseconds) {
    /* MAX3421E - full-duplex SPI, level interrupt, vbus off */
    regWr(rPINCTL, (bmFDUPSPI | bmINTLEVEL | GPX_VBDET));

    if (reset() == 0) {  // OSCOKIRQ hasn't asserted in time
        return (-1);
    }

    // Delay a minimum of 1 second to ensure any capacitors are drained.
    // 1 second is required to make sure we do not smoke a Microdrive!
    if (mseconds < 1000)
        mseconds = 1000;
    delay(mseconds);

    regWr(rMODE, bmDPPULLDN | bmDMPULLDN | bmHOST);  // set pull-downs, Host

    regWr(rHIEN, bmCONDETIE | bmFRAMEIE);  // connection detection

    /* check if device is connected */
    regWr(rHCTL, bmSAMPLEBUS);  // sample USB bus
    while (!(regRd(rHCTL) & bmSAMPLEBUS))
        ;  // wait for sample operation to finish

    busprobe();  // check if anything is connected

    regWr(rHIRQ, bmCONDETIRQ);  // clear connection detect interrupt
    regWr(rCPUCTL, 0x01);       // enable interrupt pin

    // GPX pin on. This is done here so that busprobe will fail if we have a switch connected.
    regWr(rPINCTL, (bmFDUPSPI | bmINTLEVEL));

    return (0);
}

/* probe bus to determine device presence and speed and switch host to this speed */
void MAX3421e::busprobe() {
    uint8_t bus_sample;
    bus_sample = regRd(rHRSL);              // Get J,K status
    bus_sample &= (bmJSTATUS | bmKSTATUS);  // zero the rest of the byte
    switch (bus_sample) {                   // start full-speed or low-speed host
        case (bmJSTATUS):
            if ((regRd(rMODE) & bmLOWSPEED) == 0) {
                regWr(rMODE, MODE_FS_HOST);  // start full-speed host
                vbusState = FSHOST;
            } else {
                regWr(rMODE, MODE_LS_HOST);  // start low-speed host
                vbusState = LSHOST;
            }
            break;
        case (bmKSTATUS):
            if ((regRd(rMODE) & bmLOWSPEED) == 0) {
                regWr(rMODE, MODE_LS_HOST);  // start low-speed host
                vbusState = LSHOST;
            } else {
                regWr(rMODE, MODE_FS_HOST);  // start full-speed host
                vbusState = FSHOST;
            }
            break;
        case (bmSE1):  // illegal state
            vbusState = SE1;
            break;
        case (bmSE0):  // disconnected state
            regWr(rMODE, bmDPPULLDN | bmDMPULLDN | bmHOST | bmSEPIRQ);
            vbusState = SE0;
            break;
    }  // end switch( bus_sample )
}

/* MAX3421 state change task and interrupt handler */
uint8_t MAX3421e::Task(void) {
    uint8_t rcode = 0;
    uint8_t pinvalue;
    pinvalue = digitalRead(MAX_INT);
    if (pinvalue == LOW) {
        rcode = IntHandler();
    }

    pinvalue = digitalRead(MAX_GPX);
    if (pinvalue == LOW) {
        GpxHandler();
    }

    return rcode;
}

uint8_t MAX3421e::IntHandler() {
    uint8_t HIRQ;
    uint8_t HIRQ_sendback = 0x00;
    HIRQ                  = regRd(rHIRQ);  // determine interrupt source
    if (HIRQ & bmCONDETIRQ) {
        busprobe();
        HIRQ_sendback |= bmCONDETIRQ;
    }

    /* End HIRQ interrupts handling, clear serviced IRQs */
    regWr(rHIRQ, HIRQ_sendback);
    return HIRQ_sendback;
}

uint8_t MAX3421e::GpxHandler() {
    uint8_t GPINIRQ = regRd(rGPINIRQ);  // read GPIN IRQ register
    return (GPINIRQ);
}

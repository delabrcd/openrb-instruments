#ifndef _USB_HOST_
#define _USB_HOST_

#include <LUFA/Drivers/Peripheral/SPI.h>
#include "max3421e.h"

#define MAX_SS 10
#define MAX_INT 9
#define MAX_GPX 8
#define MAX_RESET 7

typedef enum { vbus_on = 0, vbus_off = GPX_VBDET } VBUS_t;

class MAX3421e {
    static void spi_init() {
        SPI_Init(SPI_MODE_MASTER | SPI_SPEED_FCPU_DIV_2);
    }

public:
    static uint8_t vbusState;

    uint8_t setupNewDevices;

    MAX3421e();
    void     regWr(uint8_t reg, uint8_t data);
    uint8_t *bytesWr(uint8_t reg, uint8_t nbytes, uint8_t *data_p);
    void     gpioWr(uint8_t data);
    uint8_t  regRd(uint8_t reg);
    uint8_t *bytesRd(uint8_t reg, uint8_t nbytes, uint8_t *data_p);
    uint8_t  gpioRd();
    uint8_t  gpioRdOutput();
    uint16_t reset();
    int8_t   Init();
    int8_t   Init(int mseconds);

    void vbusPower(VBUS_t state) {
        regWr(rPINCTL, (bmFDUPSPI | bmINTLEVEL | state));
    }

    uint8_t getVbusState(void) {
        return vbusState;
    };
    void    busprobe();
    uint8_t GpxHandler();
    uint8_t IntHandler();
    uint8_t Task();
};

#endif  // _USB_HOST_
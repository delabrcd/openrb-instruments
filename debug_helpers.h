#pragma once
#include <stdint.h>
#include "adapter_structs.h"

#ifdef SERIAL_DEBUG
const char *getCommandName(int cmd);
void        serialPrintHex(const uint8_t *val, const uint8_t &nval);
void        printPacket(const XBPACKET &packet, const char *descriptor);
#define OUT_DESCRIPTION "OUT: "
#define IN_DESCRIPTION " IN: "
#define DEBUG_USART_BAUDRATE 115200LL
#endif

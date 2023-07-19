#pragma once
#include <stdint.h>
#include "adapter_structs.h"

#ifdef SERIAL_DEBUG
const char *printCommandName(int cmd);
void        serialPrintHex(const uint8_t *val, const uint8_t &nval);
void        printPacket(const xb_packet_t &packet, const char *descriptor);
#define OUT_DESCRIPTION "OUT: "
#define IN_DESCRIPTION " IN: "
#define DEBUG_USART_BAUDRATE 115200LL
#endif

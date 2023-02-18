#pragma once
#include <stdint.h>
#include "adapter_structs.h"

const char *getCommandName(int cmd);
void        SerialPrintHex(const uint8_t *val, const uint8_t &nval);
output_t    outputForNote(const uint8_t &note);
uint8_t     getSequence();
void        printPacket(const XBPACKET &packet, const char *descriptor);
bool        FillPacket(const uint8_t *input, const uint8_t &length, XBPACKET *packet);
void FillInputPacketFromControllerData(const uint8_t *data, const uint8_t &ndata, XBPACKET *packet);

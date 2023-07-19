#pragma once

#include "adapter_structs.h"

output_t outputForNote(const uint8_t &note);
uint8_t  getSequence();
bool     fillPacket(const uint8_t *input, const uint8_t &length, XBPACKET *packet,
                    unsigned long triggered_time = 0);
void fillInputPacketFromControllerData(const uint8_t *data, const uint8_t &ndata, XBPACKET *packet);
void updateDrumStateWithDrumInput(const output_t &out, const uint8_t &state,
                                  DrumInputData *drum_input);
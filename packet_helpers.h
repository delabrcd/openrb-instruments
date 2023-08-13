#pragma once

#include "adapter_structs.h"
#include "ARDWIINO.h"

output_t outputForNote(const uint8_t &note);
uint8_t  getSequence();

bool fillPacket(const uint8_t *input, const uint8_t &length, xb_packet_t *packet,
                unsigned long triggered_time = 0);
void fillInputPacketFromControllerData(const uint8_t *data, const uint8_t &ndata,
                                       xb_packet_t *packet);
void fillInputPacketFromGuitarData(const xb_three_gh_input_pkt_t *guitar_in,
                                   xb_packet_t *packet_out, uint8_t playerId);
void updateDrumStateWithDrumInput(const output_t &out, const uint8_t &state,
                                  xb_one_drum_input_pkt_t *drum_input);

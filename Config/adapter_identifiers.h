#pragma once

#include <stdint.h>
#include <avr/pgmspace.h>

struct xb_packet_t;
void fill_from_pgm(xb_packet_t *packet, const uint8_t *pgm, const uint8_t &size);

namespace identifiers {

int get_announce(xb_packet_t *packet);
int get_n_identify();
int get_identify(const uint8_t &sequence, xb_packet_t *packet);

}  // namespace identifiers

#pragma once
#include "adapter_structs.h"

namespace packet_circ_buf {
xb_packet_t *get_write();
xb_packet_t *get_read();
void         incr_read();
}  // namespace packet_circ_buf
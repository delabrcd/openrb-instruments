#include "packet_circ_buf.h"

namespace packet_circ_buf {
#define OUT_BUF_LEN 6

static uint8_t     write                    = 0;
static uint8_t     read                     = 0;
static xb_packet_t out_packets[OUT_BUF_LEN] = {{.header = {true, 0}}};

xb_packet_t *get_write() {
    xb_packet_t *ret = &out_packets[write];
    write++;
    if (write >= OUT_BUF_LEN)
        write = 0;

    return ret;
}

xb_packet_t *get_read() {
    if (write == read)
        return nullptr;

    xb_packet_t *ret = &out_packets[read];
    return ret;
}

void incr_read() {
    read++;
    if (read >= OUT_BUF_LEN)
        read = 0;
    return;
}
}  // namespace packet_circ_buf

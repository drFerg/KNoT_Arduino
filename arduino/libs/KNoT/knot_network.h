#ifndef KNOT_NETWORK_H
#define KNOT_NETWORK_H

#include "knot_protocol.h"
#include "knot_network_pan.h"
#include "knot_channel_state.h"

/*Change include to suit lower layer network */



int init_knot_network();
int check_seqno(ChannelState *state, DataPayload *dp);
void send_on_knot_channel(ChannelState *state, DataPayload *dp);
void knot_broadcast(ChannelState *state, DataPayload *dp);
void set_broadcast(Address a);
void copy_link_address(ChannelState *state);
void clean_packet(DataPayload *dp);
void ping(ChannelState *state);
void pack_handler(ChannelState *state, DataPayload *dp);
void ping_handler(ChannelState *state, DataPayload *dp);
void close_graceful(ChannelState *state);
void close_handler(ChannelState *state, DataPayload *dp);


#endif /* KNOT_NETWORK_H */
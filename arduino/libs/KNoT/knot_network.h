/*
 * knot_network.h - Basic network functions used by devices.
 * 
 * It calls a facade of functions provided by the link layer 
 * linked at compile time
 * 
 * Author: Fergus William Leahy
 * 
 */

#ifndef KNOT_NETWORK_H
#define KNOT_NETWORK_H
/*Change include to suit lower layer network */
#include "knot_protocol.h"
#include "knot_network_pan.h"
#include "knot_channel_state.h"

/* Macro signifying payload of 0 length */
#define NO_PAYLOAD     0

/* Memsets a Datapayload */
#define clean_packet(dp) (memset(dp, '\0', sizeof(DataPayload)))


/* Initialises the linked in lower level link layer */
int init_knot_network();

/* A helper function to fill in a DataPayload struct */
void dp_complete(DataPayload *dp, uint8_t src, uint8_t dst, 
             uint8_t cmd, uint8_t len);

/* Checks the sequence number and returns 1 if in sequence, 0 otherwise */
int check_seqno(ChannelState *state, DataPayload *dp);

/* Sends DataPayload on a KNoT channel specified in the state */
void send_on_knot_channel(ChannelState *state, DataPayload *dp);

/* Sends a DataPayload as a broadcast transmission */
void knot_broadcast(ChannelState *state, DataPayload *dp);

/* Sets the broadcast address */
void set_broadcast(Address a);

/* ??? */
void copy_link_address(ChannelState *state);

/* Sends a ping packet to the channel in state */
void ping(ChannelState *state);

/* Handles the reception of a PING packet, replies with a PACK */
void ping_handler(ChannelState *state, DataPayload *dp);

/* Handles the reception of a PACK packet */
void pack_handler(ChannelState *state, DataPayload *dp);

/* Closes the channel specified and sends out a DISCONNECT packet */
void close_graceful(ChannelState *state);

/* Handles the reception of a DISCONNECT packet */
void close_handler(ChannelState *state, DataPayload *dp);


#endif /* KNOT_NETWORK_H */
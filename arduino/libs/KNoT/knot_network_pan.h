/*
* author Fergus William Leahy
*/
#ifndef KNOT_NETWORK_PAN_H
#define KNOT_NETWORK_PAN_H

#include <stdio.h>
#include <string.h>

#include "knot_protocol.h"
#include "knot_channel_state.h"

#define DEVICE_ADDRESS 255

typedef void * Address;

/* 
 * Initialises the radio link layer
 */
int init_link_layer();

/* 
 * Sets the device address to addr
 */
void set_dev_addr(int addr);

/* 
 * Returns 1 if a packet is available, 0 otherwise
 */
int packetAvailable();

/*
 * Sends datapayload to the connections default address
 * (good for broadcast)
 */
void send_uni(ChannelState *state, DataPayload *dp);

/*
 * Sends datapayload to the address specified in the channel state
 */
void send_on_channel(ChannelState *state, DataPayload *dp);

/*
 * Broadcasts a DataPayload on the network
 */
void broadcast(ChannelState *state, DataPayload *dp);

/*
 * Resends last packet stored in the state
 */
void resend(ChannelState *state);

int recv_pkt(DataPayload *dp);
//int sleep_recv_pkt(DataPayload *dp);

void copy_address(void *addr1, void *addr2);
void copy_address_broad(void *addr);

#endif /*KNOT_NETWORK_PAN_H*/

/*
* author Fergus William Leahy
*/
#ifndef KNOT_UDP_NETWORK_H
#define KNOT_UDP_NETWORK_H

#include <stdio.h>
#include <string.h>

#include "knot_protocol.h"
#include "knot_channel_state.h"

#define DEVICE_ADDRESS 255

typedef void * Address;
/* 
 * returns fd if successful, -1 otherwise 
 */
int init_link_layer();
void set_dev_addr(int addr);

int packetAvailable();
/*
 * sends datapayload to the connections default address
 * (good for broadcast)
 */
void send_uni(ChannelState *state, DataPayload *dp);

/*
 * sends datapayload to the address specified in the channel state
 */
void send_on_channel(ChannelState *state, DataPayload *dp);

void broadcast(ChannelState *state, DataPayload *dp);

void resend(ChannelState *state);

int recv_pkt(DataPayload *dp);
//int sleep_recv_pkt(DataPayload *dp);

void copy_address(ChannelState *state);
void copy_address_ab(Address a, Address b);
void copy_address_broad(Address a);

#endif /*KNOT_UDP_NETWORK*/

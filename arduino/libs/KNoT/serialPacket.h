#ifndef SERIAL_PACKET_H
#define SERIAL_PACKET_H

void attach_serial(void(*callback)(void), char *pkt);
void recv_serial();
void write_to_serial(char *data, int len);

#endif /* SERIAL_PACKET_H */
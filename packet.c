// Helper functions for the packet to serialize and deserialize onto the network.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "packet.h"

char* serialize_packet(packet_t p)
{
	char* buf = malloc(PACKET_SIZE);
	memcpy(buf, &p.type, 4);
	memcpy(buf+4, &p.packet_num, 4);
	memcpy(buf+8, &p.packet_length, 4);
	memcpy(buf+12, &p.checksum, 4);
	memcpy(buf+16, &p.data, DATA_SIZE);

	return buf;
}

packet_t deserialize_packet(char* s)
{
	packet_t p;

	memcpy(&p.type, s, 4);
	memcpy(&p.packet_num, s+4, 4);
	memcpy(&p.packet_length, s+8, 4);
	memcpy(&p.checksum, s+12, 4);
	memcpy(&p.data, s+16, DATA_SIZE);

	return p;
}

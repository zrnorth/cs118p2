// Helper functions for the packet to serialize and deserialize onto the network.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "packet.h"

char* serialize_packet(packet_t p)
{
	char* buf = malloc(PACKET_SIZE);

	char temp[4];
	sprintf(temp, "%d", p.source_port);
	strncpy(buf, temp, 4);

	sprintf(temp, "%d", p.dest_port);
	strncpy(buf+4, temp, 4);

	sprintf(temp, "%d", p.type);
	strncpy(buf+8, temp, 4);

	sprintf(temp, "%d", p.packet_num);
	strncpy(buf+12, temp, 4);

	sprintf(temp, "%d", p.packet_length);
	strncpy(buf+16, temp, 4);

	sprintf(temp, "%d", p.checksum);
	strncpy(buf+20, temp, 4);

	strncpy(buf+24, p.data, DATA_SIZE);
	return buf;
}

packet_t deserialize_packet(char* s)
{
	packet_t p;

	char buf[4];
	strncpy(buf, s, 4);
	p.source_port = atoi(buf);

	strncpy(buf, s+4, 4);
	p.dest_port = atoi(buf);

	strncpy(buf, s+8, 4);
	p.type = atoi(buf);

	strncpy(buf, s+12, 4);
	p.packet_num = atoi(buf);

	strncpy(buf, s+16, 4);
	p.packet_length = atoi(buf);

	strncpy(buf, s+20, 4);
	p.checksum = atoi(buf);

	strcpy(s+24, p.data);

	return p;
}

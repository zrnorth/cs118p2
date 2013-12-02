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

	buf[8] = p.type;

	sprintf(temp, "%d", p.packet_num);
	strncpy(buf+9, temp, 4);

	sprintf(temp, "%d", p.packet_length);
	strncpy(buf+13, temp, 4);

	sprintf(temp, "%d", p.checksum);
	strncpy(buf+17, temp, 4);

	strncpy(buf+21, p.data, DATA_SIZE);
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

	p.type = s[8];

	strncpy(buf, s+9, 4);
	p.packet_num = atoi(buf);

	strncpy(buf, s+13, 4);
	p.packet_length = atoi(buf);

	strncpy(buf, s+17, 4);
	p.checksum = atoi(buf);

	strcpy(s+21, p.data);

	return p;
}

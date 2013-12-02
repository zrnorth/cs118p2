// Helper functions for the packet to serialize and deserialize onto the network.

#include <stdlib.h>
#include "packet.h"

char* serialize_packet(struct PACKET p)
{
  char buf[1000];
  buf[0] = p.source_port;
  buf[4] = p.dest_port;
  buf[8] = p.type;
  buf[9] = p.packet_num;
  buf[13] = p.packet_length;
  buf[17] = p.checksum;
  strncpy(&buf[20], p.data, DATA_SIZE);
}
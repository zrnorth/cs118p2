/* This file contains the struct of the packet we are using for the project.
   Packet size is 1000 bytes. The layout is as follows

    BYTE #      NAME              DESCRIPTION
    0-3        Type              The type of packet being sent (Request, Message, or ACK)
                                  - Request: Value 0x00. This indicates the data field will
                                        contain a filename request.
                                  - Message: Value 0x01. Indicates the packet is a data
                                        message from the server to a receiver.
                                  - ACK: Value 0x10. Indicates the receiver is sending
                                        an ACK to the sender in the Packet # field.
    4-7       Packet #          If the type of packet is a message, this indicates
                                  which packet number is being sent (starting at pkt 0.)
                                  If the type of packet is an ACK, this indicates which
                                  packet number is being acknowledged.
    8-11       Length            Indicates the length in bytes of the packet.
    12-15       Checksum          Validity checksum. The server compares each sent message's
                                  checksum with the ACK's computed checksum to determine
                                  if the message was received correctly.
    26-999      Data              The actual data in the packet.
*/

#define PACKET_SIZE   1000
#define HEADER_SIZE   16 // size of the non-data elements of a packet
#define DATA_SIZE (PACKET_SIZE - HEADER_SIZE)
#define TYPE_REQUEST  0
#define TYPE_MESSAGE  1
#define TYPE_ACK      2

typedef struct PACKET {
  unsigned int type;
  unsigned int packet_num;
  unsigned int packet_length;
  unsigned int checksum;
  char data[DATA_SIZE];
} packet_t;

char* serialize_packet(packet_t);
packet_t deserialize_packet(char*);

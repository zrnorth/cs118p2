/* This file contains the struct of the packet we are using for the project.
   Packet size is 1000 bytes. The layout is as follows

    BYTE #      NAME              DESCRIPTION
    0-3         Source Port       The source port of the packet.
    4-7         Dest Port         The destination port of the packet.
    8           Type              The type of packet being sent (Request, Message, or ACK)
                                  - Request: Value 0x00. This indicates the data field will
                                        contain a filename request.
                                  - Message: Value 0x01. Indicates the packet is a data
                                        message from the server to a receiver.
                                  - ACK: Value 0x10. Indicates the receiver is sending
                                        an ACK to the sender in the Packet # field.
    9-12        Packet #          If the type of packet is a message, this indicates
                                  which packet number is being sent (starting at pkt 0.)
                                  If the type of packet is an ACK, this indicates which
                                  packet number is being acknowledged.
    13-16       Length            Indicates the length in bytes of the packet.
    17-20       Checksum          Validity checksum. The server compares each sent message's
                                  checksum with the ACK's computed checksum to determine
                                  if the message was received correctly.
    21-999      Data              The actual data in the packet.
*/

struct PACKET {
  unsigned int source_port;
  unsigned int dest_port;
  char type;
  unsigned int packet_num;
  unsigned int packet_length;
  unsigned int checksum;
  char data[978];
}




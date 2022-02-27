#ifndef PACKET_H_HEADER_GUARD
#define PACKET_H_HEADER_GUARD

#define PACKET_MAX_LEN 32

struct Packet {
  unsigned char id;
  unsigned char len;
  unsigned char dat[PACKET_MAX_LEN];
  unsigned char state;
};

/**
 * @brief Initializes a packet.
*/
void Packet_init(Packet* packet);

/**
 * @brief Parses the next byte in a packet.
 * @param packet Reference to packet.
 * @param nextDat Next byte received on the wire.
 * @return True iff packet finished parsing.
*/
bool Packet_parse(Packet* packet, char nextDat);

#endif

#include "packet.h"
#include "Arduino.h"

/**
 * Packet state: If escape bit is set, next byte is escaped. Remaining bits follow PacketState enum.
*/
#define PACKET_START_BYTE 0x7E
#define PACKET_ESCAPE_BYTE 0xA5
#define PACKET_ESCAPE_BIT 0b10000000
#define PACKET_CRC_POLY 0x5b // https://users.ece.cmu.edu/~koopman/crc/ 8 bit CRC with good msg length, max HD is 4

enum PacketState : unsigned char {
  // values 0 thru PACKET_MAX_LEN - 1 indicate the position in the data array
  PacketState_CHECKSUM = PACKET_MAX_LEN,

  // Below states represent what byte is about to be read. Separate start state is thus unneeded
  PacketState_ID,
  PacketState_LENGTH,
  PacketState_ERROR, // doubles as start state
};

void Packet_init(Packet* packet) {
    packet->state = PacketState_ERROR;
}

/**
 * @brief Parses the next byte in a packet.
 * @param packet Reference to packet.
 * @param nextDat Next byte received on the wire.
 * @return True iff packet finished parsing.
*/
bool Packet_parse(Packet* packet, char nextDat) {

    if (nextDat == PACKET_START_BYTE) {
        // Unescaped start byte - discard existing data and restart
        packet->state = static_cast<unsigned char>(PacketState_ID);
        return false;
    }

    if (packet->state & PACKET_ESCAPE_BIT) {
        // Last byte was escape byte
        nextDat ^= PACKET_ESCAPE_BYTE;
        packet->state &= ~PACKET_ESCAPE_BIT;
    } else if (nextDat == PACKET_ESCAPE_BYTE) {
        // This byte is escape byte - early return
        packet->state |= PACKET_ESCAPE_BIT;
        return false;
    }

    if (packet->state < PACKET_MAX_LEN) {
        // Read packet data
        packet->dat[packet->state] = nextDat;
        ++packet->state;
        if(packet->state == packet->len) {
            packet->state = PacketState_CHECKSUM;
        }
        return false;
    }

    switch(packet->state) {
    case PacketState_ID:
        packet->id = nextDat;
        packet->state = PacketState_LENGTH;
        return false;
    case PacketState_LENGTH:
        if (nextDat > PACKET_MAX_LEN) {
            // invalid - discard packet
            packet->state = PacketState_ERROR;
            return false;
        }
        packet->len = static_cast<unsigned char>(nextDat);
        packet->state = 0;
        return false;
    case PacketState_CHECKSUM:
        // TODO: compute checksom
        packet->state = PacketState_ERROR; // start anew
        return true;
    case PacketState_ERROR:
    default:
        return false;
    }
    return false;
}


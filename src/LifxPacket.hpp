#include <Arduino.h>

#if 0
struct LifxPacket_old {
  uint16_t size; //little endian
  uint16_t protocol; //little endian
  uint32_t reserved1;
  byte bulbAddress[6];
  uint16_t reserved2;
  byte site[6];
  uint16_t reserved3;
  uint64_t timestamp;
  uint16_t packet_type; //little endian
  uint16_t reserved4;
  
  byte data[128];
  int data_size;
};
#endif

/**
 * Lifx Frame structure
 * 
 * @see https://lan.developer.lifx.com/docs/header-description#frame
 */
struct LifxFrame {
    uint16_t size;
    union {
        uint16_t raw_protocol;
        struct {
            uint16_t protocol:12;
            uint8_t  addressable:1;
            uint8_t  tagged:1;
            uint8_t  origin:2;
        };
    };
    uint32_t source;
};

/**
 * Lifx frame address structure
 * 
 * @see https://lan.developer.lifx.com/docs/header-description#frame-address
 */
struct LifxFrameAddress {
    uint8_t target[8];
    uint8_t reserved0[6];
    uint8_t res_required : 1;
    uint8_t ack_required : 1;
    uint8_t reserved1 : 6;
    uint8_t sequence;
};

/**
 * Lifx protocol header structure
 * 
 * @see https://lan.developer.lifx.com/docs/header-description#protocol-header
 */
#pragma pack(push, 1)
struct LifxProtocolHeader {
    uint64_t reserved0;
    uint16_t type;
    uint16_t reserved1;
};
#pragma pack(pop)

struct LifxPacket {
    // Always stored in network endian
    LifxFrame frame;
    LifxFrameAddress frameAddress;
    LifxProtocolHeader protocolHeader;

    // Setters
    void init();
};

/**
 * Wrapper around the Lifx packet structure
 * 
 * This wrapper allows easier manipulation of the packet,
 * both incoming and outgoing.
 */
class LifxPacketWrapper {
    protected:
        LifxPacket* packet;

    public:
        LifxPacketWrapper(byte* buffer);

        void dump();

        uint16_t getSize();
        uint16_t getPayloadSize();
        uint16_t getType();

};

#include <Arduino.h>

#define LOG_OFF 0
#define LOG_FATAL 1
#define LOG_ERROR 2
#define LOG_WARN 3
#define LOG_INFO 4
#define LOG_DEBUG 5
#define LOG_TRACE 6
#define LOG_ALL 7

/**
 * Lifx Frame structure
 * 
 * @see https://lan.developer.lifx.com/docs/header-description#frame
 */
#pragma pack(push, 1)
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
#pragma pack(pop)

/**
 * Lifx frame address structure
 * 
 * @see https://lan.developer.lifx.com/docs/header-description#frame-address
 */
#pragma pack(push, 1)
struct LifxFrameAddress {
    uint8_t target[8];
    uint8_t reserved0[6];
    uint8_t res_required : 1;
    uint8_t ack_required : 1;
    uint8_t reserved1 : 6;
    uint8_t sequence;
};
#pragma pack(pop)

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

#pragma pack(push, 1)
struct LifxPacket {
    // Always stored in network endian
    LifxFrame frame;
    LifxFrameAddress frameAddress;
    LifxProtocolHeader protocolHeader;

    // Setters
    void init();
};
#pragma pack(pop)


#define CODE_TO_STR(code) case (code) : return #code
#define CODE_INVALID_STR "!! invalid !!"

struct LifxPacketType {
    enum Code : uint16_t {
        INVALID = 0,

        GET_SERVICE = 2,
        STATE_SERVICE = 3,

        STATE_GROUP = 53,

        ECHO_RESPONSE = 59,

        STATE = 107,
    };

    static const char* toStr(Code code) {
        switch (code) {
            case (INVALID) : return CODE_INVALID_STR;

            CODE_TO_STR(GET_SERVICE);
            CODE_TO_STR(STATE_SERVICE);

            CODE_TO_STR(STATE_GROUP);

            CODE_TO_STR(ECHO_RESPONSE);

            CODE_TO_STR(STATE);

            default: {
                Serial.printf("\nWARNING: I don't know what packet type %i = 0x%04X is...\n", code, code);
                return CODE_INVALID_STR;
            }
        }
    }

    static boolean isRequest(Code code) {
        switch(code) {
            case (GET_SERVICE):

                return true;

            default:
                return false;
        }
    }
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

        boolean isToBeDumped();
        void dump();

        void handle(byte mac[WL_MAC_ADDR_LENGTH], WiFiUDP& Udp);

        uint16_t getSize();
        uint16_t getPayloadSize();
        LifxPacketType::Code getType();
        uint32_t getSource();
        uint8_t getSequence();

        void* getPayload();

        void initResponse(LifxPacketWrapper* pRequest, byte mac[WL_MAC_ADDR_LENGTH], LifxPacketType::Code, size_t payload_len);

        static size_t getResponseSize(size_t size_payload) { return sizeof(LifxPacket) + size_payload; };

        uint32_t sendUDP(WiFiUDP& Udp);
    private:
        
};

class LifxHandler {
    protected:
        WiFiUDP wifiUDP;
        LifxPacketWrapper* pRequest;
        byte local_mac[WL_MAC_ADDR_LENGTH];

    public :
        LifxHandler(byte mac[WL_MAC_ADDR_LENGTH], WiFiUDP& Udp, LifxPacketWrapper* incoming) : 
            wifiUDP(Udp),
            pRequest{incoming} {
                memcpy(local_mac, mac, sizeof(mac)/sizeof(mac[0]));
            };

        virtual void handle() = 0;
};

class LifxHandlerService : public LifxHandler {
    private:
        static uint8_t const service_UDP = 1; // @see https://lan.developer.lifx.com/docs/device-messages#section-service

        /**
         * Response structure
         * 
         * @see https://lan.developer.lifx.com/docs/device-messages#section-stateservice-3
         */
        #pragma pack(push, 1)
        struct LifxServiceState {
            uint8_t service;
            uint32_t port;
        };
        #pragma pack(pop)

    public:
        LifxHandlerService(byte mac[WL_MAC_ADDR_LENGTH], WiFiUDP& Udp, LifxPacketWrapper* incoming) : LifxHandler(mac, Udp, incoming) {};
        void handle();
};
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <lifx.h>

#define LOG_OFF 0
#define LOG_FATAL 1
#define LOG_ERROR 2
#define LOG_WARN 3
#define LOG_INFO 4
#define LOG_DEBUG 5
#define LOG_TRACE 6
#define LOG_ALL 7

#define LIFX_PORT 56700

extern LifxEEPROM eeprom;

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

        STATE_HOST_INFO = 13,
        GET_HOST_FIRMWARE = 14,
        STATE_HOST_FIRMWARE = 15,

        STATE_WIFI_INFO = 17,
        GET_WIFI_FIRMWARE = 18,
        STATE_WIFI_FIRMWARE = 19,

        GET_LABEL = 23,
        SET_LABEL = 24,
        STATE_LABEL = 25,

        GET_VERSION = 32,
        STATE_VERSION = 33,

        STATE_GROUP = 53,

        ECHO_RESPONSE = 59,

        STATE = 107,
    };

    static const char* toStr(Code code) {
        switch (code) {
            case (INVALID) : return CODE_INVALID_STR;

            CODE_TO_STR(GET_SERVICE);
            CODE_TO_STR(STATE_SERVICE);

            CODE_TO_STR(STATE_HOST_INFO);
            CODE_TO_STR(GET_HOST_FIRMWARE);
            CODE_TO_STR(STATE_HOST_FIRMWARE);

            CODE_TO_STR(STATE_WIFI_INFO);
            CODE_TO_STR(GET_WIFI_FIRMWARE);
            CODE_TO_STR(STATE_WIFI_FIRMWARE);

            CODE_TO_STR(GET_LABEL);
            CODE_TO_STR(SET_LABEL);
            CODE_TO_STR(STATE_LABEL);

            CODE_TO_STR(GET_VERSION);
            CODE_TO_STR(STATE_VERSION);

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
            case (GET_WIFI_FIRMWARE):
            case (GET_HOST_FIRMWARE):
            case (GET_LABEL):
            case (SET_LABEL):

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
                memcpy(local_mac, mac, WL_MAC_ADDR_LENGTH);
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

class LifxHandlerHostFirmware : public LifxHandler {
    private:

        /**
         * Response structure
         * 
         * @see https://lan.developer.lifx.com/docs/device-messages#section-statehostfirmware-15
         */
        #pragma pack(push, 1)
        struct LifxHostFirmwareState {
            uint64_t build;
            uint64_t reserved;
            uint16_t version_minor;
            uint16_t version_major;
        };
        #pragma pack(pop)

    public:
        LifxHandlerHostFirmware(byte mac[WL_MAC_ADDR_LENGTH], WiFiUDP& Udp, LifxPacketWrapper* incoming) : LifxHandler(mac, Udp, incoming) {};
        void handle();
};

class LifxHandlerWifiFirmware : public LifxHandler {
    private:

        /**
         * Response structure
         * 
         * @see https://lan.developer.lifx.com/docs/device-messages#section-statewififirmware-19
         */
        #pragma pack(push, 1)
        struct LifxWifiFirmwareState {
            uint64_t build;
            uint64_t reserved;
            uint16_t version_minor;
            uint16_t version_major;
        };
        #pragma pack(pop)

    public:
        LifxHandlerWifiFirmware(byte mac[WL_MAC_ADDR_LENGTH], WiFiUDP& Udp, LifxPacketWrapper* incoming) : LifxHandler(mac, Udp, incoming) {};
        void handle();
};

class LifxHandlerLabel : public LifxHandler {
    private:

        /**
         * Response structure
         * 
         * @see https://lan.developer.lifx.com/docs/device-messages#section-statelabel-25
         */
        #pragma pack(push, 1)
        struct LifxLabelState {
            char label[LIFX_LABEL_LENGTH];
        };
        #pragma pack(pop)

    public:
        LifxHandlerLabel(byte mac[WL_MAC_ADDR_LENGTH], WiFiUDP& Udp, LifxPacketWrapper* incoming) : LifxHandler(mac, Udp, incoming) {};
        void handle();
};

class LifxHandlerVersion : public LifxHandler {
    private:

        /**
         * Response structure
         * 
         * @see https://lan.developer.lifx.com/docs/device-messages#section-stateversion-33
         */
        #pragma pack(push, 1)
        struct LifxVersionState {
            uint32_t vendor;
            uint32_t product;
            uint32_t version;
        };
        #pragma pack(pop)

    public:
        LifxHandlerVersion(byte mac[WL_MAC_ADDR_LENGTH], WiFiUDP& Udp, LifxPacketWrapper* incoming) : LifxHandler(mac, Udp, incoming) {};
        void handle();
};

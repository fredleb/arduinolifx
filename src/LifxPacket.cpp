#include "LifxPacket.hpp"
#include "util.h"

// Here you can tweak the boolean answer to make them more visible in the output.
#define DISPLAY_BOOLEAN(x) ((x) ? "Y" : "_")

void LifxPacket::init() {
    frame.protocol = htons(1024U);
    frame.addressable = 1;
    frame.origin = 0;

    memset(frameAddress.reserved0, 0, sizeof(frameAddress.reserved0));
}

LifxPacketWrapper::LifxPacketWrapper(byte* buffer) {
    packet = (LifxPacket* )buffer;
}

uint16_t LifxPacketWrapper::getSize() {
    if (packet != NULL) {
        return ntohs(packet->frame.size);
    }
    return 0;
}

uint16_t LifxPacketWrapper::getPayloadSize() {
    return getSize() - sizeof(LifxPacket);
}

LifxPacketType::Code LifxPacketWrapper::getType() {
    if (packet != NULL) {
        return (LifxPacketType::Code)(packet->protocolHeader.type);
    }
    return LifxPacketType::Code::INVALID;
}

}

#define SPACE " "

void LifxPacketWrapper::dump() {
    if (packet == NULL) {
        Serial.println("! Packet buffer is not allocated !");
    } else {
        // size
        Serial.printf("size %i", getSize());

        // protocol (always the same so don't print)
        //Serial.printf(", protocol %i", packet->frame.protocol);
            // TODO ? ignored field
        Serial.printf(", tagged %s", DISPLAY_BOOLEAN(packet->frame.tagged));
            // TODO ? ignored field
        Serial.printf(", source 0x%08X", packet->frame.source);

        // bulbAddress mac address
        Serial.print(", target");
        for (int i = 0; i < sizeof(packet->frameAddress.target); i++) {
            Serial.printf(" %02X", lowByte(packet->frameAddress.target[i]));
        }

        Serial.printf(", res_required %s", DISPLAY_BOOLEAN(packet->frameAddress.res_required));
        Serial.printf(", ack_required %s", DISPLAY_BOOLEAN(packet->frameAddress.ack_required));
        Serial.printf(", sequence %i", packet->frameAddress.sequence);

        Serial.printf(", type %i", packet->protocolHeader.type);

        if (getPayloadSize()) {
            Serial.printf(", payload (%i) ", getPayloadSize());
            for(int i = 0; i < getPayloadSize(); i++) {
                Serial.printf("%02X ", *((char*)(packet + 1) + i));
            }
        }
    }
}

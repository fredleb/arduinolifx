#include "LifxPacket.hpp"
#include "util.h"

// Here you can tweak the boolean answer to make them more visible in the output.
#define DISPLAY_BOOLEAN(x) ((x) ? "Y" : "_")

void LifxPacket::init() {
    memset(this, 0, sizeof(LifxPacket));

    frame.protocol = 1024U;
    frame.addressable = 1;
}

LifxPacketWrapper::LifxPacketWrapper(byte* buffer) {
    packet = (LifxPacket* )buffer;
}

uint16_t LifxPacketWrapper::getSize() {
    if (packet != NULL) {
        return packet->frame.size;
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

void* LifxPacketWrapper::getPayload() {
    if (packet != NULL) {
        return (void*)(packet + 1);
    }
    return NULL;
}

uint32_t LifxPacketWrapper::getSource() {
    if (packet != NULL) {
        return packet->frame.source;
    }
    return 0;
}

uint8_t LifxPacketWrapper::getSequence() {
    if (packet != NULL) {
        return packet->frameAddress.sequence;
    }
    return 0;
}

uint32_t LifxPacketWrapper::sendUDP(WiFiUDP& Udp) {
    if (DEBUG >= LOG_INFO) {
        Serial.printf("<UDP ");
        dump();
        Serial.println();
    }

    // broadcast packet on local subnet
    IPAddress remote_addr(Udp.remoteIP());
    IPAddress broadcast_addr(remote_addr[0], remote_addr[1], remote_addr[2], 255);

    Udp.beginPacket(broadcast_addr, Udp.remotePort());
    size_t written = Udp.write((const char *)packet, getSize());
    Udp.endPacket();

    return (uint32_t) written;
}

boolean LifxPacketWrapper::isToBeDumped() {
    if (DEBUG >= LOG_TRACE) {
        return true;
    }

    if (DEBUG >= LOG_DEBUG) {
        if (LifxPacketType::isRequest(getType())) {
            return true;
        }
    }
    return false;
}

#define SPACE " "

void LifxPacketWrapper::dump() {
    if (packet == NULL) {
        Serial.printf("! Packet buffer is not allocated !\n");
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
        Serial.printf(", target");
        for (int i = 0; i < sizeof(packet->frameAddress.target); i++) {
            Serial.printf(" %02X", lowByte(packet->frameAddress.target[i]));
        }

        Serial.printf(", res_required %s", DISPLAY_BOOLEAN(packet->frameAddress.res_required));
        Serial.printf(", ack_required %s", DISPLAY_BOOLEAN(packet->frameAddress.ack_required));
        Serial.printf(", sequence %i", packet->frameAddress.sequence);

        Serial.printf(", type %s", LifxPacketType::toStr(getType()));

        if (getPayloadSize()) {
            Serial.printf(", payload (%i) ", getPayloadSize());
            for(int i = 0; i < getPayloadSize(); i++) {
                Serial.printf("%02X ", *((char*)(packet + 1) + i));
            }
        }
    }
}

void LifxPacketWrapper::initResponse(LifxPacketWrapper* pRequest, byte mac[WL_MAC_ADDR_LENGTH], LifxPacketType::Code code, size_t payload_len) {
    packet->init();
    packet->frame.size = getResponseSize(payload_len);
    packet->frame.source = pRequest->getSource();
    packet->frameAddress.sequence = pRequest->getSequence();
    packet->protocolHeader.type = (uint16_t)code;

    memcpy(packet->frameAddress.target, mac, WL_MAC_ADDR_LENGTH);
}

void LifxPacketWrapper::handle(byte mac[WL_MAC_ADDR_LENGTH], WiFiUDP& Udp) {
    if (DEBUG >= LOG_INFO) {
        if (isToBeDumped()) {
            Serial.print(F(">UDP "));
            dump();
            Serial.println();
        }
    }

    switch(getType()) {
        case (LifxPacketType::Code::GET_SERVICE) : {
            LifxHandlerService handler(mac, Udp, this);
            handler.handle();
        } break;

    }
}

void LifxHandlerService::handle() {
    size_t packetLen = LifxPacketWrapper::getResponseSize(sizeof(LifxServiceState));
    byte* buffer = (byte*)malloc(packetLen);
    LifxPacketWrapper response(buffer);
    response.initResponse(pRequest, local_mac, LifxPacketType::Code::STATE_SERVICE, sizeof(LifxServiceState));

    LifxServiceState* pServiceState = (LifxServiceState*) response.getPayload();
    pServiceState->port = LIFX_PORT;
    pServiceState->service = service_UDP;

    response.sendUDP(wifiUDP);
    free(buffer);
}
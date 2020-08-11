#include "LifxPacket.hpp"
#include "util.h"

// TODO: remove this horror
extern long power_status;
extern long hue;
extern long sat;
extern long bri;
extern long kel;

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

    if (!LifxPacketType::isRequest(getType())) {
        // it's not a request, ignore it.
        return;
    }

    switch (getType()) {
        case (LifxPacketType::Code::GET_SERVICE) : {
            LifxHandler<LifxProtocol::LifxServiceState> handler(mac, Udp, this);
            handler.handle(LifxPacketType::Code::STATE_SERVICE, [](LifxProtocol::LifxServiceState * pState) {
                    pState->port = LIFX_PORT;
                    pState->service = LifxProtocol::service_UDP;
                } );
        } break;

        case (LifxPacketType::Code::GET_HOST_FIRMWARE) : {
            LifxHandler<LifxProtocol::LifxHostFirmwareState> handler(mac, Udp, this);
            handler.handle(LifxPacketType::Code::STATE_HOST_FIRMWARE, [](LifxProtocol::LifxHostFirmwareState * pState) {
                    pState->build = 0;
                    pState->version_minor = 5;
                    pState->version_major = 1;
                } );
        } break;

        case (LifxPacketType::Code::GET_WIFI_INFO) : {
            LifxHandler<LifxProtocol::LifxWifiInfoState> handler(mac, Udp, this);
            handler.handle(LifxPacketType::Code::STATE_WIFI_INFO, [](LifxProtocol::LifxWifiInfoState * pState) {
                    pState->signal = 10;
                    pState->rx = 1024;
                    pState->tx = 1024;
                } );
        } break;

        case (LifxPacketType::Code::GET_WIFI_FIRMWARE) : {
            LifxHandler<LifxProtocol::LifxWifiFirmwareState> handler(mac, Udp, this);
            handler.handle(LifxPacketType::Code::STATE_WIFI_FIRMWARE, [](LifxProtocol::LifxWifiFirmwareState * pState) {
                    pState->build = 0;
                    pState->version_minor = 1;
                    pState->version_major = 2;
                } );
        } break;

        case (LifxPacketType::Code::GET_LABEL) : {
            LifxHandler<LifxProtocol::LifxLabelState> handler(mac, Udp, this);
            handler.handle(LifxPacketType::Code::STATE_LABEL, [](LifxProtocol::LifxLabelState * pState) {
                memcpy(pState->label, eeprom.sLabel, LIFX_LABEL_LENGTH);
            } );
        } break;

        case (LifxPacketType::Code::GET_VERSION) : {
            LifxHandler<LifxProtocol::LifxVersionState> handler(mac, Udp, this);
            handler.handle(LifxPacketType::Code::STATE_VERSION, [](LifxProtocol::LifxVersionState * pState) {
                    pState->vendor = 1;
                    pState->product = 1;
                    pState->version = 1;
            } );
        } break;

        case (LifxPacketType::Code::GET_LOCATION) : {
            LifxHandler<LifxProtocol::LifxLocationState> handler(mac, Udp, this);
            handler.handle(LifxPacketType::Code::STATE_LOCATION, [](LifxProtocol::LifxLocationState * pState) {
                memcpy(pState->location, eeprom.sLocation, LIFX_LOCATION_LENGTH);
                memcpy(pState->label, eeprom.sLabel, LIFX_LABEL_LENGTH);
                pState->updated_at = eeprom.location_updated_at;
            } );
        } break;

        case (LifxPacketType::Code::GET_GROUP) : {
            LifxHandler<LifxProtocol::LifxGroupState> handler(mac, Udp, this);
            handler.handle(LifxPacketType::Code::STATE_GROUP, [](LifxProtocol::LifxGroupState * pState) {
                memcpy(pState->group, eeprom.sGroup, LIFX_GROUP_LENGTH);
                memcpy(pState->label, eeprom.sLabel, LIFX_LABEL_LENGTH);
                pState->updated_at = eeprom.group_updated_at;
            } );
        } break;

        case (LifxPacketType::Code::GET) : {
            LifxHandler<LifxProtocol::LifxState> handler(mac, Udp, this);
            handler.handle(LifxPacketType::Code::STATE, [](LifxProtocol::LifxState * pState) {

                // TODO replace these globals
                pState->color.hue = hue;
                pState->color.brightness = bri;
                pState->color.saturation = sat;
                pState->color.kelvin = kel;
                pState->power = power_status;

                memcpy(pState->label, eeprom.sLabel, LIFX_LABEL_LENGTH);
            } );
        } break;

        case (LifxPacketType::Code::SET_POWER) : {
            LifxProtocol::LifxPowerSet * pSet = (LifxProtocol::LifxPowerSet *)getPayload();
            power_status = pSet->level;
            if ( packet->frameAddress.ack_required) {
                LifxHandler<void> handler(mac, Udp, this);
                handler.handleEmpty(LifxPacketType::Code::ACKNOWLEDGEMENT);
            }
            /*
            if ( ! packet->frameAddress.res_required) {
                break;
            }
            */
        } // no break

        case (LifxPacketType::Code::GET_POWER) : {
            LifxHandler<LifxProtocol::LifxPowerState> handler(mac, Udp, this);
            handler.handle(LifxPacketType::Code::STATE_POWER, [](LifxProtocol::LifxPowerState * pState) {
                pState->power = power_status;
            } );
        } break;

        default : {
            Serial.printf("\nERROR: not handled type 0x%04X = %i\n", getType(), getType());
        }
    }
}

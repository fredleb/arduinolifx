/*
 LIFX bulb emulator by Kayne Richens (kayno@kayno.net)
 
 Emulates a LIFX bulb. Connect an RGB LED (or LED strip via drivers) 
 to redPin, greenPin and bluePin as you normally would on an 
 ethernet-ready Arduino and control it from the LIFX app!
 
 Notes:
 - Only one client (e.g. app) can connect to the bulb at once
 
 Set the following variables below to suit your Arduino and network 
 environment:
 - mac (unique mac address for your arduino)
 - redPin (PWM pin for RED)
 - greenPin  (PWM pin for GREEN)
 - bluePin  (PWM pin for BLUE)
 
 Made possible by the work of magicmonkey: 
 https://github.com/magicmonkey/lifxjs/ - you can use this to control 
 your arduino bulb as well as real LIFX bulbs at the same time!
 
 And also the RGBMood library by Harold Waterkeyn, which was modified
 slightly to support powering down the LED
 */

#include <Arduino.h>

#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>

#include "lifx.h"
#include "LifxPacket.hpp"
#include "RGBMoodLifx.h"
#include "color.h"

// set to 1 to output debug messages (including packet dumps) to serial (38400 baud)
#ifndef DEBUG
#define DEBUG 0
#endif

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 
  0xDE, 0xAD, 0xDE, 0xAD, 0xDE, 0xAD };
byte site_mac[] = { 
  0x4c, 0x49, 0x46, 0x58, 0x56, 0x32 }; // spells out "LIFXV2" - version 2 of the app changes the site address to this...

// pins for the RGB LED:
const int redPin = 3;
const int greenPin = 5;
const int bluePin = 6;

// tags for this bulb
char bulbTags[LifxBulbTagsLength] = {
  0,0,0,0,0,0,0,0};
char bulbTagLabels[LifxBulbTagLabelsLength] = "";

// initial bulb values - warm white!
long power_status = 65535;
long hue = 0;
long sat = 0;
long bri = 65535;
long kel = 2000;
long dim = 0;


// Whole config
LifxEEPROM eeprom;

// Ethernet instances, for UDP broadcasting, and TCP server and client
WiFiUDP Udp;
WiFiServer TcpServer(LifxPort);
WiFiClient client;

RGBMoodLifx LIFXBulb(redPin, greenPin, bluePin);


void setLight(int origin) {
  if (DEBUG) {
    Serial.print(F("Set light - "));
    Serial.print(" origin: ");
    Serial.print(origin);
    Serial.print(" - ");

    Serial.print(F("hue: "));
    Serial.print(hue);
    Serial.print(F(", sat: "));
    Serial.print(sat);
    Serial.print(F(", bri: "));
    Serial.print(bri);
    Serial.print(F(", kel: "));
    Serial.print(kel);
    Serial.print(F(", power: "));
    Serial.print(power_status);
    Serial.println(power_status ? " (on)" : " (off)");
  }

  if(power_status) {
    int this_hue = map(hue, 0, 65535, 0, 359);
    int this_sat = map(sat, 0, 65535, 0, 255);
    int this_bri = map(bri, 0, 65535, 0, 255);

    // if we are setting a "white" colour (kelvin temp)
    if(kel > 0 && this_sat < 1) {
      // convert kelvin to RGB
      rgb kelvin_rgb;
      kelvin_rgb = kelvinToRGB(kel);

      // convert the RGB into HSV
      hsv kelvin_hsv;
      kelvin_hsv = rgb2hsv(kelvin_rgb);

      // set the new values ready to go to the bulb (brightness does not change, just hue and saturation)
      this_hue = kelvin_hsv.h;
      this_sat = map(kelvin_hsv.s*1000, 0, 1000, 0, 255); //multiply the sat by 1000 so we can map the percentage value returned by rgb2hsv
    }

    LIFXBulb.fadeHSB(this_hue, this_sat, this_bri);
  } 
  else {
    LIFXBulb.fadeHSB(0, 0, 0);
  }
}


// print out a LifxPacket data structure as a series of hex bytes - used for DEBUG
void printLifxPacket(LifxPacket &pkt) {
  // size
  Serial.print(lowByte(LifxPacketSize + pkt.data_size), HEX);
  Serial.print(SPACE);
  Serial.print(highByte(LifxPacketSize + pkt.data_size), HEX);
  Serial.print(SPACE);

  // protocol
  Serial.print(lowByte(pkt.protocol), HEX);
  Serial.print(SPACE);
  Serial.print(highByte(pkt.protocol), HEX);
  Serial.print(SPACE);

  // reserved1
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);

  // bulbAddress mac address
  for(int i = 0; i < sizeof(mac); i++) {
    Serial.print(lowByte(mac[i]), HEX);
    Serial.print(SPACE);
  }

  // reserved2
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);

  // site mac address
  for(int i = 0; i < sizeof(site_mac); i++) {
    Serial.print(lowByte(site_mac[i]), HEX);
    Serial.print(SPACE);
  }

  // reserved3
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);

  // timestamp
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);

  //packet type
  Serial.print(lowByte(pkt.packet_type), HEX);
  Serial.print(SPACE);
  Serial.print(highByte(pkt.packet_type), HEX);
  Serial.print(SPACE);

  // reserved4
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);

  //data
  for(int i = 0; i < pkt.data_size; i++) {
    Serial.print(pkt.data[i], HEX);
    Serial.print(SPACE);
  }
}


unsigned int sendUDPPacket(LifxPacket &pkt) { 
  // broadcast packet on local subnet
  IPAddress remote_addr(Udp.remoteIP());
  IPAddress broadcast_addr(remote_addr[0], remote_addr[1], remote_addr[2], 255);

  if (DEBUG >= 3) {
    Serial.print(F("+UDP "));
    printLifxPacket(pkt);
    Serial.println();
  }

  Udp.beginPacket(broadcast_addr, Udp.remotePort());

  // size
  Udp.write(lowByte(LifxPacketSize + pkt.data_size));
  Udp.write(highByte(LifxPacketSize + pkt.data_size));

  // protocol
  Udp.write(lowByte(pkt.protocol));
  Udp.write(highByte(pkt.protocol));

  // reserved1
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));

  // bulbAddress mac address
  for(int i = 0; i < sizeof(mac); i++) {
    Udp.write(lowByte(mac[i]));
  }

  // reserved2
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));

  // site mac address
  for(int i = 0; i < sizeof(site_mac); i++) {
    Udp.write(lowByte(site_mac[i]));
  }

  // reserved3
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));

  // timestamp
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));

  //packet type
  Udp.write(lowByte(pkt.packet_type));
  Udp.write(highByte(pkt.packet_type));

  // reserved4
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));

  //data
  for(int i = 0; i < pkt.data_size; i++) {
    Udp.write(lowByte(pkt.data[i]));
  }

  Udp.endPacket();

  return LifxPacketSize + pkt.data_size;
}


unsigned int sendTCPPacket(LifxPacket &pkt) { 

  if (DEBUG >= 3) {
    Serial.print(F("+TCP "));
    printLifxPacket(pkt);
    Serial.println();
  }

  byte TCPBuffer[128]; //buffer to hold outgoing packet,
  int byteCount = 0;

  // size
  TCPBuffer[byteCount++] = lowByte(LifxPacketSize + pkt.data_size);
  TCPBuffer[byteCount++] = highByte(LifxPacketSize + pkt.data_size);

  // protocol
  TCPBuffer[byteCount++] = lowByte(pkt.protocol);
  TCPBuffer[byteCount++] = highByte(pkt.protocol);

  // reserved1
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);

  // bulbAddress mac address
  for(int i = 0; i < sizeof(mac); i++) {
    TCPBuffer[byteCount++] = lowByte(mac[i]);
  }

  // reserved2
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);

  // site mac address
  for(int i = 0; i < sizeof(site_mac); i++) {
    TCPBuffer[byteCount++] = lowByte(site_mac[i]);
  }

  // reserved3
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);

  // timestamp
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);

  //packet type
  TCPBuffer[byteCount++] = lowByte(pkt.packet_type);
  TCPBuffer[byteCount++] = highByte(pkt.packet_type);

  // reserved4
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);

  //data
  for(int i = 0; i < pkt.data_size; i++) {
    TCPBuffer[byteCount++] = lowByte(pkt.data[i]);
  }

  client.write(TCPBuffer, byteCount);

  return LifxPacketSize + pkt.data_size;
}


void sendPacket(LifxPacket &pkt) {
  sendUDPPacket(pkt);

  if(client.connected()) {
    sendTCPPacket(pkt);
  }
}


void sendPowerState(byte cmd) {
  LifxPacket response;

  switch (cmd) {
    case GET_POWER_STATE: response.packet_type = POWER_STATE; break;
    case GET_POWER_STATE2: response.packet_type = POWER_STATE2; break;
  }

  response.protocol = LifxProtocol_AllBulbsResponse;
  byte PowerData[] = {
    lowByte(power_status),
    highByte(power_status)
    };

  memcpy(response.data, PowerData, sizeof(PowerData));
  response.data_size = sizeof(PowerData);
  sendPacket(response);
}


void handleRequest(LifxPacket &request) {
  LifxPacket response;
  switch(request.packet_type) {

  case GET_SERVICE:
    {
      if (DEBUG >= 2) {
        Serial.println("Received GET_SERVICE request");
      }

      // we are a gateway, so respond to this

      // respond with the UDP port
      response.packet_type = STATE_SERVICE;
      response.protocol = LifxProtocol_AllBulbsResponse;
      byte UDPdata[] = { 
        SERVICE_UDP, //UDP
        lowByte(LifxPort), 
        highByte(LifxPort), 
        0x00, 
        0x00 
      };

      memcpy(response.data, UDPdata, sizeof(UDPdata));
      response.data_size = sizeof(UDPdata);
      sendPacket(response);

      // respond with the TCP port details
      response.packet_type = STATE_SERVICE;
      response.protocol = LifxProtocol_AllBulbsResponse;
      byte TCPdata[] = { 
        SERVICE_TCP, //TCP
        lowByte(LifxPort), 
        highByte(LifxPort), 
        0x00, 
        0x00 
      };

      memcpy(response.data, TCPdata, sizeof(TCPdata)); 
      response.data_size = sizeof(TCPdata);
      sendPacket(response);

    } 
    break;


  case SET_LIGHT_STATE: 
    {
      if (DEBUG >= 2) {
        Serial.print("Received SET_LIGHT_STATE request");
      }

      // set the light colors
      long new_hue = word(request.data[2], request.data[1]);
      long new_sat = word(request.data[4], request.data[3]);
      long new_bri = word(request.data[6], request.data[5]);
      long new_kel = word(request.data[8], request.data[7]);

      if ((new_hue != hue) ||
          (new_sat != sat) ||
          (new_bri != bri) ||
          (new_kel != kel)) {
        hue = new_hue;
        sat = new_sat;
        bri = new_bri;
        kel = new_kel;

        if (DEBUG >= 2) Serial.println("");

        setLight(SET_LIGHT_ORIGIN_SET_LIGHT_STATE);
      } else {
        if (DEBUG >= 2) Serial.println(" but ignored it");
      }

    } 
    break;


  case GET_LIGHT_STATE: 
    {
      if (DEBUG >= 2) {
        Serial.println("Received GET_LIGHT_STATE request");
      }

      // send the light's state
      response.packet_type = LIGHT_STATUS;
      response.protocol = LifxProtocol_AllBulbsResponse;

      Lifx_GET_LIGHT_STATE__ANSWER answer;
      answer.hue = LE16(hue);
      answer.saturation = LE16(sat);
      answer.brightness = LE16(sat);
      answer.kelvin = LE16(kel);

      answer.reserved0 = 0;

      answer.power = power_status;
      strncpy(answer.bulb_label, eeprom.sLabel, LIFX_LABEL_LENGTH);

      answer.tags = 0;

      memcpy(response.data, (void*) &answer, sizeof(answer));
      response.data_size = sizeof(answer);
      sendPacket(response);
    } 
    break;


  case SET_POWER_STATE:
  case SET_POWER_STATE2:
    {
      if (DEBUG >= 2) {
        Serial.print("Received SET_POWER_STATE");
        if (request.packet_type == SET_POWER_STATE2) {
          Serial.print("2");
        }
        Serial.print(" request");
      }

      // set if we are setting
      uint16 new_power_status = word(request.data[1], request.data[0]);

      // uint32 new_duration is ignored for SET_POWER_STATE2

      if (new_power_status != power_status) {
        if (DEBUG >= 2) Serial.println("");
        power_status = new_power_status;
        setLight(SET_LIGHT_ORIGIN_SET_POWER_STATE);
      } else {
        if (DEBUG >= 2) Serial.println(" and ignored it");
      }

      // TODO: only if es_required field is set to one
      if (request.packet_type == SET_POWER_STATE2) {
        sendPowerState(request.packet_type);
      }
    }
    break;


  case GET_POWER_STATE:
  case GET_POWER_STATE2:
    {
      if (DEBUG >= 2) {
        Serial.print("Received GET_POWER_STATE");
        if (request.packet_type == GET_POWER_STATE2) {
          Serial.print("2");
        }
        Serial.print(" request");
      }

      sendPowerState(request.packet_type);
    }
    break;


  case SET_BULB_LABEL:
  case GET_BULB_LABEL:
    {
      if (DEBUG >= 2) {
        Serial.println("Received *_BULB_LABEL request");
      }

      // set if we are setting
      if(request.packet_type == SET_BULB_LABEL) {
        Serial.println("TODO SET_BULB_LABEL");
        /*
        for(int i = 0; i < LifxBulbLabelLength; i++) {
          if(bulbLabel[i] != request.data[i]) {
            bulbLabel[i] = request.data[i];
            EEPROM.write(EEPROM_BULB_LABEL_START+i, request.data[i]);
          }
        }
        */
      }

      // respond to both get and set commands
      response.packet_type = BULB_LABEL;
      response.protocol = LifxProtocol_AllBulbsResponse;
      memcpy(response.data, eeprom.sLabel, sizeof(LIFX_LABEL_LENGTH));
      response.data_size = sizeof(LIFX_LABEL_LENGTH);
      sendPacket(response);
    } 
    break;


  case GET_VERSION_STATE: 
    {
      // respond to get command
      response.packet_type = VERSION_STATE;
      response.protocol = LifxProtocol_AllBulbsResponse;
      byte VersionData[] = { 
        lowByte(LifxBulbVendor),
        highByte(LifxBulbVendor),
        0x00,
        0x00,
        lowByte(LifxBulbProduct),
        highByte(LifxBulbProduct),
        0x00,
        0x00,
        lowByte(LifxBulbVersion),
        highByte(LifxBulbVersion),
        0x00,
        0x00
        };

      memcpy(response.data, VersionData, sizeof(VersionData));
      response.data_size = sizeof(VersionData);
      sendPacket(response);
      
      /*
      // respond again to get command (real bulbs respond twice, slightly diff data (see below)
      response.packet_type = VERSION_STATE;
      response.protocol = LifxProtocol_AllBulbsResponse;
      byte VersionData2[] = { 
        lowByte(LifxVersionVendor), //vendor stays the same
        highByte(LifxVersionVendor),
        0x00,
        0x00,
        lowByte(LifxVersionProduct*2), //product is 2, rather than 1
        highByte(LifxVersionProduct*2),
        0x00,
        0x00,
        0x00, //version is 0, rather than 1
        0x00,
        0x00,
        0x00
        };

      memcpy(response.data, VersionData2, sizeof(VersionData2));
      response.data_size = sizeof(VersionData2);
      sendPacket(response);
      */
    } 
    break;


  case GET_MESH_FIRMWARE_STATE: 
    {
      // respond to get command
      response.packet_type = MESH_FIRMWARE_STATE;
      response.protocol = LifxProtocol_AllBulbsResponse;
      // timestamp data comes from observed packet from a LIFX v1.5 bulb
      byte MeshVersionData[] = { 
        0x00, 0x2e, 0xc3, 0x8b, 0xef, 0x30, 0x86, 0x13, //build timestamp
        0xe0, 0x25, 0x76, 0x45, 0x69, 0x81, 0x8b, 0x13, //install timestamp
        lowByte(LifxFirmwareVersionMinor),
        highByte(LifxFirmwareVersionMinor),
        lowByte(LifxFirmwareVersionMajor),
        highByte(LifxFirmwareVersionMajor)
        };

      memcpy(response.data, MeshVersionData, sizeof(MeshVersionData));
      response.data_size = sizeof(MeshVersionData);
      sendPacket(response);
    } 
    break;


  case GET_WIFI_FIRMWARE_STATE: 
    {
      // respond to get command
      response.packet_type = WIFI_FIRMWARE_STATE;
      response.protocol = LifxProtocol_AllBulbsResponse;
      // timestamp data comes from observed packet from a LIFX v1.5 bulb
      byte WifiVersionData[] = { 
        0x00, 0xc8, 0x5e, 0x31, 0x99, 0x51, 0x86, 0x13, //build timestamp
        0xc0, 0x0c, 0x07, 0x00, 0x48, 0x46, 0xd9, 0x43, //install timestamp
        lowByte(LifxFirmwareVersionMinor),
        highByte(LifxFirmwareVersionMinor),
        lowByte(LifxFirmwareVersionMajor),
        highByte(LifxFirmwareVersionMajor)
        };

      memcpy(response.data, WifiVersionData, sizeof(WifiVersionData));
      response.data_size = sizeof(WifiVersionData);
      sendPacket(response);
    } 
    break;

  case LIGHT_STATUS:
  case ECHO_RESPONSE:
  case STATE_SERVICE:
  case STATE_HOST_INFO:
  case STATE_WIFI_INFO:
  case ACKNOWLEDGEMENT:
    {
      if (DEBUG >= 3) {
        Serial.print("Received and ignored ");
        const char* tag;
        switch(request.packet_type) {
          case (LIGHT_STATUS): tag = "LIGHT_STATUS"; break;
          case (ECHO_RESPONSE): tag = "ECHO_RESPONSE"; break;
          case (STATE_SERVICE): tag = "STATE_SERVICE"; break;
          case (STATE_HOST_INFO): tag = "STATE_HOST_INFO"; break;
          case (STATE_WIFI_INFO): tag = "STATE_WIFI_INFO"; break;
          case (ACKNOWLEDGEMENT): tag = "ACKNOWLEDGEMENT"; break;
          default: tag = "!!error!!";
        }
        Serial.print(tag);
        Serial.println(" packet");
      }
    }
    break;

  default: 
    {
      if (DEBUG) {
        Serial.print(F("  Unknown packet received of type 0x"));
        Serial.print(request.packet_type, HEX);
        Serial.print(" = ");
        Serial.println(request.packet_type);
      }
    } 
    break;
  }
}


// Standard arduino functions

void setup() {

  Serial.begin(115200);
  Serial.println(F("LIFX bulb emulator for ESP8266 starting up..."));

  // set up the LED pins
  pinMode(redPin, OUTPUT); 
  pinMode(greenPin, OUTPUT); 
  pinMode(bluePin, OUTPUT); 

  LIFXBulb.setFadingSteps(20);
  LIFXBulb.setFadingSpeed(20);
  
  // read in settings from EEPROM (if they exist) for bulb label and tags
  EEPROM.begin(EEPROM_CONFIG_LEN);

  eeprom = EEPROM.get(0, eeprom);

  if (strncmp(LIFX_MAGIC, eeprom.sMagic, LIFX_MAGIC_LENGTH)) {
    if (DEBUG) {
      Serial.println(F("Config in EEPROM is invalid, rewriting..."));

      strncpy(eeprom.sMagic, LIFX_MAGIC, LIFX_MAGIC_LENGTH);
      eeprom.sMagic[LIFX_MAGIC_LENGTH] = 0;

      strncpy(eeprom.sLabel, LIFX_LABEL_DEFAULT, LIFX_LABEL_LENGTH);
      eeprom.sLabel[LIFX_LABEL_LENGTH] = 0;

      #ifdef DEFAULT_WIFI_SSID
        strncpy(eeprom.sSSID, DEFAULT_WIFI_SSID, LIFX_WIFI_SSID_LENGTH);
        eeprom.sSSID[LIFX_WIFI_SSID_LENGTH] = 0;
        strncpy(eeprom.sPassword, DEFAULT_WIFI_PASSWORD, LIFX_WIFI_PASSWORD_LENGTH);
        eeprom.sPassword[LIFX_WIFI_PASSWORD_LENGTH] = 0;
      #else
        eeprom.sSSID[0] = 0;
        eeprom.sPassword[0] = 0;
      #endif

      EEPROM.put(0, eeprom);
      EEPROM.commit();

      if (DEBUG) {
        Serial.println(F("Done writing EEPROM config."));
      }
    }
  }
  
  if (DEBUG) {
    Serial.println(F("EEPROM dump:"));
    for (int i = 0; i < EEPROM_CONFIG_LEN; i++) {
      Serial.print(EEPROM.read(i));
      Serial.print(SPACE);
    }
    Serial.println();
  }

  // If no wifi credentials have been saved in the config, go into AP mode
  if (eeprom.sSSID[0] == 0) {
    //setAPMode();
  } else {
    /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
    WiFi.mode(WIFI_STA);

    // connect to known wifi network
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.print(eeprom.sSSID);
    if (DEBUG) {
      Serial.print("/");
      Serial.print(eeprom.sPassword);
    }
    WiFi.begin(eeprom.sSSID, eeprom.sPassword);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    Serial.println("");
    Serial.print(F("WiFi connected. IP address for this bulb: "));
    Serial.println(WiFi.localIP());

    // set up a UDP and TCP port ready for incoming
    Udp.begin(LifxPort);
    TcpServer.begin();
  }

  // set the bulb based on the initial colors
  setLight(SET_LIGHT_ORIGIN_SETUP);
}

void loop() {
  LIFXBulb.tick();

  // buffers for receiving and sending data
  byte PacketBuffer[128]; //buffer to hold incoming packet,

  client = TcpServer.available();
  if (client == true) {
    // read incoming data
    int packetSize = 0;
    while (client.available()) {
      byte b = client.read();
      PacketBuffer[packetSize] = b;
      packetSize++;
    }

    if(DEBUG) {
      Serial.print(F("-TCP "));
      for(int i = 0; i < LifxPacketSize; i++) {
        Serial.print(PacketBuffer[i], HEX);
        Serial.print(SPACE);
      }

      for(int i = LifxPacketSize; i < packetSize; i++) {
        Serial.print(PacketBuffer[i], HEX);
        Serial.print(SPACE);
      }
      Serial.println();
    }

    // push the data into the LifxPacket structure
    LifxPacket request;
    processRequest(PacketBuffer, packetSize, request);

    //respond to the request
    handleRequest(request);
  }

  // if there's UDP data available, read a packet
  int packetSize = Udp.parsePacket();
  if(packetSize) {
    Udp.read(PacketBuffer, 128);

    if (DEBUG >= 3) {
      Serial.print(F("-UDP "));
      for(int i = 0; i < LifxPacketSize; i++) {
        Serial.print(PacketBuffer[i], HEX);
        Serial.print(SPACE);
      }

      for(int i = LifxPacketSize; i < packetSize; i++) {
        Serial.print(PacketBuffer[i], HEX);
        Serial.print(SPACE);
      }
      Serial.println();
    }

    // push the data into the LifxPacket structure
    LifxPacket request;
    processRequest(PacketBuffer, sizeof(PacketBuffer), request);

    //respond to the request
    handleRequest(request);

  }

  //Ethernet.maintain();

  //delay(10);
}

#ifndef __LIFX_H__
#define __LIFX_H__

const unsigned int LifxProtocol_AllBulbsResponse = 21504; // 0x5400
const unsigned int LifxProtocol_AllBulbsRequest  = 13312; // 0x3400
const unsigned int LifxProtocol_BulbCommand      = 5120;  // 0x1400

const unsigned int LifxPacketSize      = 36;
const unsigned int LifxPort            = 56700;  // local port to listen on

#define LIFX_MAGIC_LENGTH 4
#define LIFX_MAGIC "EL02"

#define LIFX_LABEL_LENGTH 32
#define LIFX_LABEL_DEFAULT "ESP8266 Lifx"

#define LIFX_LOCATION_LENGTH 16
#define LIFX_LOCATION_DEFAULT "Outta space"

#define LIFX_GROUP_LENGTH 16
#define LIFX_GROUP_DEFAULT "Home made"

#define LIFX_WIFI_SSID_LENGTH 32
#define LIFX_WIFI_PASSWORD_LENGTH 64

const unsigned int LifxBulbTagsLength = 8;
const unsigned int LifxBulbTagLabelsLength = 32;

// firmware versions, etc
const unsigned int LifxBulbVendor = 1;
const unsigned int LifxBulbProduct = 1;
const unsigned int LifxBulbVersion = 1;
const unsigned int LifxFirmwareVersionMajor = 1;
const unsigned int LifxFirmwareVersionMinor = 5;

#define EEPROM_BULB_LABEL_START 0 // 32 bytes long
#define EEPROM_BULB_TAGS_START 32 // 8 bytes long
#define EEPROM_BULB_TAG_LABELS_START 40 // 32 bytes long
// future data for EEPROM will start at 72...

// EEPROM structure
#define EEPROM_CONFIG_LEN 256

struct LifxEEPROM {
  char sMagic[LIFX_MAGIC_LENGTH + 1];
  char sLabel[LIFX_LABEL_LENGTH + 1];
  char sLocation[LIFX_LOCATION_LENGTH + 1];
  uint64_t location_updated_at;
  char sGroup[LIFX_GROUP_LENGTH + 1];
  uint64_t group_updated_at;
  char sSSID[LIFX_WIFI_SSID_LENGTH + 1];
  char sPassword[LIFX_WIFI_PASSWORD_LENGTH + 1];
};

#define SET_LIGHT_ORIGIN_SETUP 0
#define SET_LIGHT_ORIGIN_SET_LIGHT_STATE 1
#define SET_LIGHT_ORIGIN_SET_POWER_STATE 2

#endif
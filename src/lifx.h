
const unsigned int LifxProtocol_AllBulbsResponse = 21504; // 0x5400
const unsigned int LifxProtocol_AllBulbsRequest  = 13312; // 0x3400
const unsigned int LifxProtocol_BulbCommand      = 5120;  // 0x1400

const unsigned int LifxPacketSize      = 36;
const unsigned int LifxPort            = 56700;  // local port to listen on

#define LIFX_MAGIC_LENGTH 4
#define LIFX_MAGIC "EL02"

#define LIFX_LABEL_LENGTH 32
#define LIFX_LABEL_DEFAULT "ESP8266 Lifx"

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

const byte SERVICE_UDP = 0x01;
const byte SERVICE_TCP = 0x02;

// packet types
const byte GET_SERVICE = 0x02;
const byte STATE_SERVICE = 0x03;

const byte GET_HOST_INFO = 0x0C;
const byte STATE_HOST_INFO = 0x0D;

const byte GET_WIFI_INFO = 0x10;
const byte STATE_WIFI_INFO = 0x11;

const byte GET_WIFI_FIRMWARE_STATE = 0x12;
const byte WIFI_FIRMWARE_STATE = 0x13;

const byte GET_POWER_STATE = 0x14;
const byte SET_POWER_STATE = 0x15;
const byte POWER_STATE = 0x16;

const byte GET_BULB_LABEL = 0x17;
const byte SET_BULB_LABEL = 0x18;
const byte BULB_LABEL = 0x19;

const byte GET_VERSION_STATE = 0x20;
const byte VERSION_STATE = 0x21;

const byte GET_BULB_TAGS = 0x1a;
const byte SET_BULB_TAGS = 0x1b;
const byte BULB_TAGS = 0x1c;

const byte GET_BULB_TAG_LABELS = 0x1d;
const byte SET_BULB_TAG_LABELS = 0x1e;
const byte BULB_TAG_LABELS = 0x1f;

const byte ACKNOWLEDGEMENT = 0x2D;

const byte ECHO_REQUEST = 0x3A;
const byte ECHO_RESPONSE = 0x3B;

const byte GET_LIGHT_STATE = 0x65;
const byte SET_LIGHT_STATE = 0x66;
const byte LIGHT_STATUS = 0x6b;

const byte GET_POWER_STATE2 = 0x74;
const byte SET_POWER_STATE2 = 0x75;
const byte POWER_STATE2 = 0x76;

const byte GET_MESH_FIRMWARE_STATE = 0x0e;
const byte MESH_FIRMWARE_STATE = 0x0f;


#define EEPROM_BULB_LABEL_START 0 // 32 bytes long
#define EEPROM_BULB_TAGS_START 32 // 8 bytes long
#define EEPROM_BULB_TAG_LABELS_START 40 // 32 bytes long
// future data for EEPROM will start at 72...

// EEPROM structure
#define EEPROM_CONFIG_LEN 256

struct LifxEEPROM {
  char sMagic[LIFX_MAGIC_LENGTH + 1];
  char sLabel[LIFX_LABEL_LENGTH + 1];
  char sSSID[LIFX_WIFI_SSID_LENGTH + 1];
  char sPassword[LIFX_WIFI_PASSWORD_LENGTH + 1];
};

// helpers
#define SPACE " "

inline uint16_t reverse16(uint16_t value)
{
    return (((value & 0x00FF) << 8) |
            ((value & 0xFF00) >> 8));
}

#define LE16 reverse16

// message structures and their answers
struct Lifx_GET_LIGHT_STATE__ANSWER {
    uint16 hue;          // LE
    uint16 saturation;   // LE
    uint16 brightness;   // LE
    uint16 kelvin;       // LE

    short reserved0;

    uint16 power;
    char bulb_label[LIFX_LABEL_LENGTH]; // UTF-8 encoded string
    uint64 tags;
};


#define SET_LIGHT_ORIGIN_SETUP 0
#define SET_LIGHT_ORIGIN_SET_LIGHT_STATE 1
#define SET_LIGHT_ORIGIN_SET_POWER_STATE 2

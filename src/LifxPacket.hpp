
struct LifxPacket {
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

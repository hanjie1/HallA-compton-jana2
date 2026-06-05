#ifndef MOLLERADC_RAW_PACKET_H
#define MOLLERADC_RAW_PACKET_H

#include <cstdint>
#include <vector>

class MOLLERADCRawPacket {
public:
    enum class PacketType : uint8_t {
        Integrating = 0xaa,
        Streaming = 0xdd,
        Unknown = 0xff
    };

    uint32_t rocid = 0;
    uint64_t event_number = 0;
    PacketType packet_type = PacketType::Unknown;
    uint8_t packet_id = 0;
    uint32_t num_pkt = 0;
    uint16_t num_words = 0;
    uint64_t timestamp = 0;
    uint64_t packet_count = 0;
    uint8_t block = 0;
    uint64_t total_samples = 0;
    std::vector<uint64_t> words;
};

#endif // MOLLERADC_RAW_PACKET_H

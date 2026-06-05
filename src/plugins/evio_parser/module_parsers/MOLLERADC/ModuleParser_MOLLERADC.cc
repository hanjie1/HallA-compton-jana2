#include "ModuleParser_MOLLERADC.h"

#include <iomanip>

#include <JANA/JException.h>

#include "EventHits_MOLLERADC.h"
#include "MOLLERADCIntegratingHit.h"
#include "MOLLERADCRawPacket.h"
#include "MOLLERADCStreamingSample.h"

uint64_t ModuleParser_MOLLERADC::byteSwap64(uint64_t value) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap64(value);
#else
    return ((value & 0x00000000000000ffULL) << 56) |
           ((value & 0x000000000000ff00ULL) << 40) |
           ((value & 0x0000000000ff0000ULL) << 24) |
           ((value & 0x00000000ff000000ULL) << 8) |
           ((value & 0x000000ff00000000ULL) >> 8) |
           ((value & 0x0000ff0000000000ULL) >> 24) |
           ((value & 0x00ff000000000000ULL) >> 40) |
           ((value & 0xff00000000000000ULL) >> 56);
#endif
}

uint64_t ModuleParser_MOLLERADC::getBitsInRange64(uint64_t value, int high, int low) {
    if (high == 63 && low == 0) {
        return value;
    }
    return (value >> low) & ((1ULL << (high - low + 1)) - 1ULL);
}

int32_t ModuleParser_MOLLERADC::signExtend(uint64_t value, int bits) {
    const uint64_t sign_bit = 1ULL << (bits - 1);
    const uint64_t mask = (1ULL << bits) - 1ULL;
    value &= mask;
    return static_cast<int32_t>((value ^ sign_bit) - sign_bit);
}

void ModuleParser_MOLLERADC::parse(std::shared_ptr<evio::BaseStructure> data_block,
                                   uint32_t rocid,
                                   std::vector<PhysicsEvent*>& physics_events,
                                   TriggerData& trigger_data) {
    auto raw_words = data_block->getULongData();
    if (raw_words.empty()) {
        return;
    }

    auto event_hits = std::make_shared<EventHits_MOLLERADC>();
    size_t packet_start = 0;
    while (packet_start < raw_words.size()) {
        const uint64_t header = byteSwap64(raw_words[packet_start]);
        const uint16_t num_words = static_cast<uint16_t>(getBitsInRange64(header, 15, 0));
        if (num_words == 0) {
            throw JException("ModuleParser_MOLLERADC::parse: zero-length packet at word %d", static_cast<int>(packet_start));
        }

        const size_t packet_size = static_cast<size_t>(num_words) + 1U;
        if (packet_start + packet_size > raw_words.size()) {
            throw JException(
                "ModuleParser_MOLLERADC::parse: packet at word %d declares %d payload words, but only %d words remain",
                static_cast<int>(packet_start),
                num_words,
                static_cast<int>(raw_words.size() - packet_start - 1)
            );
        }

        parsePacket(raw_words, packet_start, packet_size, header, rocid, trigger_data.first_event_number, *event_hits);
        packet_start += packet_size;
    }

    if (!event_hits->raw_packets.empty() ||
        !event_hits->integrating_hits.empty() ||
        !event_hits->streaming_samples.empty()) {
        physics_events.push_back(new PhysicsEvent(trigger_data.first_event_number, event_hits));
    }
}

void ModuleParser_MOLLERADC::parsePacket(const std::vector<uint64_t>& raw_words,
                                         size_t packet_start,
                                         size_t packet_size,
                                         uint64_t header,
                                         uint32_t rocid,
                                         uint64_t event_number,
                                         EventHits_MOLLERADC& event_hits) {
    auto* packet = new MOLLERADCRawPacket();
    fillRawPacket(*packet, raw_words, packet_start, packet_size, header, rocid, event_number);

    switch (packet->packet_type) {
        case MOLLERADCRawPacket::PacketType::Integrating:
            parseIntegratingPacket(*packet, event_hits);
            break;
        case MOLLERADCRawPacket::PacketType::Streaming:
            parseStreamingPacket(*packet, event_hits);
            break;
        case MOLLERADCRawPacket::PacketType::Unknown:
            LOG_WARN(GetLogger()) << "ModuleParser_MOLLERADC::parsePacket: unknown packet id 0x"
                                  << std::hex << static_cast<uint32_t>(packet->packet_id)
                                  << std::dec << LOG_END;
            break;
    }

    event_hits.raw_packets.push_back(packet);
}

void ModuleParser_MOLLERADC::fillRawPacket(MOLLERADCRawPacket& packet,
                                           const std::vector<uint64_t>& raw_words,
                                           size_t packet_start,
                                           size_t packet_size,
                                           uint64_t header,
                                           uint32_t rocid,
                                           uint64_t event_number) {
    packet.words.reserve(packet_size);
    packet.words.push_back(header);
    for (size_t i = 1; i < packet_size; ++i) {
        packet.words.push_back(byteSwap64(raw_words[packet_start + i]));
    }

    packet.rocid = rocid;
    packet.event_number = event_number;
    packet.packet_id = static_cast<uint8_t>(getBitsInRange64(header, 63, 56));
    packet.num_pkt = static_cast<uint32_t>(getBitsInRange64(header, 47, 16));
    packet.num_words = static_cast<uint16_t>(getBitsInRange64(header, 15, 0));

    if (packet.packet_id == 0xaa) {
        packet.packet_type = MOLLERADCRawPacket::PacketType::Integrating;
    } else if (packet.packet_id == 0xdd) {
        packet.packet_type = MOLLERADCRawPacket::PacketType::Streaming;
    } else {
        packet.packet_type = MOLLERADCRawPacket::PacketType::Unknown;
    }

    if (packet.words.size() > 1) {
        packet.timestamp = packet.words[1];
    }
    if (packet.words.size() > 2) {
        packet.block = static_cast<uint8_t>(getBitsInRange64(packet.words[2], 63, 60));
        packet.packet_count = getBitsInRange64(packet.words[2], 59, 0);
    }
    if (packet.words.size() > 3) {
        packet.total_samples = packet.words[3];
    }
}

void ModuleParser_MOLLERADC::parseIntegratingPacket(const MOLLERADCRawPacket& packet,
                                                    EventHits_MOLLERADC& event_hits) {
    constexpr size_t header_words = 4;
    constexpr size_t values_per_channel = 7;
    if (packet.words.size() <= header_words) {
        return;
    }

    const size_t channel_payload_words = packet.words.size() - header_words;
    if (channel_payload_words % values_per_channel != 0) {
        LOG_WARN(GetLogger()) << "ModuleParser_MOLLERADC::parseIntegratingPacket: payload word count "
                              << channel_payload_words << " is not divisible by "
                              << values_per_channel << LOG_END;
        return;
    }

    const size_t channel_count = channel_payload_words / values_per_channel;
    for (size_t channel = 0; channel < channel_count; ++channel) {
        const size_t base = header_words + channel;
        auto* hit = new MOLLERADCIntegratingHit();
        hit->rocid = packet.rocid;
        hit->event_number = packet.event_number;
        hit->region_number = packet.num_pkt;
        hit->num_words = packet.num_words;
        hit->timestamp = packet.timestamp;
        hit->block = packet.block;
        hit->packet_count = packet.packet_count;
        hit->total_samples = packet.total_samples;
        hit->channel = static_cast<uint32_t>(channel);

        const uint64_t misc = packet.words[base];
        hit->min_sample = signExtend(getBitsInRange64(misc, 19, 0), 20);
        hit->max_sample = signExtend(getBitsInRange64(misc, 39, 20), 20);
        hit->window_sample_count = packet.words[base + channel_count];
        hit->window_sum = packet.words[base + 2 * channel_count];
        hit->window_sum_of_squares = packet.words[base + 3 * channel_count];
        hit->sample_count = packet.words[base + 4 * channel_count];
        hit->sum = packet.words[base + 5 * channel_count];
        hit->sum_of_squares = packet.words[base + 6 * channel_count];

        event_hits.integrating_hits.push_back(hit);
    }
}

void ModuleParser_MOLLERADC::parseStreamingPacket(const MOLLERADCRawPacket& packet,
                                                  EventHits_MOLLERADC& event_hits) {
    if (packet.words.size() < 2) {
        return;
    }

    const size_t sample_words = packet.words.size() - 2;
    for (size_t index = 0; index < sample_words; ++index) {
        const uint64_t word = packet.words[index + 2];
        const uint32_t ch0 = static_cast<uint32_t>(getBitsInRange64(word, 31, 0));
        const uint32_t ch1 = static_cast<uint32_t>(getBitsInRange64(word, 63, 32));
        auto* sample = new MOLLERADCStreamingSample();
        sample->rocid = packet.rocid;
        sample->event_number = packet.event_number;
        sample->region_number = packet.num_pkt;
        sample->num_words = packet.num_words;
        sample->timestamp = packet.timestamp;
        sample->sample_index = index;
        sample->channel0 = static_cast<uint8_t>(ch0 & 0xfU);
        sample->prescale = static_cast<uint8_t>(((ch0 >> 4) & 0x7fU) + 1U);
        sample->gate0 = ((ch0 >> 12) & 0x1U) != 0;
        sample->gate1 = ((ch0 >> 13) & 0x1U) != 0;
        sample->sample0 = signExtend(ch0 >> 14, 18);
        sample->channel1 = static_cast<uint8_t>(ch1 & 0xfU);
        sample->sample1 = signExtend(ch1 >> 14, 18);
/* not sure if the following codes are correct
        const uint64_t ticks_per_sample = sample->channel0 == sample->channel1
                                            ? static_cast<uint64_t>(index) * 2ULL
                                            : static_cast<uint64_t>(index);
        sample->sample_time = packet.timestamp + ticks_per_sample * static_cast<uint64_t>(sample->prescale);
*/
        event_hits.streaming_samples.push_back(sample);
    }
}

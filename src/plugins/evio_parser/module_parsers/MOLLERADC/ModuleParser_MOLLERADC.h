#ifndef MODULEPARSER_MOLLERADC_H
#define MODULEPARSER_MOLLERADC_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "ModuleParser.h"

class EventHits_MOLLERADC;
class MOLLERADCRawPacket;

class ModuleParser_MOLLERADC : public ModuleParser {
public:
    void parse(std::shared_ptr<evio::BaseStructure> data_block,
               uint32_t rocid,
               std::vector<PhysicsEvent*>& physics_events,
               TriggerData& trigger_data) override;

private:
    static uint64_t byteSwap64(uint64_t value);
    static uint64_t getBitsInRange64(uint64_t value, int high, int low);
    static int32_t signExtend(uint64_t value, int bits);
    static int64_t asSigned64(uint64_t value);

    void parsePacket(const std::vector<uint64_t>& raw_words,
                     size_t packet_start,
                     size_t packet_size,
                     uint64_t header,
                     uint32_t rocid,
                     uint64_t event_number,
                     EventHits_MOLLERADC& event_hits);

    void fillRawPacket(MOLLERADCRawPacket& packet,
                       const std::vector<uint64_t>& raw_words,
                       size_t packet_start,
                       size_t packet_size,
                       uint64_t header,
                       uint32_t rocid,
                       uint64_t event_number);

    void parseIntegratingPacket(const MOLLERADCRawPacket& packet,
                                EventHits_MOLLERADC& event_hits);

    void parseStreamingPacket(const MOLLERADCRawPacket& packet,
                              EventHits_MOLLERADC& event_hits);
};

#endif // MODULEPARSER_MOLLERADC_H

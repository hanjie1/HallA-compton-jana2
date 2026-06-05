#ifndef EVENT_HITS_MOLLERADC_H
#define EVENT_HITS_MOLLERADC_H

#include <vector>

#include <JANA/JEvent.h>

#include "EventHits.h"
#include "MOLLERADCIntegratingHit.h"
#include "MOLLERADCRawPacket.h"
#include "MOLLERADCStreamingSample.h"

class EventHits_MOLLERADC : public EventHits {
public:
    std::vector<MOLLERADCIntegratingHit*> integrating_hits;
    std::vector<MOLLERADCStreamingSample*> streaming_samples;
    std::vector<MOLLERADCRawPacket*> raw_packets;

    void insertIntoEvent(JEvent& event) override {
        for (auto& hit : integrating_hits) {
            event.Insert(hit);
        }
        for (auto& sample : streaming_samples) {
            event.Insert(sample);
        }
        for (auto& packet : raw_packets) {
            event.Insert(packet);
        }
    }
};

#endif // EVENT_HITS_MOLLERADC_H

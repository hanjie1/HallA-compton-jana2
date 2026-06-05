#ifndef MOLLERADC_STREAMING_SAMPLE_H
#define MOLLERADC_STREAMING_SAMPLE_H

#include <cstdint>

class MOLLERADCStreamingSample {
public:
    uint32_t rocid = 0;
    uint64_t event_number = 0;
    uint32_t region_number = 0;
    uint16_t num_words = 0;
    uint64_t timestamp = 0;
    uint64_t sample_index = 0;
    uint64_t sample_time = 0;
    uint8_t channel0 = 0;
    uint8_t channel1 = 0;
    uint8_t prescale = 0;
    bool gate0 = false;
    bool gate1 = false;
    int32_t sample0 = 0;
    int32_t sample1 = 0;
};

#endif // MOLLERADC_STREAMING_SAMPLE_H

#ifndef MOLLERADC_INTEGRATING_HIT_H
#define MOLLERADC_INTEGRATING_HIT_H

#include <cstdint>

class MOLLERADCIntegratingHit {
public:
    uint32_t rocid = 0;
    uint64_t event_number = 0;
    uint32_t region_number = 0;
    uint16_t num_words = 0;
    uint64_t timestamp = 0;
    uint8_t block = 0;
    uint64_t packet_count = 0;
    uint64_t total_samples = 0;
    uint32_t channel = 0;
    int32_t max_sample = 0;
    int32_t min_sample = 0;
    int64_t window_sample_count = 0;
    int64_t window_sum = 0;
    int64_t window_sum_of_squares = 0;
    int64_t sample_count = 0;
    int64_t sum = 0;
    int64_t sum_of_squares = 0;
};

#endif // MOLLERADC_INTEGRATING_HIT_H

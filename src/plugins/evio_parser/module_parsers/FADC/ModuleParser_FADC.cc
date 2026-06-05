#include "ModuleParser_FADC.h"
#include "EventHits_FADC.h"
#include <JANA/JException.h>

/**
 * @brief Parse a raw data block and extract hits
 * 
 * This method parses the raw FADC250 data block by:
 * 1. Processing data words sequentially
 * 2. Identifying different word types (headers, trailers, data)
 * 3. Extracting waveform and pulse hits
 * 4. Adding hits to the event hits container
 * 
 * @param data_block The data block to parse
 * @param rocid ROC ID for this data block
 * @param physics_events Reference to physics events vector (will be updated)
 * @param trigger_data Trigger metadata for this EVIO block
 */
void ModuleParser_FADC::parse(std::shared_ptr<evio::BaseStructure> data_block,
                               uint32_t rocid,
                               std::vector<PhysicsEvent*>& physics_events,
                               TriggerData& trigger_data) {
    // Get all data words from the block
    std::vector<uint32_t> data_words = data_block->getUIntData();
    
    // Variables to track block and event information
    uint32_t block_slot = 0;
    uint32_t module_id = 0;
    uint32_t trigger_num = 0;
    uint32_t timestamp1 = 0;
    uint32_t timestamp2 = 0;
    uint64_t event_number = 0;
    int block_number = -1;
    int block_nevents = -1;  // -1 indicates no block header processed yet
    uint32_t type2_triggertime = 0;
    uint32_t type9_eventnumber = 0;
    uint64_t event_index = 0;

    // Map from event_number to EventHits_FADC, used to merge hits from
    // multiple blocks within the same data bank into a single PhysicsEvent.
    std::map<uint64_t, std::shared_ptr<EventHits_FADC>> event_hits_map;

    
    // Process each data word sequentially
    for (size_t j = 0; j < data_words.size(); j++) {
        auto d = data_words[j];
        uint32_t word_type = getBitsInRange(d, 31, 31);
        
        // Process data type defining words (word_type == 1)
        if (word_type == 1) {
            uint32_t data_type = getBitsInRange(d, 30, 27);
            
            if (data_type == 0) { // Block header
                block_slot = getBitsInRange(d, 26, 22);
                module_id = getBitsInRange(d, 21, 18);
                block_number = getBitsInRange(d, 17, 8);
                block_nevents = getBitsInRange(d, 7, 0);
                LOG_DEBUG(GetLogger()) << "ModuleParser_FADC::DEBUG - data type 0 Block slot = " << block_slot  << "; Module ID = " << module_id << "; Block number = " << block_number << "; Number of block events = " << block_nevents << LOG_END;
            } else if (data_type == 1) { // Block trailer
                if (block_nevents != 0) {
                    throw JException(
                        "ModuleParser_FADC::parse: Invalid data format — block trailer word before reading in all events"
                    );
                }
                block_nevents = -1;
                event_index = 0;
            } else if (data_type == 2) { // Event header
                if (block_nevents <= 0) {
                    throw JException(
                        "ModuleParser_FADC::parse: Invalid data format — event header before block header"
                    );
                }
                block_nevents--;
                auto eh_slot = getBitsInRange(d, 26, 22);
                if (eh_slot != block_slot) {
                    throw JException(
                        "ModuleParser_FADC::parse: Invalid data — event slot(%d) != block slot(%d)", 
                        eh_slot, block_slot
                    );
                }
                type2_triggertime = getBitsInRange(d, 21, 12);
                trigger_num = getBitsInRange(d, 11, 0);
                LOG_DEBUG(GetLogger()) << "ModuleParser_FADC::DEBUG - data type 2 Type2 trigger time = " << type2_triggertime << "; Trigger number = " << trigger_num << LOG_END;

                // Compute event number and get or create the hits container
                event_number = trigger_data.first_event_number + event_index;
                event_index++;
                if (event_hits_map.find(event_number) == event_hits_map.end()) {
                    event_hits_map[event_number] = std::make_shared<EventHits_FADC>();
                }   
            } else if (data_type == 3) { // Trigger time
                if (block_nevents < 0) {
                    throw JException(
                        "ModuleParser_FADC::parse: Invalid data format — trigger time word before block & event header"
                    );
                }
                timestamp1 = getBitsInRange(d, 23, 0);
                auto d2 = data_words[++j];
                timestamp2 = getBitsInRange(d2, 23, 0);
                LOG_DEBUG(GetLogger()) << "ModuleParser_FADC::DEBUG - data type 3 Time stamp 1 = " << timestamp1  << "; Time stamp 2 = " << timestamp2 << LOG_END;
            } else if (data_type == 4) { // Waveform data
                if (block_nevents < 0) {
                    throw JException(
                        "ModuleParser_FADC::parse: Invalid data format — waveform data word before block & event header"
                    );
                }
                uint32_t chan = getBitsInRange(d, 26, 23);
                uint32_t waveform_len = getBitsInRange(d, 11, 0);

                LOG_DEBUG(GetLogger()) << "ModuleParser_FADC::DEBUG - data type 4 CHAN = " << chan  << "; waveform length = " << waveform_len<<LOG_END;
                
                // Parse waveform data and add to event hits
                FADC250WaveformHit hit = parseWaveformData(
                    data_words, j, trigger_num, timestamp1, timestamp2,
                    rocid, block_slot, module_id, chan, waveform_len
                );
                event_hits_map[event_number]->waveforms.push_back(new FADC250WaveformHit(hit));
            } else if (data_type == 7) { // Pulse Integral data
                if (block_nevents < 0) {
                    throw JException(
                        "ModuleParser_FADC::parse: Invalid data format — pulse integral data word before block & event header"
                    );
                }
                uint32_t chan = getBitsInRange(d, 26, 23);
                uint32_t pulse_number = getBitsInRange(d, 22, 21);
                uint32_t pulse_integral = getBitsInRange(d, 20, 0);
                event_hits_map[event_number]->pulse_integrals.push_back(new FADC250HallBPulseIntegralHit(trigger_num, timestamp1, timestamp2, rocid, block_slot, module_id, chan, pulse_number, pulse_integral));
                LOG_DEBUG(GetLogger()) << "ModuleParser_FADC::DEBUG - data type 7 CHAN = " << chan << "; Pulse number = " << pulse_number << "; Pulse integral = " << pulse_integral << LOG_END;
            } else if (data_type == 8) { // Pulse Time data
                if (block_nevents < 0) {
                    throw JException(
                        "ModuleParser_FADC::parse: Invalid data format — pulse time data word before block & event header"
                    );
                }
                uint32_t chan = getBitsInRange(d, 26, 23);
                uint32_t pulse_number = getBitsInRange(d, 22, 21);
                uint32_t measurement_quality_factor = getBitsInRange(d, 20, 19);
                uint32_t coarse_pulse_time = getBitsInRange(d, 15, 6);
                uint32_t fine_pulse_time = getBitsInRange(d, 5, 0);
                event_hits_map[event_number]->pulse_times.push_back(new FADC250HallBPulseTimeHit(trigger_num, timestamp1, timestamp2, rocid, block_slot, module_id, chan, pulse_number, measurement_quality_factor, coarse_pulse_time, fine_pulse_time));
                LOG_DEBUG(GetLogger()) << "ModuleParser_FADC::DEBUG - data type 8 CHAN = " << chan  << "; Pulse number = " << pulse_number << "; Measurement quality factor = " <<  measurement_quality_factor << "; Coarse pulse time = " <<  coarse_pulse_time << "; Fine pulse time = " <<  fine_pulse_time << LOG_END;
            } else if (data_type == 9) { // Pulse Raw data
                if (block_nevents < 0) {
                    throw JException(
                        "ModuleParser_FADC::parse: Invalid data format — pulse data word before block & event header"
                    );
                }
                type9_eventnumber = getBitsInRange(d, 26, 19);
                uint32_t chan = getBitsInRange(d, 18, 15);
                uint32_t pedestal_quality = getBitsInRange(d, 14, 14);
                uint32_t pedestal_sum = getBitsInRange(d, 13, 0);

                LOG_DEBUG(GetLogger()) << "ModuleParser_FADC::DEBUG - data type 9 Type 9 Event Number = " << type9_eventnumber << ";  CHAN = " << chan  << "; Pedestal quality = " << pedestal_quality<<"; Pedestal sum = "<<pedestal_sum<<LOG_END;
                
                // Parse pulse data and add to event hits
                auto hits = parsePulseData(
                    data_words, j, trigger_num, timestamp1, timestamp2,
                    rocid, block_slot, module_id, chan, pedestal_quality, pedestal_sum
                );
                for (auto& hit : hits) {
                    event_hits_map[event_number]->pulses.push_back(new FADC250PulseHit(hit));
                }

            } else if (data_type == 10) { // Pulse Peak data
                if (block_nevents < 0) {
                    throw JException(
                        "ModuleParser_FADC::parse: Invalid data format — pulse peak data word before block & event header"
                    );
                }
                uint32_t chan = getBitsInRange(d, 26, 23);
                uint32_t pulse_number = getBitsInRange(d, 22, 21);
                uint32_t Vmin = getBitsInRange(d, 20, 12);
                uint32_t Vpeak = getBitsInRange(d, 11, 0);
                LOG_DEBUG(GetLogger()) << "ModuleParser_FADC::DEBUG - data type 10 CHAN = " << chan << "; Pulse number = " << pulse_number << "; Vmin = "<< Vmin << "; Vpeak = "<< Vpeak << LOG_END;
                event_hits_map[event_number]->pulse_peaks.push_back(new FADC250HallBPulsePeakHit(trigger_num, timestamp1, timestamp2, rocid, block_slot, module_id, chan, pulse_number, Vmin, Vpeak));
            }
        }
    }

    // Create PhysicsEvent objects from the map
    for (auto& event_hit : event_hits_map) {
        PhysicsEvent* physics_event = new PhysicsEvent(event_hit.first, event_hit.second);
        physics_events.push_back(physics_event);
    }
}


/**
 * @brief Parse waveform data from data words
 */
FADC250WaveformHit ModuleParser_FADC::parseWaveformData(
    const std::vector<uint32_t>& data_words,
    size_t& index,
    uint32_t trigger_num,
    uint32_t timestamp1,
    uint32_t timestamp2,
    uint32_t rocid,
    uint32_t slot,
    uint32_t module_id,
    uint32_t chan,
    uint32_t waveform_len) {
    
    // Create waveform hit with basic information
    FADC250WaveformHit hit(trigger_num, timestamp1, timestamp2, rocid, slot, module_id, chan);
    
    // Calculate number of waveform words (each word contains 2 samples)
    auto nwaveform_words = (waveform_len + 1) / 2;
    
    // Parse waveform samples from continuation words
    for (size_t k = index + 1; k < index + 1 + nwaveform_words; k++) {
        if (k >= data_words.size()) break;
        
        auto ww = data_words[k]; // waveform word
        auto ww_word_type = getBitsInRange(ww, 31, 31);
        if (ww_word_type != 0) {
            throw JException(
                "ModuleParser_FADC::parseWaveformData: Invalid data format — lesser words than required for getting all waveform samples"
            );
        }
        
        // Extract first sample (bits 28-16)
        auto x_notvalid = getBitsInRange(ww, 29, 29);
        if (!x_notvalid) {
            hit.addSample(getBitsInRange(ww, 28, 16));
            LOG_DEBUG(GetLogger()) << "ModuleParser_FADC::DEBUG - data type 4 Waveform sample " << getBitsInRange(ww, 28, 16) <<LOG_END;
        }
        
        // Extract second sample (bits 12-0)
        auto x_plus_notvalid = getBitsInRange(ww, 13, 13);
        if (!x_plus_notvalid) {
            hit.addSample(getBitsInRange(ww, 12, 0));
            LOG_DEBUG(GetLogger()) << "ModuleParser_FADC::DEBUG - data type 4 Waveform sample " << getBitsInRange(ww, 12, 0) <<LOG_END;
        }
    }
    
    // Validate waveform length
    if (waveform_len != hit.getWaveformSize()) {
        throw JException(
            "ModuleParser_FADC::parseWaveformData: Invalid data — Header given waveform size (%d) != Obtained waveform size (%d)",
            waveform_len, hit.getWaveformSize()
        );
    }
    
    // Update index to skip over waveform continuation words
    index += nwaveform_words;
    
    return hit;
}

/**
 * @brief Parse pulse data from data words
 */
std::vector<FADC250PulseHit> ModuleParser_FADC::parsePulseData(
    const std::vector<uint32_t>& data_words,
    size_t& index,
    uint32_t trigger_num,
    uint32_t timestamp1,
    uint32_t timestamp2,
    uint32_t rocid,
    uint32_t slot,
    uint32_t module_id,
    uint32_t chan,
    uint32_t pedestal_quality,
    uint32_t pedestal_sum) {
    
    std::vector<FADC250PulseHit> pulse_hits;
    
    // Parse continuation word pairs (Words 2+3, repeated per pulse)
    while (true) {
        if (index + 2 >= data_words.size()) break; // no more words, avoid overrun
        
        uint32_t w2 = data_words[++index];
        uint32_t w3 = data_words[++index];
        
        // Check word type = 0 (continuation words)
        if (getBitsInRange(w2, 31, 31) != 0 || getBitsInRange(w3, 31, 31) != 0) {
            // If we encounter non-continuation word, backtrack and break
            index -= 2;
            break;
        }
        
        // Create pulse hit with basic information
        FADC250PulseHit pulse_hit(trigger_num, timestamp1, timestamp2, rocid, slot, module_id, chan,
                               pedestal_quality, pedestal_sum);
        
        // Word 2: pulse integral information
        pulse_hit.integral_sum = getBitsInRange(w2, 29, 12);
        pulse_hit.integral_quality = getBitsInRange(w2, 11, 9);
        pulse_hit.nsamples_above_th = getBitsInRange(w2, 8, 0);
        
        // Word 3: pulse timing and peak information
        pulse_hit.coarse_time = getBitsInRange(w3, 29, 21);
        pulse_hit.fine_time = getBitsInRange(w3, 20, 15);
        pulse_hit.pulse_peak = getBitsInRange(w3, 14, 3);
        pulse_hit.time_quality = getBitsInRange(w3, 2, 0);
        
        pulse_hits.push_back(pulse_hit);

        LOG_DEBUG(GetLogger()) << "ModuleParser_FADC::DEBUG - parsePulseData integral_sum = " << pulse_hit.integral_sum  << "; integral_quality = " << pulse_hit.integral_quality <<"; nsamples_above_th = "<< pulse_hit.nsamples_above_th <<LOG_END;
        LOG_DEBUG(GetLogger()) << "ModuleParser_FADC::DEBUG - parsePulseData coarse_time = " << pulse_hit.coarse_time  << "; fine_time = " << pulse_hit.fine_time <<"; pulse_peak = "<< pulse_hit.pulse_peak << "; time_quality = " <<  pulse_hit.time_quality << LOG_END;
    }
    
    return pulse_hits;
}

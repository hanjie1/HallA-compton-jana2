#include <iostream>
#include <map>

#include <JANA/JException.h>

#include "EvioEventParser.h"
#include "EvioEventWrapper.h"

#include "JEventService_FilterDB.h"
#include "JEventService_BankToModuleMap.h"
#include "JEventService_ModuleParsersMap.h"

/**
 * @brief Parse the EVIO event and extract all detector hits
 * 
 * This method orchestrates the parsing of an EVIO event by:
 * 1. Validating the event structure
 * 2. Parsing the trigger bank to extract ROC segments and event number
 * 3. Parsing data banks to extract detector hits using ModuleParser
 */
void EvioEventParser::parse(const JEvent& event, std::vector<PhysicsEvent*>& physics_events) {
    // Get all child structures (Trigger Bank + ROC Banks)
    auto evio_data_block = event.Get<EvioEventWrapper>().at(0)->evio_event;
    auto& children = evio_data_block->getChildren();

    // Parse the trigger bank to extract ROC segments and event number
    TriggerData trigger_data(0);
    auto trigger_bank_roc_segments = parseTriggerBank(children.at(0), trigger_data);
    
    // Parse the data banks while using the trigger bank ROC segments for validation
    std::vector<std::shared_ptr<evio::BaseStructure>> roc_banks(children.begin() + 1, children.end());
    parseROCBanks(roc_banks, trigger_bank_roc_segments, trigger_data, physics_events);

    if (physics_events.empty()) {
        throw JException("EvioEventParser::parse: No physics events found for evio block %d", evio_data_block->getEventNumber());
    }

    // Consolidate PhysicsEvents that share the same event number.
    // Different bank parsers (FADC, Scaler, TI Scaler, VFTDC, MPD, etc.) each
    // create their own PhysicsEvent objects.  Events with the same number are
    // merged here so that a single PhysicsEvent carries hits from all subsystems.
    std::map<int, PhysicsEvent*> event_map;
    for (auto* pe : physics_events) {
        auto it = event_map.find(pe->GetEventNumber());
        if (it != event_map.end()) {
            // Same event number already seen — move hits into the existing PhysicsEvent
            pe->insertHitsInto(*it->second);
            delete pe;
        } else {
            event_map[pe->GetEventNumber()] = pe;
        }
    }
    physics_events.clear();
    for (auto& [num, pe] : event_map) {
        if (num < 0 || static_cast<uint64_t>(num) < trigger_data.first_event_number) {
            throw JException(
                "EvioEventParser::parse: Physics event number %d is before trigger bank first event number %llu",
                num,
                static_cast<unsigned long long>(trigger_data.first_event_number)
            );
        }

        uint64_t event_index = static_cast<uint64_t>(num) - trigger_data.first_event_number;
        if (event_index >= trigger_data.event_timestamps.size()) {
            throw JException(
                "EvioEventParser::parse: No EB1 timestamp for physics event number %d at block index %d",
                num,
                static_cast<int>(event_index)
            );
        }

        pe->SetEventTimestamp(trigger_data.event_timestamps[event_index]);
        physics_events.push_back(pe);
    }
}

/**
 * @brief Parse the trigger bank and extract ROC segments
 * 
 * This method parses the trigger bank structure to:
 * 1. Extract the first event number and per-event timestamps from the EB1 segment
 * 2. Collect all ROC segments (UINT32 data type)
 * 3. Validate the number of ROC segments matches the header
 * 
 * @param trigger_bank  The trigger bank structure to parse
 * @param trigger_data  Output trigger metadata (first event number, etc.)
 * @return Vector of trigger bank ROC segments
 */
std::vector<std::shared_ptr<evio::BaseStructure>> EvioEventParser::parseTriggerBank(std::shared_ptr<evio::BaseStructure> trigger_bank,
                                                                                    TriggerData& trigger_data) {
    // Get the number of ROC segments from the header
    int trigger_bank_rocs_count = static_cast<int>(trigger_bank->getHeader()->getNumber());
    auto trigger_bank_children = trigger_bank->getChildren();
    if (trigger_bank_children.empty()) {
        throw JException("EvioEventParser::parseTriggerBank: Trigger bank has no EB1 segment");
    }
    
    // Extract first event metadata from the first segment (EB1).
    // EB1 layout:
    //   eb1[0]    first event number in the block
    //   eb1[1...] per-event timestamps for this block
    auto eb1_segment = trigger_bank_children.at(0);
    std::vector<uint64_t> eb1_data = eb1_segment->getULongData();
    if (eb1_data.size() < 2) {
        throw JException("EvioEventParser::parseTriggerBank: EB1 segment has %d words, expected at least 2", static_cast<int>(eb1_data.size()));
    }
    trigger_data.first_event_number = static_cast<uint64_t>(eb1_data[0]);
    trigger_data.event_timestamps.assign(eb1_data.begin() + 1, eb1_data.end());
    
    // Collect all ROC segments (UINT32 data type)
    std::vector<std::shared_ptr<evio::BaseStructure>> trigger_bank_rocs_data;
    
    for (auto& child : trigger_bank_children) {
        auto dtype = child->getHeader()->getDataType();
        if (dtype == evio::DataType::UINT32) {
            trigger_bank_rocs_data.push_back(child);
        }
    }
    
    // Validate that the number of ROC segments matches the header
    if(trigger_bank_rocs_count != trigger_bank_rocs_data.size()) {
        throw JException("EvioEventParser::parseTriggerBank: #ROC segments != header #ROCS -- %d != %d", trigger_bank_rocs_count, trigger_bank_rocs_data.size());
    }

    return trigger_bank_rocs_data;
}

/**
 * @brief Parse data banks and extract hits
 * 
 * This method processes the data banks by:
 * 1. Validating that the number of data banks matches trigger bank ROC segments
 * 2. Matching ROC IDs between trigger and data banks
 * 3. Parsing each data block using ModuleParser
 * 
 * @param data_banks                 Vector of data banks to parse
 * @param trigger_bank_roc_segments  Vector of trigger bank ROC segments for validation
 * @param trigger_data               Trigger metadata for this EVIO block
 * @param physics_events             Output vector which will be filled with PhysicsEvent*
 */
void EvioEventParser::parseROCBanks(const std::vector<std::shared_ptr<evio::BaseStructure>>& data_banks,
                                     const std::vector<std::shared_ptr<evio::BaseStructure>>& trigger_bank_roc_segments,
                                     TriggerData& trigger_data,
                                     std::vector<PhysicsEvent*>& physics_events) {
    
    // Validate that the number of data banks matches the number of ROC segments
    // Expected: #ROCs == #TriggerBankROCsegments == #RemainingBanksAfterTriggerBank 
    if(data_banks.size() != trigger_bank_roc_segments.size()) {
        throw JException("EvioEventParser::parseROCBanks: #ROC databanks != #ROC segments in trigger bank -- %d != %d", data_banks.size(), trigger_bank_roc_segments.size());
    }
    
    auto filter_db_svc = m_app->GetService<JEventService_FilterDB>();
    if (filter_db_svc == nullptr) {
        throw JException("EvioEventParser::parseROCBanks: Filter database service not found");
    }

    // Process each data bank
    for (size_t i = 0; i < data_banks.size(); i++) {
        auto db = data_banks[i];
        auto tb = trigger_bank_roc_segments[i];
        
        // Extract ROC IDs for validation
        uint32_t tb_rocid = tb->getHeader()->getTag();
        uint32_t db_rocid = (db->getHeader()->getTag()) & 0x0FFF;  // Mask to get ROC ID
        
        // Validate that ROC IDs match between trigger and data banks
        if(tb_rocid != db_rocid) {
            throw JException("EvioEventParser::parseROCBanks: Trigger bank roc segment rocid != Data bank rocid -- %d != %d", tb_rocid, db_rocid);
        }

        // Check if ROC is allowed
        if (!filter_db_svc->isROCAllowed(db_rocid)) {
            continue;
        }
        
        // Parse one or more DMA banks within this ROC bank using the registered ModuleParsers
        auto dma_blocks = db->getChildren();
        for (auto dma : dma_blocks) {
            auto bank_id = dma->getHeader()->getTag();

            // Check if bank is allowed for this ROC
            if (!filter_db_svc->isBankAllowed(db_rocid, bank_id)) {
                continue;
            }

            // Resolve bank ID -> module ID
            int module_id;
            auto bank_to_module_svc = m_app->GetService<JEventService_BankToModuleMap>();
            if (bank_to_module_svc == nullptr) {
                throw JException("EvioEventParser::parseROCBanks: Bank-to-module mapping service not found");
            }
            module_id = bank_to_module_svc->getModuleId(bank_id);
            if (module_id == -1) {
                // ignore any banks that are not mapped to a module
                continue;
            }

            // Get parser by module ID
            auto module_parser = m_app->GetService<JEventService_ModuleParsersMap>()->getParser(module_id);
            if (module_parser == nullptr) {
                throw JException("EvioEventParser::parseROCBanks: No parser found for module ID %d (bank tag %d)", module_id, bank_id);
            }

            // Set the logger equal to component logger
            module_parser->SetLogger(m_logger);

            // Parse bank using resolved module parser
            module_parser->parse(dma, db_rocid, physics_events, trigger_data);
        }
    }
}

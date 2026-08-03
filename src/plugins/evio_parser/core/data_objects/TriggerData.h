#ifndef _TRIGGER_DATA_H_
#define _TRIGGER_DATA_H_

#include <cstdint>
#include <vector>

/**
 * @class TriggerData
 * @brief Simple POD type holding the first event metadata for an EVIO block
 */
class TriggerData {
public:
   uint64_t first_event_number;
   std::vector<uint64_t> event_timestamps;

   TriggerData()
      : first_event_number(0) {}

   TriggerData(uint64_t first_event_number)
      : first_event_number(first_event_number) {}
};

#endif // _TRIGGER_DATA_H_

#ifndef _JEventUnfolder_EVIO_h_
#define _JEventUnfolder_EVIO_h_

#include <JANA/JEventUnfolder.h>
#include "PhysicsEvent.h"
#include "FADC250WaveformHit.h"
#include "FADC250PulseHit.h"

/**
 * @class JEventUnfolder_EVIO
 * @brief Unfolder that generates child physics events from block-level events
 * 
 * This unfolder takes PhysicsEvent objects from block-level events and creates
 * individual physics event-level child events, extracting waveform and pulse hits
 * for each physics event.
 */
class JEventUnfolder_EVIO : public JEventUnfolder {
private:
    Input<PhysicsEvent> events_in {this};  

public:
    /**
     * @brief Constructor
     * 
     * Sets the parent level to Block and child level to PhysicsEvent.
     */
    JEventUnfolder_EVIO() {  
        SetParentLevel(JEventLevel::Block);  
        SetChildLevel(JEventLevel::PhysicsEvent);  
    }  

    /**
     * @brief Unfold a physics event from the block-level parent event
     * 
     * Extracts a PhysicsEvent at the given iteration index and creates a child
     * event with the corresponding waveform and pulse hits.
     * 
     * Data flow:
     * 1. JEventSource_EVIO reads EVIO file and creates block-level events with EvioEventWrapper
     * 2. JEventSource_EVIO::ProcessParallel uses EvioEventParser and ModuleParser implementations
     *    to decode EVIO banks into PhysicsEvent objects and inserts them into the block event
     * 3. This unfolder creates child physics events, each containing info from one PhysicsEvent
     * 
     * @param parent The block-level parent event containing PhysicsEvent objects
     * @param child The physics event-level child event to populate with hits
     * @param iter Index of the child event
     * @return Result indicating whether to continue with next child/parent
     */
    Result Unfold(const JEvent& parent, JEvent& child, int iter) override {  
        // Get all physics events from the block-level parent event
        auto& physics_events = events_in();
        
        // Safety check: if index is out of bounds, throw an exception
        if (iter >= physics_events.size()) {
            throw JException("Index out of bounds in JEventUnfolder_EVIO");
        }
        
        // Extract the physics event at the current iteration index
        auto physics_event = physics_events[iter];  
          
        // Set event metadata for the child event
        child.SetEventNumber(physics_event->GetEventNumber());   
        child.SetRunNumber(parent.GetRunNumber());       
        
        // Extract hits from the physics event and set them as outputs
        // These hits will be available to processors at the physics event level
        child.Insert(new PhysicsEvent(*physics_event));
        physics_event->insertHitsIntoEvent(child);
  
        // Return appropriate result based on whether this is the last physics event
        // If last event, move to next parent; otherwise, keep parent and process next child
        return (iter == physics_events.size() - 1 ? Result::NextChildNextParent : Result::NextChildKeepParent);  
    }  
};

#endif // _JEventUnfolder_EVIO_h_

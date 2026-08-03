
#ifndef _JEventProcessor_EVIO_h_
#define _JEventProcessor_EVIO_h_

#include <TFile.h>
#include <TTree.h>
#include <TH1.h>

#include <fstream>
#include <string>
#include <vector>

#include <JANA/JEventProcessor.h>
#include "PhysicsEvent.h"
#include "EventHits_FADC.h"
#include "FADCScalerHit.h"
#include "TIScalerHit.h"
#include "HelicityDecoderData.h"
#include "MPDHit.h"
#include "VFTDCHit.h"
#include "FADC250HallBPulseIntegralHit.h"
#include "FADC250HallBPulseTimeHit.h"
#include "FADC250HallBPulsePeakHit.h"

/**
 * @struct WaveformTreeRow
 * @brief Data structure representing one row in the waveform ROOT TTree
 * 
 * Contains the waveform hit information to be stored in ROOT Tree:
 * - slot: FADC250 slot number
 * - chan: Channel number within the slot
 * - waveform: Vector of ADC sample values
 */
struct WaveformTreeRow {
    uint32_t slot;
    uint32_t chan;
    std::vector<uint32_t> waveform;
};

struct HelDec_t {
    uint32_t helicity_seed;
    uint32_t n_tstable_fall;
    uint32_t n_tstable_rise;
    uint32_t n_pattsync;
    uint32_t n_pairsync;
    uint32_t time_tstable_start;
    uint32_t time_tstable_end;
    uint32_t last_tstable_duration;
    uint32_t last_tsettle_duration;
    uint32_t trig_tstable;
    uint32_t trig_pattsync;
    uint32_t trig_pairsync;
    uint32_t trig_helicity;
    uint32_t trig_pat0_helicity;
    uint32_t trig_polarity;
    uint32_t trig_pat_count;
    uint32_t last32wins_pattsync;
    uint32_t last32wins_pairsync;
    uint32_t last32wins_helicity;
    uint32_t last32wins_pattsync_hel;
};

/**
 * @class JEventProcessor_EVIO
 * @brief Main event processor for FADC250 detector data analysis
 * 
 * This processor receives FADC250 detector hits (both waveform and pulse hits)
 * and outputs the data to a ROOT file containing:
 * - Waveform TTree: Channel-by-channel waveform data
 * - Pulse integral histogram: Distribution of pulse integral sums
 * 
 * The output filename can be customized via JANA2 parameters.
 */
class JEventProcessor_EVIO : public JEventProcessor {

private:
    // Declare Inputs
    Input<FADC250WaveformHit>          m_waveform_hits_in {this}; 
    Input<FADC250PulseHit>             m_pulse_hits_in {this};
    Input<FADCScalerHit>               m_fadc_scaler_hits_in {this};
    Input<TIScalerHit>                 m_ti_scaler_hits_in {this};
    Input<HelicityDecoderData>         m_heldec_data_in {this};
    Input<MPDHit>                      m_mpd_hits_in {this};
    Input<VFTDCHit>                    m_vftdc_hits_in {this};
    Input<FADC250HallBPulseIntegralHit> m_hallb_pulse_integral_hits_in {this};
    Input<FADC250HallBPulseTimeHit>    m_hallb_pulse_time_hits_in {this};
    Input<FADC250HallBPulsePeakHit>    m_hallb_pulse_peak_hits_in {this};
    Input<PhysicsEvent>                m_physics_event_in {this};

    /**
     * @brief ROOT output filename parameter
     * 
     * This parameter allows users to specify the ROOT output filename via JANA2 configuration.
     * The parameter constructor takes the following arguments:
     * - owner: Pointer to this component (for parameter registration)
     * - name: "ROOT_OUT_FILENAME" - the parameter name used in configuration files/command line
     * - default_value: "evio_processor.root" - default filename if not specified
     * - description: "Output file name for root data" - help text for the parameter
     * - is_shared: if true, the parameter name is used as-is;  
     *              if false (default), the component's prefix (set in the constructor) is prepended to the name.
     */
    Parameter<std::string> m_root_output_filename {this, "ROOT_OUT_FILENAME", "evio_processor.root", "Output file name for ROOT data", true};

    /**
     * @brief Text output filename parameter
     *
     * Text file where per-event summaries of waveform, pulse, and scaler
     * hits are written.
     */
    Parameter<std::string> m_txt_output_filename {this, "TXT_OUT_FILENAME", "evio_processor_hits.txt", "Output text file name for event hit summaries", true};

    // ROOT Tree variables 
    uint64_t ev_event_number;
    uint64_t ev_event_timestamp;
    std::vector<uint32_t> ev_slot;
    std::vector<uint32_t> ev_chan;
    std::vector<uint32_t> ev_waveform;
    uint32_t integral_sum;
    uint32_t coarse_time;
    uint32_t fine_time;
    uint32_t pulse_peak;
    uint32_t pedestal_sum;
    uint32_t pedestal_quality;
    int number_hit;
    std::vector<uint32_t> ev_integral_sum;
    std::vector<uint32_t> ev_coarse_time;
    std::vector<uint32_t> ev_fine_time;
    std::vector<uint32_t> ev_pulse_peak;
    std::vector<uint32_t> ev_pulse_slot;
    std::vector<uint32_t> ev_pulse_chan;

    // helicity decoder tree variables
    HelDec_t heldec{};

    // ROOT output objects
    TFile *m_root_output_file;                ///< ROOT file for histogram and tree storage
    WaveformTreeRow m_waveform_tree_row;      ///< Data structure holding the current row for TTree filling
    TTree *m_waveform_tree;                   ///< ROOT tree for waveform data
    TTree *m_tree;                            ///< ROOT tree for physics event
    TH1I *m_pulse_integral_hist;              ///< Histogram of pulse integral sums
    TTree *m_pulse_tree;                      ///< ROOT tree for pulse hit
    
    // Text output for human-readable dump of hits per event
    std::ofstream m_txt_output_file;

public:

    JEventProcessor_EVIO();
    virtual ~JEventProcessor_EVIO() = default;

    void Init() override;
    void ProcessSequential(const JEvent& event) override;
    void Finish() override;

};

#endif // _JEventProcessor_EVIO_h_

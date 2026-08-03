#include "JEventProcessor_EVIO.h"
#include <JANA/JLogger.h>

/**
 * @brief Constructor for JEventProcessor_EVIO
 * 
 * Initialize the processor with the appropriate type name, prefix, and callback style.
 */
JEventProcessor_EVIO::JEventProcessor_EVIO() {
    SetTypeName(NAME_OF_THIS);                    // Provide JANA with this class's name
    SetPrefix("jeventprocessor_evio");            // Set unique prefix for parameters
    SetCallbackStyle(CallbackStyle::ExpertMode);  // Use expert mode for full control

    // All of these are optional because not all events will have these hits
    m_fadc_scaler_hits_in.SetOptional(true);
    m_pulse_hits_in.SetOptional(true);
    m_waveform_hits_in.SetOptional(true);
    m_ti_scaler_hits_in.SetOptional(true);
    m_heldec_data_in.SetOptional(true);
    m_mpd_hits_in.SetOptional(true);
    m_vftdc_hits_in.SetOptional(true);
    m_hallb_pulse_integral_hits_in.SetOptional(true);
    m_hallb_pulse_time_hits_in.SetOptional(true);
    m_hallb_pulse_peak_hits_in.SetOptional(true);
    m_physics_event_in.SetOptional(true);
}

/**
 * @brief Initialize the processor
 * 
 * Called once at the start of processing. Open the output files and set up
 * any necessary resources for event processing.
 */
void JEventProcessor_EVIO::Init() {
    LOG << "JEventProcessor_EVIO::Init" << LOG_END;
    
    // Open the ROOT output file
    m_root_output_file = new TFile(m_root_output_filename().c_str(), "RECREATE");
    if (m_root_output_file == nullptr || m_root_output_file->IsZombie()) {
        throw JException("Failed to open ROOT output file: " + m_root_output_filename());  
    }

    // Initialize the waveform tree row data structure
    m_waveform_tree_row = WaveformTreeRow();

    // Create ROOT tree for waveform data
    m_waveform_tree = new TTree("waveform_tree", "FADC250 Waveform Data (slot, channel, waveform)");
    m_waveform_tree->Branch("event_number", &ev_event_number);
    m_waveform_tree->Branch("event_timestamp", &ev_event_timestamp);
    m_waveform_tree->Branch("slot", &ev_slot);
    m_waveform_tree->Branch("chan", &ev_chan);
    m_waveform_tree->Branch("waveform", &ev_waveform);

    // Create ROOT tree for pulse data
    m_pulse_tree = new TTree("pulse_tree","FADC250 pulse data(slow, channel, integral, time)");
    m_pulse_tree->Branch("event_number", &ev_event_number);
    m_pulse_tree->Branch("event_timestamp", &ev_event_timestamp);
    m_pulse_tree->Branch("integral_sum", &ev_integral_sum);
    m_pulse_tree->Branch("pedestal_sum", &pedestal_sum);
    m_pulse_tree->Branch("coarse_time",&ev_coarse_time);
    m_pulse_tree->Branch("fine_time",&ev_fine_time);
    m_pulse_tree->Branch("pulse_peak",&ev_pulse_peak);
    m_pulse_tree->Branch("pedestal_quality",&pedestal_quality);
    m_pulse_tree->Branch("nhits",&number_hit);
    m_pulse_tree->Branch("chan",&ev_pulse_chan);
    m_pulse_tree->Branch("slot",&ev_pulse_slot);

    // Create the Helicity Decoder Tree
    m_tree = new TTree("m_tree", "Physics Event Tree");
    m_tree->Branch("event_number", &ev_event_number);
    m_tree->Branch("event_timestamp", &ev_event_timestamp);
    m_tree->Branch(
        "heldec",
        &heldec,
        "helicity_seed/i:"
        "n_tstable_fall/i:"
        "n_tstable_rise/i:"
        "n_pattsync/i:"
        "n_pairsync/i:"
        "time_tstable_start/i:"
        "time_tstable_end/i:"
        "last_tstable_duration/i:"
        "last_tsettle_duration/i:"
        "trig_tstable/i:"
        "trig_pattsync/i:"
        "trig_pairsync/i:"
        "trig_helicity/i:"
        "trig_pat0_helicity/i:"
        "trig_polarity/i:"
        "trig_pat_count/i:"
        "last32wins_pattsync/i:"
        "last32wins_pairsync/i:"
        "last32wins_helicity/i:"
        "last32wins_pattsync_hel/i"
    );

    // Optionally: Text output file for human-readable hit summaries
    m_txt_output_file.open(m_txt_output_filename().c_str());

    // Create histogram for pulse integral distribution
    m_pulse_integral_hist = new TH1I("h_integral", "Pulse Integral Distribution;Integral Sum;Counts", 100, 0, 1);
    m_pulse_integral_hist->SetCanExtend(TH1::kAllAxes);  // Allow ROOT to automatically extend bins
}

/**
 * @brief Process a single event sequentially
 * 
 * Processes FADC250 detector data for a single event. Fills ROOT tree with
 * waveform data and histogram with pulse integral values. This method is 
 * called for each event in the processing pipeline.
 * 
 * @param event Reference to the JANA2 event to process
 */
void JEventProcessor_EVIO::ProcessSequential(const JEvent &event) {
    
    // Clear previous event data
    ev_event_number = event.GetEventNumber();
    ev_event_timestamp = 0;
    const auto& physics_events = m_physics_event_in();
    if (!physics_events.empty()) {
        ev_event_timestamp = physics_events.at(0)->GetEventTimestamp();
    }
    ev_slot.clear();
    ev_chan.clear();
    ev_waveform.clear();
    ev_coarse_time.clear();
    ev_pulse_chan.clear();
    ev_pulse_slot.clear();
    ev_fine_time.clear();
    ev_integral_sum.clear();
    ev_pulse_peak.clear();
    pedestal_sum= 0;
    pedestal_quality = 0;
    number_hit =0;

    // FADC250 waveform hits
    for (const auto& waveform_hit : m_waveform_hits_in()) {
        // Fill ROOT tree with waveform data
        m_waveform_tree_row.slot = waveform_hit->slot;
        m_waveform_tree_row.chan = waveform_hit->chan;
        m_waveform_tree_row.waveform = waveform_hit->waveform;

	size_t waveform_sample_number = m_waveform_tree_row.waveform.size();

	ev_slot.insert(ev_slot.end(), waveform_sample_number, m_waveform_tree_row.slot);
	ev_chan.insert(ev_chan.end(), waveform_sample_number, m_waveform_tree_row.chan);
	ev_waveform.insert(ev_waveform.end(),  m_waveform_tree_row.waveform.begin(), m_waveform_tree_row.waveform.end());
    }

    // FADC250 pulse hits
 
    int nn=0;
    for (const auto& pulse_hit : m_pulse_hits_in()){
        integral_sum = pulse_hit->integral_sum;
        pedestal_sum = pulse_hit->pedestal_sum;
        coarse_time = pulse_hit->coarse_time;
        fine_time = pulse_hit->fine_time;
        pulse_peak = pulse_hit->pulse_peak;
        pedestal_quality = pulse_hit->pedestal_quality;
        if(integral_sum!=0){
            nn++;
            ev_integral_sum.push_back(integral_sum);
            ev_coarse_time.push_back(coarse_time);
            ev_fine_time.push_back(fine_time);
            ev_pulse_peak.push_back(pulse_peak);
            ev_pulse_slot.push_back(pulse_hit->slot);
            ev_pulse_chan.push_back(pulse_hit->chan);
        }
       
        
    }
    number_hit = nn;
    m_waveform_tree->Fill();
    if(nn>0){
        m_pulse_tree->Fill();
    }


    // FADC250 pulse hits
    for (const auto& pulse_hit : m_pulse_hits_in()) {
        // Fill histogram with pulse integral values
        m_pulse_integral_hist->Fill(pulse_hit->integral_sum);
    }

    heldec = {};
    // Helicity decoder data
    for(const auto& heldec_hit : m_heldec_data_in()){
	heldec.helicity_seed          = heldec_hit->helicity_seed;
        heldec.n_tstable_fall         = heldec_hit->n_tstable_fall;
        heldec.n_tstable_rise         = heldec_hit->n_tstable_rise;
        heldec.n_pattsync             = heldec_hit->n_pattsync;
        heldec.n_pairsync             = heldec_hit->n_pairsync;
        heldec.time_tstable_start     = heldec_hit->time_tstable_start;
        heldec.time_tstable_end       = heldec_hit->time_tstable_end;
        heldec.last_tstable_duration  = heldec_hit->last_tstable_duration;
        heldec.last_tsettle_duration  = heldec_hit->last_tsettle_duration;
        heldec.trig_tstable           = heldec_hit->trig_tstable;
        heldec.trig_pattsync          = heldec_hit->trig_pattsync;
        heldec.trig_pairsync          = heldec_hit->trig_pairsync;
        heldec.trig_helicity          = heldec_hit->trig_helicity;
        heldec.trig_pat0_helicity     = heldec_hit->trig_pat0_helicity;
        heldec.trig_polarity          = heldec_hit->trig_polarity;
        heldec.trig_pat_count         = heldec_hit->trig_pat_count;
        heldec.last32wins_pattsync    = heldec_hit->last32wins_pattsync;
        heldec.last32wins_pairsync    = heldec_hit->last32wins_pairsync;
        heldec.last32wins_helicity    = heldec_hit->last32wins_helicity;
        heldec.last32wins_pattsync_hel= heldec_hit->last32wins_pattsync_hel;
        m_tree->Fill();
    }

    // ------------------------------------------------------------------
    // Optional text dump of hits for this event (waveforms, pulses, scalers)
    // ------------------------------------------------------------------
    if (m_txt_output_file.is_open()) {
        const auto& waveform_hits            = m_waveform_hits_in();
        const auto& pulse_hits               = m_pulse_hits_in();
        const auto& fadc_scaler_hits         = m_fadc_scaler_hits_in();
        const auto& ti_scaler_hits           = m_ti_scaler_hits_in();
        const auto& mpd_hits                 = m_mpd_hits_in();
        const auto& vftdc_hits               = m_vftdc_hits_in();
        const auto& hallb_pulse_integral_hits = m_hallb_pulse_integral_hits_in();
        const auto& hallb_pulse_time_hits    = m_hallb_pulse_time_hits_in();
        const auto& hallb_pulse_peak_hits    = m_hallb_pulse_peak_hits_in();

        bool have_waveforms              = !waveform_hits.empty();
        bool have_pulses                 = !pulse_hits.empty();
        bool have_fadc_scalers           = !fadc_scaler_hits.empty();
        bool have_ti_scalers             = !ti_scaler_hits.empty();
        bool have_mpd_hits               = !mpd_hits.empty();
        bool have_vftdc_hits             = !vftdc_hits.empty();
        bool have_hallb_pulse_integrals  = !hallb_pulse_integral_hits.empty();
        bool have_hallb_pulse_times      = !hallb_pulse_time_hits.empty();
        bool have_hallb_pulse_peaks      = !hallb_pulse_peak_hits.empty();
        // Only write anything if we have at least one type of hit
        if (have_waveforms || have_pulses || have_fadc_scalers || have_ti_scalers || have_mpd_hits || have_vftdc_hits
            || have_hallb_pulse_integrals || have_hallb_pulse_times || have_hallb_pulse_peaks) {
            auto event_number = event.GetEventNumber();

            m_txt_output_file << "Event " << event_number << "\n";

            // Waveform summary
            if (have_waveforms) {
                m_txt_output_file << "  Waveform hits: " << waveform_hits.size() << "\n";
                for (const auto& hit : waveform_hits) {
                    m_txt_output_file
                        << "    WF rocid=" << hit->rocid
                        << " slot=" << hit->slot
                        << " chan=" << hit->chan
                        << " nsamples=" << hit->waveform.size()
                        << "\n";
                }
            } else {
                m_txt_output_file << "  No FADC250 waveform hits in this event\n";
            }

            // Pulse summary
            if (have_pulses) {
                m_txt_output_file << "  Pulse hits: " << pulse_hits.size() << "\n";
                for (const auto& hit : pulse_hits) {
                    m_txt_output_file
                        << "    PULSE rocid=" << hit->rocid
                        << " slot=" << hit->slot
                        << " chan=" << hit->chan
                        << " integral_sum=" << hit->integral_sum
                        << "\n";
                }
            } else {
                m_txt_output_file << "  No FADC250 pulse hits in this event\n";
            }

            // FADC scaler summary
            if (have_fadc_scalers) {
                m_txt_output_file << "  FADC scaler hits: " << fadc_scaler_hits.size() << "\n";
                for (const auto& hit : fadc_scaler_hits) {
                    m_txt_output_file
                        << "    SCALER rocid=" << hit->rocid
                        << " slot=" << hit->slot
                        << " ncounts=" << hit->ncounts
                        << " counts=";
                    for (uint32_t i = 0; i < hit->ncounts && i < 16u; ++i) {
                        m_txt_output_file << hit->counts[i];
                        if (i + 1 < hit->ncounts && i + 1 < 16u) {
                            m_txt_output_file << ",";
                        }
                    }
                    m_txt_output_file << "\n";
                }
            } else {
                m_txt_output_file << "  No FADCScalerHit objects in this event\n";
            }

            // TI scaler summary
            if (have_ti_scalers) {
                m_txt_output_file << "  TI scaler hits: " << ti_scaler_hits.size() << "\n";
                for (const auto& hit : ti_scaler_hits) {
                    m_txt_output_file
                        << "    TISCALER rocid=" << hit->rocid
                        << " slot=" << hit->slot
                        << " nwords=" << hit->nscalerwords
                        << " live_time=" << hit->live_time
                        << " busy_time=" << hit->busy_time
                        << " ts_inputs_before_busy=" << hit->ts_inputs_before_busy
                        << "\n";
                }
            } else {
                m_txt_output_file << "  No TIScalerHit objects in this event\n";
            }

            // MPD hit summary
            if (have_mpd_hits) {
                m_txt_output_file << "  MPD hits: " << mpd_hits.size() << "\n";
                for (const auto& hit : mpd_hits) {
                    m_txt_output_file
                        << "    MPD rocid=" << hit->rocid
                        << " slot=" << hit->slot
                        << " trigger_num=" << hit->trigger_num
                        << " trigger_time=" << hit->trigger_time
                        << " mpd_id=" << (int)hit->mpd_id
                        << " fiber_id=" << (int)hit->fiber_id
                        << " apv_channel=" << (int)hit->apv_channel
                        << " apv_id=" << (int)hit->apv_id
                        << " apv_samples=[";
                    for (int i = 0; i < 6; ++i) {
                        m_txt_output_file << hit->apv_samples[i];
                        if (i < 5) m_txt_output_file << ",";
                    }
                    m_txt_output_file << "]\n";
                }
            } else {
                m_txt_output_file << "  No MPDHit objects in this event\n";
            }

            // VFTDC hit summary
            if (have_vftdc_hits) {
                m_txt_output_file << "  VFTDC hits: " << vftdc_hits.size() << "\n";
                for (const auto& hit : vftdc_hits) {
                    m_txt_output_file
                        << "    VFTDC rocid=" << hit->rocid
                        << " slot=" << hit->slot
                        << " board_id=" << hit->board_id
                        << " timestamp=" << hit->timestamp
                        << " group_num=" << hit->group_num
                        << " channel_num=" << hit->channel_num
                        << " edge_type=" << hit->edge_type
                        << " coarse_time=" << hit->coarse_time
                        << " fine_time=" << hit->fine_time
                        << " two_ns=" << hit->two_ns
                        << "\n";
                }
            } else {
                m_txt_output_file << "  No VFTDCHit objects in this event\n";
            }

            // HallB pulse integral summary
            if (have_hallb_pulse_integrals) {
                m_txt_output_file << "  HallB pulse integral hits: " << hallb_pulse_integral_hits.size() << "\n";
                for (const auto& hit : hallb_pulse_integral_hits) {
                    m_txt_output_file
                        << "    HALLB_INTEGRAL slot=" << hit->slot
                        << " chan=" << hit->chan
                        << " pulse_number=" << hit->pulse_number
                        << " pulse_integral=" << hit->pulse_integral
                        << "\n";
                }
            } else {
                m_txt_output_file << "  No HallB pulse integral hits in this event\n";
            }

            // HallB pulse time summary
            if (have_hallb_pulse_times) {
                m_txt_output_file << "  HallB pulse time hits: " << hallb_pulse_time_hits.size() << "\n";
                for (const auto& hit : hallb_pulse_time_hits) {
                    m_txt_output_file
                        << "    HALLB_TIME slot=" << hit->slot
                        << " chan=" << hit->chan
                        << " pulse_number=" << hit->pulse_number
                        << " measurement_quality_factor=" << hit->measurement_quality_factor
                        << " coarse_pulse_time=" << hit->coarse_pulse_time
                        << " fine_pulse_time=" << hit->fine_pulse_time
                        << "\n";
                }
            } else {
                m_txt_output_file << "  No HallB pulse time hits in this event\n";
            }

            // HallB pulse peak summary
            if (have_hallb_pulse_peaks) {
                m_txt_output_file << "  HallB pulse peak hits: " << hallb_pulse_peak_hits.size() << "\n";
                for (const auto& hit : hallb_pulse_peak_hits) {
                    m_txt_output_file
                        << "    HALLB_PEAK slot=" << hit->slot
                        << " chan=" << hit->chan
                        << " pulse_number=" << hit->pulse_number
                        << " Vmin=" << hit->Vmin
                        << " Vpeak=" << hit->Vpeak
                        << "\n";
                }
            } else {
                m_txt_output_file << "  No HallB pulse peak hits in this event\n";
            }

            m_txt_output_file << "\n";
        }
    }
}

/**
 * @brief Finish processing and cleanup
 * 
 * Called once at the end of processing. Close the output file and perform
 * any necessary cleanup operations.
 */
void JEventProcessor_EVIO::Finish() {
    LOG << "JEventProcessor_EVIO::Finish" << LOG_END;

    // Write ROOT objects and close ROOT file
    if (m_root_output_file) {
        m_waveform_tree->Write();        // Save waveform tree to file
        m_pulse_integral_hist->Write();  // Save integral histogram to file
	    m_tree->Write();
        m_pulse_tree->Write();           // Save pulse tree to file
        m_root_output_file->Close();     // Close ROOT file
        delete m_root_output_file;       // Free memory
        m_root_output_file = nullptr;
    }

    // Close text output file if open
    if (m_txt_output_file.is_open()) {
        m_txt_output_file.close();
    }
}

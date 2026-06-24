
#ifndef _JBlockProcessor_EVIO_h_
#define _JBlockProcessor_EVIO_h_

#include <TFile.h>
#include <TTree.h>
#include <TH1.h>

#include <fstream>
#include <string>
#include <vector>

#include <JANA/JEventProcessor.h>
#include "EvioEventWrapper.h"

/**
 * @struct WaveformTreeRow
 * @brief Data structure representing one row in the waveform ROOT TTree
 * 
 * Contains the waveform hit information to be stored in ROOT Tree:
 * - slot: FADC250 slot number
 * - chan: Channel number within the slot
 * - waveform: Vector of ADC sample values
 */


/**
 * @class JBlockProcessor_EVIO
 * @brief Main event processor for FADC250 detector data analysis
 * 
 * This processor receives FADC250 detector hits (both waveform and pulse hits)
 * and outputs the data to a ROOT file containing:
 * - Waveform TTree: Channel-by-channel waveform data
 * - Pulse integral histogram: Distribution of pulse integral sums
 * 
 * The output filename can be customized via JANA2 parameters.
 */
class JBlockProcessor_EVIO : public JEventProcessor {

private:
    // Declare Inputs
    Input<EvioEventWrapper>    m_evio_block_in {this};

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
    Parameter<std::string> m_root_output_filename {this, "ROOT_OUT_FILENAME", "block_processor.root", "Output file name for ROOT data", true};

    // ROOT Tree variables 
    uint32_t block_size;
    std::vector<uint32_t> rocid;
    std::vector<uint32_t> bankid;
    std::vector<uint32_t> bank_size;

    // ROOT output objects
    TFile *m_root_output_file;                ///< ROOT file for histogram and tree storage
    TTree *m_block_tree;                   ///< ROOT tree for data sizes
    
public:

    JBlockProcessor_EVIO();
    virtual ~JBlockProcessor_EVIO() = default;

    void Init() override;
    void ProcessSequential(const JEvent& event) override;
    void Finish() override;

};

#endif // _JBlockProcessor_EVIO_h_


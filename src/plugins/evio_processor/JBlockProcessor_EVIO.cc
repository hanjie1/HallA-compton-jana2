#include "JBlockProcessor_EVIO.h"
#include <JANA/JLogger.h>

/**
 * @brief Constructor for JBlockProcessor_EVIO
 * 
 * Initialize the processor with the appropriate type name, prefix, and callback style.
 */
JBlockProcessor_EVIO::JBlockProcessor_EVIO() {
    SetTypeName(NAME_OF_THIS);                    // Provide JANA with this class's name
    SetPrefix("jblockprocessor_evio");            // Set unique prefix for parameters
    SetCallbackStyle(CallbackStyle::ExpertMode);  // Use expert mode for full control
    SetLevel(JEventLevel::Block);                 // Set block level processing
}

/**
 * @brief Initialize the processor
 * 
 * Called once at the start of processing. Open the output files and set up
 * any necessary resources for block processing.
 */
void JBlockProcessor_EVIO::Init() {
    LOG << "JBlockProcessor_EVIO::Init" << LOG_END;
    

    // Create ROOT tree for block size data
    m_block_tree = new TTree("block_tree", "Block sizes");
    m_block_tree->Branch("block_size", &block_size);
    m_block_tree->Branch("rocid", &rocid);
    m_block_tree->Branch("bankid", &bankid);
    m_block_tree->Branch("bank_size", &bank_size);

}

/**
 * @brief Process a single block sequentially
 * 
 * @param block Reference to the JANA2 block to process
 */
void JBlockProcessor_EVIO::ProcessSequential(const JEvent &block) {
    
    // Clear previous block data
    rocid.clear();
    bankid.clear();
    bank_size.clear();

    for (const auto& a_bank : m_evio_block_in()) {
         block_size = a_bank->getLength();
   
         auto evio_data_block = a_bank.at(0)->evio_event;
         auto& children = evio_data_block->getChildren();

	 /* trigger bank */
	 rocid.insert(rocid.end(), 0);
	 bankid.insert(bankid.end(), 0);
	 bank_size.insert(band_size.end(), children.at(0)->getDataLength());

         /* ROC bansk */
	 for (size_t i = 1; i < children.size(); ++i) {
	     uint32_t tmp_rocid = (children.at(i)->getHeader()->getTag()) & 0x0FFF;

	     auto dma_blocks = children.at(i)->getChildren();
	     for( const auto dma : dma_blocks){
                  rocid.insert( rocid.end(), tmp_rocid );
		  bankid.insert( bankid.end(), dma->getHeader()->getTag() );
		  bank_size.insert( bank_size.end(), dma->getDataLength() );
	     } 
         }

         m_block_tree->Fill();
    }

 
     
}

/**
 * @brief Finish processing and cleanup
 * 
 * Called once at the end of processing. Close the output file and perform
 * any necessary cleanup operations.
 */
void JBlockProcessor_EVIO::Finish() {
    LOG << "JBlockProcessor_EVIO::Finish" << LOG_END;

    // Write ROOT objects and close ROOT file
    if (m_root_output_file) {
        m_block_tree->Write();        // Save block tree to file
        delete m_root_output_file;       // Free memory
        m_root_output_file = nullptr;
    }
}


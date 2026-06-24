#include <JANA/JApplication.h>
#include "JEventProcessor_EVIO.h"
#include "JBlockProcessor_EVIO.h"

extern "C" {
    void InitPlugin(JApplication* app) {
        InitJANAPlugin(app);
        app->Add(new JEventProcessor_EVIO());
        app->Add(new JBlockProcessor_EVIO());
    }
}

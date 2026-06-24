#include <JANA/JApplication.h>
#include "JEventProcessor_EVIO.h"

extern "C" {
    void InitPlugin(JApplication* app) {
        InitJANAPlugin(app);
        app->Add(new JEventProcessor_EVIO());
    }
}

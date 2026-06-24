#include <JANA/JApplication.h>
#include "JBlockProcessor_EVIO.h"

extern "C" {
    void InitBlockPlugin(JApplication* app) {
        InitJANAPlugin(app);
        app->Add(new JBlockProcessor_EVIO());
    }
}

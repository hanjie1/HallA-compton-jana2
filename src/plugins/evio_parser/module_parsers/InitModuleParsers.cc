#ifndef INITMODULEPARSERS_H
#define INITMODULEPARSERS_H

#include <JANA/JApplication.h>
#include "JEventService_ModuleParsersMap.h"

// Module parsers
#include "ModuleParser_FADC.h"
#include "ModuleParser_FADCScaler.h"
#include "ModuleParser_TIScaler.h"
#include "ModuleParser_HelicityDecoder.h"
#include "ModuleParser_MPD.h"
#include "ModuleParser_VFTDC.h"
#include "ModuleParser_MOLLERADC.h"

void InitModuleParsers(JApplication* app) {
    // Get the module parsers service
    auto module_parsers_svc = app->GetService<JEventService_ModuleParsersMap>();

    // Register all module parsers
    // Format: module_parsers_svc->addParser(module_id, new ModuleParser_instance());
    module_parsers_svc->addParser(250,   new ModuleParser_FADC());
    module_parsers_svc->addParser(9250,  new ModuleParser_FADCScaler());
    module_parsers_svc->addParser(9001,  new ModuleParser_TIScaler());
    module_parsers_svc->addParser(0xdec, new ModuleParser_HelicityDecoder());
    module_parsers_svc->addParser(3561,  new ModuleParser_MPD());
    module_parsers_svc->addParser(9,     new ModuleParser_VFTDC());
    module_parsers_svc->addParser(77,    new ModuleParser_MOLLERADC());
}

#endif

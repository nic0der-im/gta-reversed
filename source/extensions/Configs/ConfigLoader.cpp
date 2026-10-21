#include "StdInc.h"

#include "extensions/Configs/ConfigLoader.h"

#include "extensions/Configuration.hpp"
#include "extensions/Configs/FastLoader.hpp"
#include "extensions/Configs/Miscellaneous.hpp"

namespace notsa {

void LoadConfigurations() {
    // Firstly load the INI into the memory.
    g_ConfigurationMgr.Load(DEFAULT_INI_FILENAME);

    // Then load all specific configurations.
    g_FastLoaderConfig.Load();
    g_MiscConfig.Load();
    // ...
}

} // namespace notsa

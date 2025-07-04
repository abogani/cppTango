#ifndef Config_h
#define Config_h

#include <string>

namespace Tango {
    const std::string kProjectBinaryDir{"@PROJECT_BINARY_DIR@/tests"};
    const std::string kStartServerCmd{"@PROJECT_BINARY_DIR@/tests/environment/start_server.sh "};
    const std::string kKillServerCmd{"@PROJECT_BINARY_DIR@/tests/environment/kill_server.sh"};
}

#endif

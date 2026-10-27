


#include "../Registry/Registry.h"
#include "../App/App.h"
#include "../App/Logger.h"

int main() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    stdout_logger.log("CoInitializeEx: 0x", std::hex, hr);
    #ifdef _DEBUG
    stdout_logger.log("Project: ", PROJECT_NAME);
    stdout_logger.log("Version: ", PROJECT_VERSION);
    stdout_logger.log("Engine Version: ", ENGINE_VERSION);
    stdout_logger.log("Debug Mode");
    #else
    stdout_logger.log("Release Mode");
    #endif

    auto app = App(Registry::registryCallback);
    CoUninitialize();

    return 0;
}
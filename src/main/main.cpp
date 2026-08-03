


#include "../Registry/Registry.h"
#include "../App/App.h"

int main() {

    #ifdef _DEBUG
    std::cout << "Project: " << PROJECT_NAME << std::endl;
    std::cout << "Version: " << PROJECT_VERSION << std::endl;
    std::cout << "Engine Version: " << ENGINE_VERSION << std::endl;
    std::cout << "Debug Mode" << std::endl;
    #else
    std::cout << "Release Mode" << std::endl;
    #endif

    auto app = App(Registry::registryCallback);


    return 0;
}
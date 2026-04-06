#include <stdio.h>
#include <iostream>

int main() {
    std::cout << PROJECT_NAME << std::endl;
    #ifdef _DEBUG
        std::cout << "Debug mode" << std::endl;
    #else
        std::cout << "Release mode" << std::endl;
    #endif
    return 0;
}
/**
 * @file main.cpp
 * @brief Main file of the project.
 */

#include <stdio.h>
#include <iostream>


#ifndef PROJECT_NAME /** @brief .exe file and window name*/
#define PROJECT_NAME "Unknown"
#endif

int main() {
    std::cout << PROJECT_NAME << std::endl;
    #ifdef _DEBUG
        std::cout << "Debug mode" << std::endl;
    #else
        std::cout << "Release mode" << std::endl;
    #endif
    return 0;
}
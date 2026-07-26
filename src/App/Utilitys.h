#pragma once
#include <unordered_map>
#include <string>
#include <iostream>
template<typename T>
void registerObject(const std::string& id, T* object, std::unordered_map<std::string, T*>& objectsMap) {
    if (objectsMap.count(id) == 1) {
        std::cout << "Registering function with id \"" << id << "\" is already registered." << std::endl;
        return;
    }
#ifdef _DEBUG
    std::cout << "Making register with id: " << id << std::endl;
#endif
    objectsMap.emplace(id, object);
}
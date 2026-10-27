#pragma once
#include <unordered_map>
#include <string>
#include <iostream>
#include "Logger.h"

template<typename T>
static T* registerObject(const std::string& id, T* object, std::unordered_map<std::string, T*>& objectsMap) {
    if (objectsMap.count(id) == 1) {
        stdout_logger.log("Registering function with id \"", id, "\" is already registered.");
        return object;
    }
#ifdef _DEBUG
    stdout_logger.log("Making register with id: ", id);
#endif
    objectsMap.emplace(id, object);
    return object;
}

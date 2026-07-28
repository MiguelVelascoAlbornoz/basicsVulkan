//
// Created by migue on 28/07/2026.
//

#ifndef BASICSVULKAN_REGISTRY_H
#define BASICSVULKAN_REGISTRY_H

#include "../App/App.h"


class Registry
{
public:
    static void initUniforms(const App* app);
    static void initPipelines(const App* app);
    static void initMeshes(const App* app);
    static void initMenus(const App* app);

    static void registryCallback(const App* app);
};



#endif //BASICSVULKAN_REGISTRY_H

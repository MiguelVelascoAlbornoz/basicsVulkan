//
// Created by migue on 28/07/2026.
//

#ifndef BASICSVULKAN_REGISTRY_H
#define BASICSVULKAN_REGISTRY_H


class App;


class Registry
{
public:
    static void initUniforms(const App* app);
    static void initPipelines(const App* app);
    static void initMeshes(const App* app);
    static void initMenus( App * app);
    static void initFramebuffers(const App* app);
    static void initImages(const App* app);
    static void initComputePipelines(const App* app);
    static void initFonts();
    static void registryCallback( App* app);
};



#endif //BASICSVULKAN_REGISTRY_H

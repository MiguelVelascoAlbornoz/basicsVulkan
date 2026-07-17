/**
 * @file App.h
 * @brief App class declaration and all his features.
 * @author Miguel Velasco
 */
#ifndef APP_H
#define APP_H

#include "../renderer/Renderer.h"
#include "../Menu/MenuManager.h"
#include "../renderer/UniformBuffer.h"
/**
 * @brief The App class serves as the main application class that manages the window, renderer, and the main execution loop. It initializes the window and renderer, handles events, and updates the display in a continuous loop until the application is closed.
 * The App class is responsible for creating the main window and renderer, managing events such as user
 */
class App {
public:
    /** 
     * @brief Constructor of the App class
     * @details Initializes the window and starts the execution loop. If the window fails to initialize, it prints an error message and exits the program with a failure code.
    */
    void startMenu();
    void initMenus();
    App();
    Window* window = nullptr; /**< window pointer, must be deleted must be deleted and end*/
    Renderer* renderer = nullptr; /**< renderer pointer, must be deleted at end */
    MenuManager* menuManager= nullptr; /**< menu manager pointer, must be deleted at end */
    Scene* scene = nullptr; /**< scene pointer, must be deleted at end */
    UniformBuffer* uniformBuffer = nullptr; /**< uniform buffer pointer, must be deleted at end */
    ~App();

    Uint64 currentTime = 0; /**< Current time in seconds, used for animations and time-based updates. */
    Uint64 lastTickTime = 0; /**< Time of the last frame in seconds.*/
    Uint64 deltaTime = 0; /**< Time difference between the current frame and the last frame in seconds, used for time-based updates. */
private:
    /**
     * @brief Main execution loop of the application.
     */
    void executionLoop();
    /** */
    void manageEvents();
    bool runnig = true; /**< Estado do loop */
};

#endif
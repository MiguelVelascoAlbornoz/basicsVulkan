/**
 * @file App.h
 * @brief App class declaration and all his features.
 * @author Miguel Velasco
 */
#ifndef APP_H
#define APP_H

#include "../renderer/Renderer.h"
#include "../Menu/MenuManager.h"
/**
 * @brief The App class serves as the main application class that manages the window, renderer, and the main execution loop. It initializes the window and renderer, handles events, and updates the display in a continuous loop until the application is closed.
 * The App class is responsible for creating the main window and renderer, managing events such as user
 */
class App {
public:
    /** 
     * @brief Constructor of the App classs
     * @details Initializes the window and starts the execution loop. If the window fails to initialize, it prints an error message and exits the program with a failure code.
    */
    void startMenu();
    void initMenus();
    App();
    Window* window = NULL; /**< window pointer, must be deleted must be deleted and end*/
    Renderer* renderer = NULL; /**< renderer pointer, must be deleted at end */
    MenuManager* menuManager= NULL; /**< menu manager pointer, must be deleted at end */
    Scene* scene = NULL; /**< scene pointer, must be deleted at end */
    ~App() {
        delete window;
        if (renderer) {
            delete renderer;
        }
       
        if (menuManager) {
            delete menuManager;
        }
        if (scene) {
            delete scene;
        }
    }
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
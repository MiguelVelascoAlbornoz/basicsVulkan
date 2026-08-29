/**
 * @file App.h
 * @brief App class declaration and all his features.
 * @author Miguel Velasco
 */
#ifndef APP_H
#define APP_H



#include  "../Registry/Uniforms.h"
#include "../Registry/Pipelines.h"
#include "../Registry/Menus.h"
#include "../Registry/Meshes.h"
#include "../Registry/FrameBuffers.h"

#include "../Scene/Player.h"
#include <SDL3/SDL.h>



class DesktopDuplicatorManager;
class Window;
class Renderer;

/**
 * @brief The App class serves as the main application class that manages the window, renderer, and the main execution loop. It initializes the window and renderer, handles events, and updates the display in a continuous loop until the application is closed.
 * The App class is responsible for creating the main window and renderer, managing events such as user
 */
class App {
public:

    Window* window = nullptr; /**< window pointer, must be deleted must be deleted and end*/
    Renderer* renderer = nullptr; /**< renderer pointer, must be deleted at end */


    App(const std::function<void(App*)>& registryCallback);
    ~App();
    Player* player = nullptr;
    bool editorMode = false;
    bool F3Mode = false;
    Uint64 cycleStartTimeNS = 0; /**< Exact time of start of the current cycle **/
    Uint64 cycleDeltaTimeNS = 0; /**< Delay in nano seconds of a entire cycle of execution. From start to start of the while. */

    Uint64 tickDeltaTimeNS = 0; /**< Delay between the start and the start of the past tick. **/
    Uint64 tickStartTimeNS = 0; /**< Exact time of start of the current tick **/
    SDL_MouseMotionEvent mouseMotion;
    Uint64 cyclesPerSecond = 0;
    Uint64 ticksPerSecond = 0;
    bool movedMouse;
    void onFBOResolutionChange() const;

    DesktopDuplicatorManager* desktopDuplicatorManager;

private:
    /**
     * @brief Main execution loop of the application.
     */
    void executionLoop();

    void renderGUI();

    /** */
    void manageEvents();

    void managePlayerMovement();



    bool runnig = true; /**< Estado do loop */
};




#endif


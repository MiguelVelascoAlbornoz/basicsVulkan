/**
 * @file Window.h
 * @brief Window class declaration and all his features.
 * @author Miguel Velasco
 */
#ifndef WINDOW_H
#define WINDOW_H

#ifndef PROJECT_NAME /** @brief .exe file and window name*/
#define PROJECT_NAME "Unknown"
#endif
#ifndef PROJECT_VERSION /** @brief Version of the project*/
#define PROJECT_VERSION "0.0.0"
#endif
#include <SDL3/SDL.h>
#include "Vulkan/vulkan.h"


#define DEFAULT_REDUCTION_FACTOR 1.5 /**< @brief Default reduction factor for window size when not in fullscreen mode. */
/**
 * @brief The Window class encapsulates the creation and management of an SDL window, including event handling and error management.
 * It provides methods to initialize the window, manage events, and check for initialization errors. 
 */
class Window {
public:

    Window();
    /**
     * @brief Constructor of the Window class.
     * @details Initializes SDL, creates the window . If any of these steps fail, it sets the error flag to true. isError() can be used to check if the initialization was successful.
     * @param width The width of the window to be created.
     * @param height The height of the window to be created.
     */
    Window(int width, int height);
    /**
     * @brief Enumeration of possible event types that the Window class can handle.
     */
    enum class EventType {
        QUIT,
    };
    int getWidth() const { return width; }   /**< @brief Get the width of the window. */
    int getHeight() const { return height; } /**< @brief Get the height of the window. */
    int getEventState(EventType event) const { return flags[static_cast<int>(event)]; } /**< @brief Get the state of a specific event. */
    SDL_Window* window; /**< @brief SDL window pointer. */
    VkSurfaceKHR surface; /**< @brief Vulkan surface pointer, used for rendering with Vulkan. */
    void toggleFullscreen();
private:
    void inicializeSDL(); /**< @brief Initializes SDL and sets the application metadata. */
    void createWindow(int width, int height); /**< @brief Creates the SDL window with the specified width and height. */
    bool fullscreen = false; /**< @brief Flag to indicate if the window is in fullscreen mode. */
    unsigned int extensionCount; /**< @brief Count of Vulkan instance extensions required by SDL. */
    const char* const* extensions; /**< @brief Array of Vulkan instance extensions required by SDL. */
    int setState(EventType event, bool state) { return flags[static_cast<int>(event)] = state; } /**< @brief Set the state of a specific event. */
    bool flags[256]; /**< @brief Array to store the state of different events. */

    int error = false; /**< @brief Flag to indicate if there was an error during initialization. */
    int width,height; /**< @brief Window dimensions. */
    public:

    /**
     * @brief Destructor of the Window class.
     * @details Destroys the SDL window and quits SDL to clean up resources when the Window object is destroyed.
     */
    ~Window();
    /**
     * @brief Manages SDL events for the window.
     * @details Polls for SDL events and updates the event state flags accordingly.
     */
    void manageEvents();
    bool isError() const { return error; } /**< @brief Check if there was an error during initialization. */  

};

#endif  
/**
 * @file Window.h
 * @brief Window class declaration and all his features.
 * @author Miguel Velasco
 */
#ifndef WINDOW_H
#define WINDOW_H

#define SDL3_STATIC

#ifndef PROJECT_NAME /** @brief .exe file and window name*/
#define PROJECT_NAME "Unknown"
#endif
#ifndef PROJECT_VERSION /** @brief Version of the project*/
#define PROJECT_VERSION "0.0.0"
#endif

#include <stdio.h>
#include <iostream>
#include <SDL3/SDL.h>
class Window {
     
private:
    SDL_Window* window; /**< @brief SDL window pointer. */
    SDL_Renderer* renderer; /**< @brief SDL renderer pointer. */
    int error = false; /**< @brief Flag to indicate if there was an error during initialization. */

    public:
    /**
     * @brief Constructor of the Window class.
     * @details Initializes SDL, creates the window and renderer, and sets the logical presentation. If any of these steps fail, it sets the error flag to true.
     * Error flag must be checked with isError() method after creating an instance of the Window.
     */
    Window();
    ~Window();
    bool isError() const { return error; } /**< @brief Check if there was an error during initialization. */    
};

#endif
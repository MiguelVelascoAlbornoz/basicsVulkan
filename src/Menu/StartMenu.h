#ifndef START_MENU_H
#define START_MENU_H
/**
 * @file StartMenu.h
 * @brief StartMenu class declaration and all his features.
 * @author Miguel Velasco
 */
#include "Menu.h"
#include "../renderer/Window.h"

class StartMenu : public Menu {
    public:
        StartMenu(Window* window) : Menu("start_menu"),window(window) {} /**< @brief Constructor for the StartMenu class, initializes the menu with a unique identifier. */;
        ~StartMenu() {} /**< @brief Destructor for the StartMenu class. */
        void render() override; /**< @brief Function to render the start menu. */
    private:
        Window* window; /**< @brief Pointer to the window, used for file dialog operations. */
};
#endif
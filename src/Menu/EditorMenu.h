#ifndef START_MENU_H
#define START_MENU_H
/**
 * @file StartMenu.h
 * @brief StartMenu class declaration and all his features.
 * @author Miguel Velasco
 */
#include "Menus.h"
#include "../Scene/Player.h"



class EditorMenu : public Menu {
    public:
    explicit EditorMenu(Player* player) : player(player) {} /**< @brief Constructor for the StartMenu class, initializes the menu with a unique identifier. */;
        ~EditorMenu() override = default; /**< @brief Destructor for the StartMenu class. */
        void render() override; /**< @brief Function to render the start menu. */
    private:
        Player* player; /**< @brief Pointer to the window, used for file dialog operations. */
};
#endif
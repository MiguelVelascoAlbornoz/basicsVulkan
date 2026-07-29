#ifndef START_MENU_H
#define START_MENU_H
/**
 * @file StartMenu.h
 * @brief StartMenu class declaration and all his features.
 * @author Miguel Velasco
 */

#include <functional>
#include "Menu.h"
class Player;



class EditorMenu : public Menu {
    public:
    explicit EditorMenu(Player* player, std::function<void()> onPlayerRenderUpdateCallback) : player(player),onPlayerRenderUpdateCallback(std::move(onPlayerRenderUpdateCallback))  {} /**< @brief Constructor for the StartMenu class, initializes the menu with a unique identifier. */;
        ~EditorMenu() override = default; /**< @brief Destructor for the StartMenu class. */
        void render() override; /**< @brief Function to render the start menu. */
    private:
        Player* player; /**< @brief Pointer to the window, used for file dialog operations. */
        std::function<void()> onPlayerRenderUpdateCallback;
};
#endif
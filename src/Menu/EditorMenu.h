#ifndef START_MENU_H
#define START_MENU_H
/**
 * @file StartMenu.h
 * @brief StartMenu class declaration and all his features.
 * @author Miguel Velasco
 */

#include <functional>
#include <utility>
#include "Menu.h"
class Player;
class App;


class EditorMenu : public Menu {
    public:
    explicit EditorMenu(const App* app ) : app(app)  {} /**< @brief Constructor for the StartMenu class, initializes the menu with a unique identifier. */;
        ~EditorMenu() override = default; /**< @brief Destructor for the StartMenu class. */
        void render() override; /**< @brief Function to render the start menu. */
    private:
        const App* app;
};
#endif
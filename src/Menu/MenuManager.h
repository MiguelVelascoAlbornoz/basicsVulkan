/**
 * @file MenuManager.h
 * @brief MenuManager class declaration and all his features.
 * @author Miguel Velasco
 */
#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

#include <unordered_map>
#include <functional>
#include "StartMenu.h"

/**
 * @brief The MenuManager class is responsible for managing different menus in the application. It allows for registering menus and retrieving the current menu to be rendered. The MenuManager maintains a mapping of menu identifiers to their corresponding menu instances, enabling dynamic menu management within the application.
 */
class MenuManager {
public:
    StartMenu* startMenu; /**< @brief Start menu instance. */

    MenuManager(Window *window);
    
    void registerMenu(Menu *menu); /**< @brief Registers a menu rendering function for a specific menu type. */

    
    Menu* currentMenu; /**< @brief Retrieves the rendering function for the current menu type. */
        ~MenuManager() {
        for (auto& pair : menus) {
            delete pair.second;
        }
    }
    
    private:

    std::unordered_map<std::string, Menu*> menus; /**< @brief Map to store menu rendering functions. */
};

#endif
/**
 * @file MenusManager.cpp
 * @brief MenuManager class implementation.
 * @author Miguel Velasco
 */
#include "MenuManager.h"
#include <iostream>

MenuManager::MenuManager(Window* window)
{
    StartMenu* startMenu = new StartMenu(window);
    registerMenu(startMenu);
    currentMenu = startMenu;
}

void MenuManager::registerMenu(Menu *menu)
{
    if (menus.count(menu->getId())==1){
        std::cout << "Menu with id " << menu->getId() << " already exists." << std::endl;
        return;
    }
    #ifdef _DEBUG
    std::cout << "Registering menu with id: " << menu->getId() << std::endl;
    #endif
    menus[menu->getId()] = menu;
}
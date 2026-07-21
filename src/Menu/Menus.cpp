/**
 * @file MenusManager.cpp
 * @brief MenuManager class implementation.
 * @author Miguel Velasco
 */
#include "Menus.h"
#include <iostream>

std::unordered_map<std::string,Menu*> Menus::openMenus;
std::unordered_map<std::string, Menu*> Menus::menus; /**< @brief Map to store menu rendering functions. */

void Menus::registerMenu(const std::string& menuID, Menu *menu)
{
    if (menus.count(menuID)==1){
        std::cout << "Menu with id " << menuID << " already exists." << std::endl;
        return;
    }
    #ifdef _DEBUG
    std::cout << "Registering menu with id: " << menuID << std::endl;
    #endif


    Menus::menus.emplace(menuID, menu);
}

void Menus::openMenu(const std::string& menuID) {
    Menu* menuToOpen = Menus::menus[menuID];
    if (menuToOpen) {
        Menus::openMenus.emplace(menuID,menuToOpen);
    } else {
        std::cerr << "Error in openMenu(): Menu with name \"" << menuID << "\" does not exist." << std::endl;
        Menus::menus.erase(menuID);
    }

}
void Menus::closeMenu(const std::string& menuID) {
    Menu* menuToClose = Menus::openMenus[menuID];
    if (menuToClose) {
        Menus::openMenus.erase(menuID);
    } else {
        std::cerr << "Error in closeMenu(): Menu with id \"" << menuID << "\" does not exist or is not open." << std::endl;
        Menus::openMenus.erase(menuID);
    }
}
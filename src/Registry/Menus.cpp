/**
 * @file MenusManager.cpp
 * @brief MenuManager class implementation.
 * @author Miguel Velasco
 */
#include "Menus.h"
#include "../App/Utilitys.h"


std::unordered_map<std::string,Menu*> Menus::openMenus;
std::unordered_map<std::string, Menu*> Menus::menus; /**< @brief Map to store menu rendering functions. */

ChooseAppTypeMenu* Menus::chooseMenu;
EditorMenu* Menus::editorMenu;
F3GUI* Menus::F3Menu;
ListeningMenu* Menus::listeningMenu;

Menu* Menus::registerMenu(const std::string& menuID, Menu *menu)
{
    return registerObject(menuID,menu,menus);
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
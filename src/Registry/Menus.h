/**
 * @file MenuManager.h
 * @brief MenuManager class declaration and all his features.
 * @author Miguel Velasco
 */
#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H


#include "../Menu/Menu.h"
#include <unordered_map>
class EditorMenu;
class ChooseAppTypeMenu;
/**
 * @brief Static class que contiene todo lo necesario para gestionarlos menus y guardarlos.
 * Para usar un menu se registra con un dado ID y un pointer a un menu, despues para que el menu sea renderizado al llamar drawMenus() deve usarser openMenu() y poner el id del menu. Para que no se renderize mas es lo mismo pero con close menu.
 * Al final deve usarse free menus para liberar la memoria de cada menu y de los hash tables
 */

#define EDITOR_MENU_ID "editor_menu"
#define F3_MENU_ID "f3_menu"
#define CHOOSE_APP_TYPE_MENU_ID "choose_app_type_menu"
#define LISTENING_MENU_ID "listening_menu"
class F3GUI;
class ListeningMenu;
class Menus {
public:
    static EditorMenu* editorMenu;
    static F3GUI* F3Menu;
    static ChooseAppTypeMenu* chooseMenu;
    static ListeningMenu* listeningMenu;

    static std::unordered_map<std::string,Menu*> openMenus;
    static std::unordered_map<std::string, Menu*> menus; /**< @brief Map to store menu rendering functions. */


    static Menu* registerMenu(const std::string& menuID, Menu *menu); /**< @brief Registers a menu rendering function for a specific menu type. */
    static void freeMenus() {
        for (auto& [name, menu] : menus) {
            delete menu;
        }

        menus.clear();
        openMenus.clear();
    }

    static void openMenu(const std::string& menuID);
    static void closeMenu(const std::string& menuID);
    static void drawMenus() {
        for (auto& menuPair : openMenus) {
            menuPair.second->render();

        }
        for (auto it = openMenus.begin(); it != openMenus.end();) {
            if (it->second->shouldClose) {
                it = openMenus.erase(it);
            } else {
                ++it;
            }
        }

    }

};

#endif
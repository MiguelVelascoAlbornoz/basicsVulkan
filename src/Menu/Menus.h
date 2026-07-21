/**
 * @file MenuManager.h
 * @brief MenuManager class declaration and all his features.
 * @author Miguel Velasco
 */
#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

#include <unordered_map>
#include <functional>

#include <string>
/**
 * @brief The Menu class serves as a base class for different types of menus in the application. It provides a common interface for rendering menus and managing their unique identifiers. Each menu type can inherit from this base class and implement its own rendering logic while maintaining a consistent structure for menu management within the application.
 */
class Menu {
public:
    virtual void render() = 0; /**< @brief Function to render the menu. */
    virtual ~Menu() = default; /**< @brief Virtual destructor for the Menu class. */
    void openMenu(const std::string& menuID);

};
/**
 * @brief Static class que contiene todo lo necesario para gestionarlos menus y guardarlos.
 * Para usar un menu se registra con un dado ID y un pointer a un menu, despues para que el menu sea renderizado al llamar drawMenus() deve usarser openMenu() y poner el id del menu. Para que no se renderize mas es lo mismo pero con close menu.
 * Al final deve usarse free menus para liberar la memoria de cada menu y de los hash tables
 */
class Menus {
public:



    static void registerMenu(const std::string& menuID, Menu *menu); /**< @brief Registers a menu rendering function for a specific menu type. */

    static void openMenu(const std::string& menuID);
    static void closeMenu(const std::string& menuID);

    static void freeMenus() {
        for (auto& [name, menu] : menus) {
            delete menu;
        }

        menus.clear();
        openMenus.clear();
    }
    static void drawMenus() {
        for (auto& menuPair : openMenus) {
            menuPair.second->render();
        }
    }
    static std::unordered_map<std::string,Menu*> openMenus;
    static std::unordered_map<std::string, Menu*> menus; /**< @brief Map to store menu rendering functions. */
};

#endif
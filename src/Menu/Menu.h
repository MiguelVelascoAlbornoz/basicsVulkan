//
// Created by migue on 28/07/2026.
//

#ifndef BASICSVULKAN_MENU_H
#define BASICSVULKAN_MENU_H
#include <string>


/**
 * @brief The Menu class serves as a base class for different types of menus in the application. It provides a common interface for rendering menus and managing their unique identifiers. Each menu type can inherit from this base class and implement its own rendering logic while maintaining a consistent structure for menu management within the application.
 */
class Menu {
public:
    virtual void render() = 0; /**< @brief Function to render the menu. */
    virtual ~Menu() = default; /**< @brief Virtual destructor for the Menu class. */
    void openMenu(const std::string& menuID);
    bool shouldClose = false;
};

#endif //BASICSVULKAN_MENU_H

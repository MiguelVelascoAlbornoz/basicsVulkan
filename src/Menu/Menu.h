#ifndef MENU_H
#define MENU_H
/**
 * @file Menu.h
 * @brief Menu class declaration and all his features.
 * @author Miguel Velasco
 */
#include <string>
/**
 * @brief The Menu class serves as a base class for different types of menus in the application. It provides a common interface for rendering menus and managing their unique identifiers. Each menu type can inherit from this base class and implement its own rendering logic while maintaining a consistent structure for menu management within the application.
 */
class Menu {
    public:
        Menu(const char* id) : id(id) {} /**< @brief Constructor for the Menu class, initializes the menu with a unique identifier. */;
        virtual void render() = 0; /**< @brief Function to render the menu. */
        std::string getId() const { return id; } /**< @brief Retrieves the unique identifier of the menu. */
        virtual ~Menu() {} /**< @brief Virtual destructor for the Menu class. */
    protected:
        std::string id; /**< @brief Unique identifier for the menu. */

};
#endif
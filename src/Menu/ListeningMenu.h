//
// Created by migue on 30/08/2026.
//

#ifndef BASICSVULKAN_LISTENINGMENU_H
#define BASICSVULKAN_LISTENINGMENU_H

#include "Menu.h"

class App;

class ListeningMenu : public Menu
{
public:
    explicit ListeningMenu( App* app) : app(app)
    {
    };
    void render() override;

private:
    App* app; /**< @brief Pointer to the window, used for file dialog operations. */
};


#endif //BASICSVULKAN_LISTENINGMENU_H

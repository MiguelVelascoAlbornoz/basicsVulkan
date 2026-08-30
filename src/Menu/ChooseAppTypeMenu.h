//
// Created by migue on 29/08/2026.
//

#ifndef BASICSVULKAN_CHOOSEAPPTYPEMENU_H
#define BASICSVULKAN_CHOOSEAPPTYPEMENU_H

#include "Menu.h"

class App;

class ChooseAppTypeMenu : public Menu
{

    public:
    explicit ChooseAppTypeMenu( App* app) : app(app)
    {
    };
    void render() override;

private:
     App* app; /**< @brief Pointer to the window, used for file dialog operations. */
};



#endif //BASICSVULKAN_CHOOSEAPPTYPEMENU_H

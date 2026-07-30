//
// Created by migue on 29/07/2026.
//

#ifndef BASICSVULKAN_F3GUI_H
#define BASICSVULKAN_F3GUI_H

#include "Menu.h"


class App;

class F3GUI : public Menu
{
public:
    explicit F3GUI(const App* app) : app(app)
    {
    };
    void render() override;

private:
    const App* app; /**< @brief Pointer to the window, used for file dialog operations. */
};


#endif //BASICSVULKAN_F3GUI_H

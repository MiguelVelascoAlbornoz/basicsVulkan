//
// Created by migue on 02/09/2026.
//

#ifndef BASICSVULKAN_SENDMESSAGEMENU_H
#define BASICSVULKAN_SENDMESSAGEMENU_H

#include "Menu.h"

class App;

class SendMessageMenu : public Menu
{
public:
    explicit SendMessageMenu(App* app) : app(app)
    {
    };
    void render() override;

private:
    App* app; /**< @brief Pointer to the app, usado para acceder al netManager y enviar el mensaje. */
    std::string messageBuffer;
};

#endif //BASICSVULKAN_SENDMESSAGEMENU_H
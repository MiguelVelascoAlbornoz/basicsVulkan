#ifndef BASICSVULKAN_CONNECTMENU_H
#define BASICSVULKAN_CONNECTMENU_H

#include "Menu.h"
#include <string>

class App;

class ConnectMenu : public Menu
{
public:
        explicit ConnectMenu(App* app) : app(app)
        {
        };
        void render() override;

        /** @brief Llamar esto cuando se sepa el resultado real de la conexión
         *  (desde donde sea que termine confirmándose éxito/fallo). Mientras
         *  nadie la llame después de intentar conectar, el menú se queda en
         *  el estado "conectando" con la animación de puntos. */
        void setConnectionResult(bool success, const std::string& message);

private:
        App* app;

        enum class State {
                FORM,
                CONNECTING,
                RESULT
            };

        State state = State::FORM;
        bool lastAttemptSuccess = false;
        std::string resultMessage;

        char ipBuffer[64] = "127.0.0.1";
        char passwordBuffer[128] = "";
        int port = 9000;
};

#endif //BASICSVULKAN_CONNECTMENU_H
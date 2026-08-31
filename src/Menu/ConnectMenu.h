//
// Created by migue on 31/08/2026.
//

#ifndef BASICSVULKAN_CONNECTMENU_H
#define BASICSVULKAN_CONNECTMENU_H

#include "Menu.h"
class App;

class ConnectMenu : public Menu
{


        public:
        explicit ConnectMenu( App* app) : app(app)
        {
        };
        void render() override;

        private:
        App* app; /**< @brief Pointer to the window, used for file dialog operations. */


};



#endif //BASICSVULKAN_CONNECTMENU_H

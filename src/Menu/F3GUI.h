//
// Created by migue on 29/07/2026.
//

#ifndef BASICSVULKAN_F3GUI_H
#define BASICSVULKAN_F3GUI_H

#include "Menu.h"

class Player;

class F3GUI : public Menu
{
public:
    explicit F3GUI(Player* player) : player(player) {};
    void render() override;

private:
    Player* player; /**< @brief Pointer to the window, used for file dialog operations. */
};


#endif //BASICSVULKAN_F3GUI_H

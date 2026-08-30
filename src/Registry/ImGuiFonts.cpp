//
// Created by migue on 29/08/2026.
//

#include "ImGuiFonts.h"
std::unordered_map<std::string, ImFont**> ImGuiFonts::fonts; /**< @brief Map to store menu rendering functions. */
void ImGuiFonts::freeFonts()
{
    for (const auto& font : fonts) {
        delete font.second;
    }

}

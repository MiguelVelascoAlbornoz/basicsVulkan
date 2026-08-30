//
// Created by migue on 29/08/2026.
//

#ifndef BASICSVULKAN_IMGUIFONTS_H
#define BASICSVULKAN_IMGUIFONTS_H


#include "../App/Utilitys.h"
#include "imGUI/imgui.h"

#define WINDOWS_FONT_ID "windows_font"
#define WINDOWS_FONT_BOLD_ID "windows_font_bold"

struct ImFont;

class ImGuiFonts
{
    public:
    static std::unordered_map<std::string, ImFont**> fonts; /**< @brief Map to store menu rendering functions. */

    static ImFont** registerFont(const std::string& id, const std::string& fontPath,
                                 const ImFontConfig* font_cfg = nullptr,
                                 const ImWchar* glyph_ranges = nullptr) {
        auto** fontSizes = new ImFont*[5];
        fontSizes[0] = ImGui::GetIO().Fonts->AddFontFromFileTTF((fontPath).c_str(),10,font_cfg,glyph_ranges);
        fontSizes[1] = ImGui::GetIO().Fonts->AddFontFromFileTTF((fontPath).c_str(),18,font_cfg,glyph_ranges);
        fontSizes[2] = ImGui::GetIO().Fonts->AddFontFromFileTTF((fontPath).c_str(),25,font_cfg,glyph_ranges);
        fontSizes[3] = ImGui::GetIO().Fonts->AddFontFromFileTTF((fontPath).c_str(),32,font_cfg,glyph_ranges);
        fontSizes[4] = ImGui::GetIO().Fonts->AddFontFromFileTTF((fontPath).c_str(),45,font_cfg,glyph_ranges);

        return registerObject(id, fontSizes, fonts);
    };


    static void freeFonts();
};


#endif //BASICSVULKAN_IMGUIFONTS_H

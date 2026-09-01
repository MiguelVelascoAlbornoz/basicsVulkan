#include "imGUI/imgui.h"
#include "MenuUtils.h"


bool ModernButton(const char* label, const char* icon, const ImVec2 size)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 10));

    bool clicked = ImGui::Button(
        (std::string(icon) + "  " + label).c_str(),
        size
    );

    ImGui::PopStyleVar(2);

    return clicked;
}


bool DrawButtonWithShadow(const char* label, const ImVec2 pos, const ImVec2 size, const bool primary) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 absPos = ImVec2(ImGui::GetWindowPos().x + pos.x, ImGui::GetWindowPos().y + pos.y);

    // Sombra
    dl->AddRectFilled(
        ImVec2(absPos.x + 2, absPos.y + 3),
        ImVec2(absPos.x + size.x + 2, absPos.y + size.y + 3),
        IM_COL32(0, 0, 0, 25), 3.0f
    );

    ImGui::SetCursorPos(pos);
    if (primary) {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(41, 115, 217, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(56, 133, 235, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(31, 95, 190, 255));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(230, 230, 232, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(225, 225, 228, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(210, 210, 214, 255));
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(30, 30, 30, 255));
    }

    const bool ret = ImGui::Button(label, size);

    ImGui::PopStyleColor(primary ? 3 : 4);
    return ret;
}

std::string GetLoadingDots()
{
    // Ciclo: . -> .. -> ... -> repite, cada 400ms
    int step = static_cast<int>(ImGui::GetTime() / 0.4) % 3;
    return std::string(step + 1, '.');
}
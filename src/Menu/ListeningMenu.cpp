//
// Created by migue on 30/08/2026.
//

#include "ListeningMenu.h"

#include <iostream>

#include "../App/App.h"
#include "../Registry/ImGuiFonts.h"
#include "imGUI/imgui.h"


void ListeningMenu::render()
{
    const ImGuiWindowFlags flags =
    ImGuiWindowFlags_NoTitleBar |
    ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_NoBringToFrontOnFocus|
ImGuiWindowFlags_NoScrollbar |                      // <-- recomendado
ImGuiWindowFlags_NoScrollWithMouse;

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::Begin("Listening Menu", nullptr, flags);


    // Footer sutil
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();
    float footerY = winSize.y - 30;
    drawList->AddLine(
        ImVec2(winPos.x, winPos.y + footerY),
        ImVec2(winPos.x + winSize.x, winPos.y + footerY),
        IM_COL32(200, 200, 200, 255), 1.0f
    );
    ImGui::PushFont(ImGuiFonts::fonts[WINDOWS_FONT_ID][1]);
    ImGui::SetCursorPos(ImVec2(24, footerY + 6));
    ImGui::TextColored(ImVec4(0.5f,0.5f,0.5f,1), PROJECT_VERSION);
    ImGui::PopFont();
    // Franja superior de marca
    float headerHeight = 80.0f;
    drawList->AddLine(
        ImVec2(winPos.x, winPos.y + headerHeight),
        ImVec2(winPos.x + winSize.x, winPos.y + headerHeight),
        IM_COL32(200, 200, 200, 255), 1.0f
    );
    ImGui::PushFont(ImGuiFonts::fonts[WINDOWS_FONT_BOLD_ID][3]); // fuente más grande para el título
    ImGui::SetCursorPos(ImVec2(24, 24));
    ImGui::TextColored(ImVec4(.1,.1,.1,1), ("Listening port " + std::to_string(app->netManager->getHostPort())).c_str());
    ImGui::PopFont();




    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = style.FrameRounding;
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]      = ImVec4(0.9f, 0.9f, 0.9f, 1.00f); // gris casi blanco, no blanco puro
    colors[ImGuiCol_Text]          = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
    colors[ImGuiCol_Border]        = ImVec4(0.80f, 0.80f, 0.82f, 1.00f);

    // Botón primario (acción principal, ej. "Start Server")
    colors[ImGuiCol_Button]        = ImVec4(0.16f, 0.45f, 0.85f, 1.00f); // azul saturado, no pastel
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.52f, 0.92f, 1.00f);
    colors[ImGuiCol_ButtonActive]  = ImVec4(0.12f, 0.38f, 0.75f, 1.00f);
    colors[ImGuiCol_Text]          = ImVec4(0.10f, 0.10f, .10f, 1.00f);

    ImGui::SetCursorPos(ImVec2(48, headerHeight + 20));
    ImGui::Columns(3, nullptr, false);
    ImGui::PushFont(ImGuiFonts::fonts[WINDOWS_FONT_ID][3]);
    ImGui::TextUnformatted("PRIVATE IPS");
    ImGui::NextColumn();
    ImGui::TextUnformatted("PUBLIC IP");
    ImGui::NextColumn();
    ImGui::TextUnformatted("STATUS");
    ImGui::PopFont();
    ImGui::NextColumn();
    ImGui::Dummy(ImVec2(0, 3));

    ImGui::PushStyleColor(ImGuiCol_Separator, IM_COL32(210, 210, 210, 255));
    ImGui::Separator();
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 8));

    ImGui::PushFont(ImGuiFonts::fonts[WINDOWS_FONT_ID][2]);

    // Guardamos la Y donde empiezan las IPs
    float ipsY = ImGui::GetCursorPosY();

    // Private IPs
    ImGui::Indent(8.0f);
    std::vector<std::string> privateIPs = app->netManager->getPrivateIPs();
    for (const auto& ip : privateIPs)
    {
        ImGui::TextUnformatted(ip.c_str());
    }

    ImGui::Unindent(8.0f);

    // Volvemos a la primera línea
    ImGui::SetCursorPosY(ipsY);

    // Cambiamos a la columna derecha
    ImGui::NextColumn();

    ImGui::TextUnformatted(app->netManager->getPublicIP().c_str());

    ImGui::NextColumn();

    switch (app->netManager->getStatus())
    {

    case NetManager::WAITING:
        ImGui::TextColored(ImVec4(.7f,.7f,0.0f,1.0f),"Waiting");
        break;
    case NetManager::CONNECTED:
        ImGui::TextColored(ImVec4(.1f,.9f,0.2f,1.0f),("Connected to " + app->netManager->getConnectionIP()).c_str());
        break;
    default:
        ImGui::TextColored(ImVec4(.0f,.0f,0.0f,1.0f),"Undefined");
        break;
    }

    ImGui::PopFont();

    ImGui::Columns(1);

    ImGui::End();
}

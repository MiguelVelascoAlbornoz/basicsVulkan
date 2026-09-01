//
// Created by migue on 31/08/2026.
//

#include "ConnectMenu.h"

#include <iostream>

#include "../Registry/ImGuiFonts.h"
#include "MenuUtils.h"
#include "../App/App.h"
#include "imGUI/imgui.h"

inline void ConnectMenu::render()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::Begin("Connect Menu", nullptr, flags);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();

    // ---- Paleta de colores, igual que en los otros menus ----
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = style.FrameRounding;
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]      = ImVec4(0.9f, 0.9f, 0.9f, 1.00f);
    colors[ImGuiCol_Text]          = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
    colors[ImGuiCol_Border]        = ImVec4(0.80f, 0.80f, 0.82f, 1.00f);
    colors[ImGuiCol_Button]        = ImVec4(0.16f, 0.45f, 0.85f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.52f, 0.92f, 1.00f);
    colors[ImGuiCol_ButtonActive]  = ImVec4(0.12f, 0.38f, 0.75f, 1.00f);
    colors[ImGuiCol_FrameBg]       = ImVec4(1.0f, 1.0f, 1.0f, 1.00f);
    colors[ImGuiCol_Text]          = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_FrameBg]         = ImVec4(1.0f, 1.0f, 1.0f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]  = ImVec4(0.95f, 0.97f, 1.00f, 1.00f); // se ilumina levemente al pasar el mouse
    colors[ImGuiCol_FrameBgActive]   = ImVec4(0.90f, 0.95f, 1.00f, 1.00f); // se ilumina más al estar escribiendo
    colors[ImGuiCol_Border]          = ImVec4(0.70f, 0.70f, 0.72f, 1.00f);
    style.FrameBorderSize            = 1.0f; // sin esto el borde no se dibuja aunque tenga color
    // ---- Franja superior de marca ----
    float headerHeight = 80.0f;
    drawList->AddLine(
        ImVec2(winPos.x, winPos.y + headerHeight),
        ImVec2(winPos.x + winSize.x, winPos.y + headerHeight),
        IM_COL32(200, 200, 200, 255), 1.0f
    );
    ImGui::PushFont(ImGuiFonts::fonts[WINDOWS_FONT_BOLD_ID][2]);
    ImGui::SetCursorPos(ImVec2(24, 24));
    ImGui::TextColored(ImVec4(.1f, .1f, .1f, 1), "Connect to Server");
    ImGui::PopFont();

    // ---- Footer sutil ----
    float footerY = winSize.y - 30;
    drawList->AddLine(
        ImVec2(winPos.x, winPos.y + footerY),
        ImVec2(winPos.x + winSize.x, winPos.y + footerY),
        IM_COL32(200, 200, 200, 255), 1.0f
    );
    ImGui::PushFont(ImGuiFonts::fonts[WINDOWS_FONT_ID][1]);
    ImGui::SetCursorPos(ImVec2(24, footerY + 6));
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), PROJECT_VERSION);
    ImGui::PopFont();

    // ---- Formulario centrado ----
    float fieldWidth = 320.0f;
    float centerX = viewport->WorkSize.x / 2 - fieldWidth / 2;
    float y = headerHeight + 60.0f;
    float rowSpacing = 55.0f;

    ImGui::PushFont(ImGuiFonts::fonts[WINDOWS_FONT_ID][2]);

    // IP
    ImGui::SetCursorPos(ImVec2(centerX, y));
    ImGui::TextUnformatted("IP");
    ImGui::SetCursorPos(ImVec2(centerX, y + 32));
    ImGui::SetNextItemWidth(fieldWidth);
    ImGui::InputText("##ip", ipBuffer, IM_ARRAYSIZE(ipBuffer));

    // Puerto
    y += rowSpacing + 22;
    ImGui::SetCursorPos(ImVec2(centerX, y));
    ImGui::TextUnformatted("Puerto");
    ImGui::SetCursorPos(ImVec2(centerX, y + 32));
    ImGui::SetNextItemWidth(fieldWidth);
    ImGui::InputInt("##port", &port, 0);
    if (port < 0)     port = 0;
    if (port > 65535) port = 65535;

    // Contraseña
    y += rowSpacing + 22;
    ImGui::SetCursorPos(ImVec2(centerX, y));
    ImGui::TextUnformatted("Contraseña");
    ImGui::SetCursorPos(ImVec2(centerX, y + 32));
    ImGui::SetNextItemWidth(fieldWidth);
    if (ImGui::InputText("##password", passwordBuffer, IM_ARRAYSIZE(passwordBuffer)))
    {

    }

    ImGui::PopFont();

    // ---- Botón conectar ----
    float buttonWidth  = 200.0f;
    float buttonHeight = buttonWidth / 2.5f;
    y += rowSpacing + 40.0f;

    ImGui::PushFont(ImGuiFonts::fonts[WINDOWS_FONT_ID][2]);
    if (DrawButtonWithShadow("Conectar",
        ImVec2(centerX + fieldWidth / 2 - buttonWidth / 2, y),
        ImVec2(buttonWidth, buttonHeight), true))
    {
        shouldClose = true;
        app->netManager->tryConnection(ipBuffer,port,passwordBuffer);
   
    }
    ImGui::PopFont();

    ImGui::End();

}

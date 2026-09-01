//
// Created by migue on 29/08/2026.
//

#include "ChooseAppTypeMenu.h"
#include "MenuUtils.h"
#include "../App/App.h"
#include "../Registry/ImGuiFonts.h"
#include "imGUI/imgui.h"
#include "../App/DesktopDuplicatorManager.h"


void ChooseAppTypeMenu::render()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus|
    ImGuiWindowFlags_NoScrollbar |                      // <-- recomendado
    ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::Begin("MainMenu", nullptr, flags);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();


    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = style.FrameRounding;
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_InputTextCursor] = ImVec4(.0f, 0.0f, 0.0f, 1.00f);
    colors[ImGuiCol_WindowBg]      = ImVec4(0.9f, 0.9f, 0.9f, 1.00f); // gris casi blanco, no blanco puro
    colors[ImGuiCol_Text]          = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
    colors[ImGuiCol_Border]        = ImVec4(0.80f, 0.80f, 0.82f, 1.00f);

    // Botón primario (acción principal, ej. "Start Server")
    colors[ImGuiCol_Button]        = ImVec4(0.16f, 0.45f, 0.85f, 1.00f); // azul saturado, no pastel
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.52f, 0.92f, 1.00f);
    colors[ImGuiCol_ButtonActive]  = ImVec4(0.12f, 0.38f, 0.75f, 1.00f);
    colors[ImGuiCol_Text]          = ImVec4(0.10f, 0.10f, .10f, 1.00f);


    // Franja superior de marca
    float headerHeight = 80.0f;
    drawList->AddLine(
        ImVec2(winPos.x, winPos.y + headerHeight),
        ImVec2(winPos.x + winSize.x, winPos.y + headerHeight),
        IM_COL32(200, 200, 200, 255), 1.0f
    );
    ImGui::PushFont(ImGuiFonts::fonts[WINDOWS_FONT_BOLD_ID][2]); // fuente más grande para el título
    ImGui::SetCursorPos(ImVec2(24, 24));
    ImGui::TextColored(ImVec4(.1,.1,.1,1), "Select Mode ");
    ImGui::PopFont();

    // Footer sutil
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




    ImGui::PushFont(ImGuiFonts::fonts[WINDOWS_FONT_ID][2]);
    // Área de contenido con padding tipo wizard

    // Botones anclados abajo a la derecha, como Next/Cancel
    float buttonWidth = 200.0f;
    float buttonHeight = buttonWidth/2.5;
    float y = viewport->WorkSize.y/2 - buttonHeight/2;
    float spacement = 200;
    //Espacio total = spacement+buttonWidth*2 -> el cursor tiene que estar en el x total / 2 -espacioTotal/2
    float totalSize =( spacement +buttonWidth*2);


    if (DrawButtonWithShadow("Start Server",ImVec2(viewport->WorkSize.x /2- totalSize/2, y), ImVec2(buttonWidth, buttonHeight),false)) {
        app->startServer();
        shouldClose = true;
        Menus::openMenu(LISTENING_MENU_ID);
    }

    if (DrawButtonWithShadow("Connect to Server",ImVec2(viewport->WorkSize.x /2+ spacement/2, y), ImVec2(buttonWidth, buttonHeight),false)) {
       app->startClient();
        shouldClose = true;
        Menus::openMenu(CONNECT_MENU_ID);
    }

    ImGui::PopFont();

    ImGui::End();


}
//
// Created by migue on 02/09/2026.
//

#include "SendMessageMenu.h"

#include "MenuUtils.h"
#include "../App/App.h"
#include "../Registry/ImGuiFonts.h"
#include "imGUI/imgui.h"

void SendMessageMenu::render()
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

    ImGui::Begin("Send Message Menu", nullptr, flags);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();

    // ---- Paleta de colores, igual que en los otros menus ----
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = style.FrameRounding;
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]       = ImVec4(0.9f, 0.9f, 0.9f, 1.00f);
    colors[ImGuiCol_Text]           = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
    colors[ImGuiCol_Border]         = ImVec4(0.80f, 0.80f, 0.82f, 1.00f);
    colors[ImGuiCol_Button]         = ImVec4(0.16f, 0.45f, 0.85f, 1.00f);
    colors[ImGuiCol_ButtonHovered]  = ImVec4(0.22f, 0.52f, 0.92f, 1.00f);
    colors[ImGuiCol_ButtonActive]   = ImVec4(0.12f, 0.38f, 0.75f, 1.00f);
    colors[ImGuiCol_FrameBg]        = ImVec4(1.0f, 1.0f, 1.0f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.95f, 0.97f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBgActive]  = ImVec4(0.90f, 0.95f, 1.00f, 1.00f);
    style.FrameBorderSize           = 1.0f;

    // ---- Franja superior de marca ----
    float headerHeight = 80.0f;
    drawList->AddLine(
        ImVec2(winPos.x, winPos.y + headerHeight),
        ImVec2(winPos.x + winSize.x, winPos.y + headerHeight),
        IM_COL32(200, 200, 200, 255), 1.0f
    );
    ImGui::PushFont(ImGuiFonts::fonts[WINDOWS_FONT_BOLD_ID][2]);
    ImGui::SetCursorPos(ImVec2(24, 24));
    ImGui::TextColored(ImVec4(.1f, .1f, .1f, 1), "Send Message");
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

    // ---- Caja de texto + botón, centrados ----
    float fieldWidth = 400.0f;
    float centerX = viewport->WorkSize.x / 2 - fieldWidth / 2;
    float y = viewport->WorkSize.y / 2 - 60.0f;

    ImGui::PushFont(ImGuiFonts::fonts[WINDOWS_FONT_ID][2]);

    ImGui::SetCursorPos(ImVec2(centerX, y));
    ImGui::TextUnformatted("Mensaje");
    ImGui::SetCursorPos(ImVec2(centerX, y + 32));
    ImGui::SetNextItemWidth(fieldWidth);
    bool enterPressed = ImGui::InputText("##message", messageBuffer, IM_ARRAYSIZE(messageBuffer),
                                          ImGuiInputTextFlags_EnterReturnsTrue);

    ImGui::PopFont();

    float buttonWidth  = 200.0f;
    float buttonHeight = buttonWidth / 2.5f;
    y += 32 + 55.0f;

    ImGui::PushFont(ImGuiFonts::fonts[WINDOWS_FONT_ID][2]);
    bool buttonPressed = DrawButtonWithShadow("Enviar",
        ImVec2(centerX + fieldWidth / 2 - buttonWidth / 2, y),
        ImVec2(buttonWidth, buttonHeight), true);
    ImGui::PopFont();

    if (buttonPressed || enterPressed)
    {
        if (messageBuffer[0] != '\0')
        {
            app->netManager->sendPackage(messageBuffer, NetManager::MESSAGE);
            messageBuffer[0] = '\0';
        }
    }
    if (app->netManager->getStatus() != NetManager::CONNECTED)
    {
        shouldClose = true;
        if (app->type == App::CLIENT)
        {
            Menus::openMenu(CONNECT_MENU_ID);
        } else
        {
            Menus::openMenu(LISTENING_MENU_ID);
        }

    }
    ImGui::End();
}
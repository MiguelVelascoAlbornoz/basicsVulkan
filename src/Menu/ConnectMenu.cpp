#include "ConnectMenu.h"
#include "MenuUtils.h"
#include "../App/App.h"
#include "../Registry/ImGuiFonts.h"
#include "imGUI/imgui.h"

void ConnectMenu::setConnectionResult(bool success, const std::string& message)
{
    lastAttemptSuccess = success;
    resultMessage = message;
    state = State::RESULT;
}

void ConnectMenu::render()
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

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = style.FrameRounding;
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]      = ImVec4(0.9f, 0.9f, 0.9f, 1.00f);
    colors[ImGuiCol_Text]          = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
    colors[ImGuiCol_Border]        = ImVec4(0.80f, 0.80f, 0.82f, 1.00f);
    colors[ImGuiCol_Button]        = ImVec4(0.16f, 0.45f, 0.85f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.52f, 0.92f, 1.00f);
    colors[ImGuiCol_ButtonActive]  = ImVec4(0.12f, 0.38f, 0.75f, 1.00f);
    colors[ImGuiCol_FrameBg]         = ImVec4(1.0f, 1.0f, 1.0f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]  = ImVec4(0.95f, 0.97f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBgActive]   = ImVec4(0.90f, 0.95f, 1.00f, 1.00f);
    colors[ImGuiCol_Border]          = ImVec4(0.70f, 0.70f, 0.72f, 1.00f);
    style.FrameBorderSize            = 1.0f;

    // ---- Franja superior de marca ----
    float headerHeight = 80.0f;
    drawList->AddLine(
        ImVec2(winPos.x, winPos.y + headerHeight),
        ImVec2(winPos.x + winSize.x, winPos.y + headerHeight),
        IM_COL32(200, 200, 200, 255), 1.0f
    );
    ImGui::PushFont(ImGuiFonts::fonts[WINDOWS_FONT_BOLD_ID][2]);
    ImGui::SetCursorPos(ImVec2(24, 24));
    ImGui::TextColored(ImVec4(.1f, .1f, .1f, 1),
        state == State::FORM ? "Connect to Server" :
        state == State::CONNECTING ? "Connecting..." : "Connection Result");
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

    // ================= ESTADO: FORMULARIO =================
    if (state == State::FORM)
    {
        float fieldWidth = 320.0f;
        float centerX = viewport->WorkSize.x / 2 - fieldWidth / 2;
        float y = headerHeight + 50.0f;
        float rowSpacing = 70.0f;
        float labelToInputGap = 8.0f;

        ImGui::PushFont(ImGuiFonts::fonts[WINDOWS_FONT_ID][2]);

        // IP
        ImGui::SetCursorPos(ImVec2(centerX, y));
        ImGui::TextUnformatted("IP");
        ImGui::SetCursorPos(ImVec2(centerX, y + 22 + labelToInputGap));
        ImGui::SetNextItemWidth(fieldWidth);
        ImGui::InputText("##ip", ipBuffer, IM_ARRAYSIZE(ipBuffer));


        // Puerto
        y += rowSpacing + 22;
        ImGui::SetCursorPos(ImVec2(centerX, y));
        ImGui::TextUnformatted("Puerto");
        ImGui::SetCursorPos(ImVec2(centerX, y + 22 + labelToInputGap));
        ImGui::SetNextItemWidth(fieldWidth);
        ImGui::InputInt("##port", &port, 0);
        if (port < 0)     port = 0;
        if (port > 65535) port = 65535;

        // Contraseña
        y += rowSpacing + 22;
        ImGui::SetCursorPos(ImVec2(centerX, y));
        ImGui::TextUnformatted("Contraseña");
        ImGui::SetCursorPos(ImVec2(centerX, y + 22 + labelToInputGap));
        ImGui::SetNextItemWidth(fieldWidth);
        ImGui::InputText("##password", passwordBuffer, IM_ARRAYSIZE(passwordBuffer), ImGuiInputTextFlags_Password);


        // Botón conectar
        float buttonWidth  = 200.0f;
        float buttonHeight = buttonWidth / 2.5f;
        y += rowSpacing + 40.0f;
        if (DrawButtonWithShadow("Conectar",
            ImVec2(centerX + fieldWidth / 2 - buttonWidth / 2, y),
            ImVec2(buttonWidth, buttonHeight), true))
        {
            app->netManager->tryConnection(ipBuffer, port, passwordBuffer);
            state = State::CONNECTING;
        }

        ImGui::PopFont();
    }
    // ================= ESTADO: CONECTANDO =================
    else if (state == State::CONNECTING)
    {
        if (app->netManager->getStatus() != NetManager::CONNECTING)
        {
            state = State::RESULT;
        }
        ImGui::PushFont(ImGuiFonts::fonts[WINDOWS_FONT_ID][2]);

        std::string message = std::string("Trying to connect to ") + ipBuffer + ":" + std::to_string(port);
        ImVec2 msgSize = ImGui::CalcTextSize(message.c_str());
        ImGui::SetCursorPos(ImVec2(viewport->WorkSize.x / 2 - msgSize.x / 2,
                                    viewport->WorkSize.y / 2 - 20));
        ImGui::TextColored(ImVec4(.1f, .1f, .1f, 1), "%s", message.c_str());

        std::string dots = GetLoadingDots();
        ImVec2 dotsSize = ImGui::CalcTextSize(dots.c_str());
        ImGui::SetCursorPos(ImVec2(viewport->WorkSize.x / 2 - dotsSize.x / 2,
                                    viewport->WorkSize.y / 2 + 12));
        ImGui::TextColored(ImVec4(0.16f, 0.45f, 0.85f, 1.0f), "%s", dots.c_str());

        ImGui::PopFont();
    }
    // ================= ESTADO: RESULTADO =================
    else if (state == State::RESULT)
    {
        if (app->netManager->getStatus() == NetManager::INVALID_IP)
        {
            resultMessage = "Invalid IP";
        } else if (app->netManager->getStatus() == NetManager::CONNECTED)
        {
            resultMessage = "Succesfully connected";
        } else
        {
            resultMessage = "Unexpected error";
        }
        ImGui::PushFont(ImGuiFonts::fonts[WINDOWS_FONT_ID][2]);

        ImVec4 msgColor = lastAttemptSuccess
            ? ImVec4(0.2f, 0.6f, 0.2f, 1.0f)
            : ImVec4(0.8f, 0.2f, 0.2f, 1.0f);

        ImVec2 msgSize = ImGui::CalcTextSize(resultMessage.c_str());
        float msgY = viewport->WorkSize.y / 2 - 60;
        ImGui::SetCursorPos(ImVec2(viewport->WorkSize.x / 2 - msgSize.x / 2, msgY));
        ImGui::TextColored(msgColor, "%s", resultMessage.c_str());
        colors[ImGuiCol_Button]        = ImVec4(0.96f, 0.25f, 0.05f, 1.00f);
        float buttonWidth  = 200.0f;
        float buttonHeight = buttonWidth / 2.5f;

        if (DrawButtonWithShadow("Try again",
            ImVec2(viewport->WorkSize.x / 2 - buttonWidth / 2, msgY + 50),
            ImVec2(buttonWidth, buttonHeight), true))
        {
            state = State::FORM;
        }

        ImGui::PopFont();
    }

    ImGui::End();
}
#include "StartMenu.h"
#include <SDL3/SDL_dialog.h>
#include <imGUI/imgui.h>
#include <iostream>
/**
 * @file StartMenu.h
 * @brief StartMenu class declaration and all his features.
 * @author Miguel Velasco
 */

/**
 * @brief Callback function for file selection in the SDL file dialog. This function is called when a file is selected in the dialog, and it receives the selected file list, the filter index, and user data. The function prints the selected file, filter index, and user data to the standard output.
 */
void fileSelectedCallback(void* userdata, const char* const* filelist, int filter) {
    if (filelist && filelist[0]) {
        std::cout << "Selected file: " << filelist[0] << std::endl;
        std::cout << "Filter index: " << filter << std::endl;
        std::cout << "User data: " << userdata << std::endl;
    } else {
        std::cout << "No file selected." << std::endl;
    }
}

void StartMenu::render()
{
     ImGui::Begin("Start Menu",NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
        if (ImGui::Button("Select Sound File") ){
            SDL_DialogFileFilter filters[] = {
                { "Sound Files", "wav;mp3;ogg" }
            };
            SDL_ShowOpenFileDialog(fileSelectedCallback, this, window->window, filters,1,"C:\\",false);
            
        }
        ImGui::End();
}

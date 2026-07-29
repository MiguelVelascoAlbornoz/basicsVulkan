//
// Created by migue on 29/07/2026.
//

#include "F3GUI.h"
ImGui::Begin("##Data", &show, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
ImGui::Text(("Pos: (" + std::to_string(pos->x) + "," + std::to_string(pos->y) + "," + std::to_string(pos->z) + ")").c_str());
ImGui::Text(("Direction: (" + std::to_string(directionVector->x) + "," + std::to_string(directionVector->y) + "," + std::to_string(directionVector->z) + ")").c_str());
ImGui::Text(("Left Direction: (" + std::to_string(leftVector->x) + "," + std::to_string(leftVector->y) + "," + std::to_string(leftVector->z) + ")").c_str());
ImGui::Text(("Up Direction: (" + std::to_string(upVector->x) + "," + std::to_string(upVector->y) + "," + std::to_string(upVector->z) + ")").c_str());
ImGui::Text(("Angle (pitch,yaw,roll): (" + std::to_string(*pitch) + "," + std::to_string(*yaw) + "," + std::to_string(*roll) + ")").c_str());
ImGui::Text(("Fps: " + std::to_string(1 / ellapsedTime)).c_str());
ImGui::Text("Direction Vector: (%.5f, %.5f, %.5f)", directionVector->x, directionVector->y, directionVector->z, "%.5f");
ImGui::End();
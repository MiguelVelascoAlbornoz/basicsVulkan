//
// Created by migue on 29/07/2026.
//

#include "F3GUI.h"


#include <imGUI/imgui.h>
#include "../App/App.h"

void F3GUI::render()
{
    Player* player = app->player;
    const glm::vec3* pos = player->getPosition();
    mat3 worldMatrix = player->camera->getWorldMatrix();
    const vec3 directionVector = worldMatrix[1];
    const vec3 leftVector = worldMatrix[2];
    const vec3 upVector = worldMatrix[0];
    vec3 rotationVector = player->getRotation();
    const float pitch = rotationVector[0];
    const float yaw = rotationVector[1];
    const float roll = rotationVector[2];

    const unsigned int tps =static_cast<unsigned > (1000000000.0f/ app->tickDeltaTimeNS);
    const unsigned int cyclesPerSecond =static_cast<unsigned > (1000000000.0f/ app->cycleDeltaTimeNS);
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);
    ImGui::Begin("##Data",nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    ImGui::Text(("Pos: (" + std::to_string(pos->x) + "," + std::to_string(pos->y) + "," + std::to_string(pos->z) + ")").c_str());
    ImGui::Text(("Direction: (" + std::to_string(directionVector.x) + "," + std::to_string(directionVector.y) + "," + std::to_string(directionVector.z) + ")").c_str());
    ImGui::Text(("Left Direction: (" + std::to_string(leftVector.x) + "," + std::to_string(leftVector.y) + "," + std::to_string(leftVector.z) + ")").c_str());
    ImGui::Text(("Up Direction: (" + std::to_string(upVector.x) + "," + std::to_string(upVector.y) + "," + std::to_string(upVector.z) + ")").c_str());
    ImGui::Text(("Angle (pitch,yaw,roll): (" + std::to_string(pitch) + "," + std::to_string(yaw) + "," + std::to_string(roll) + ")").c_str());
    ImGui::Text(("TPS: " +std::to_string(tps)).c_str());
    ImGui::Text(("Cycles per second: " +std::to_string(cyclesPerSecond)).c_str());
    //ImGui::Text("Direction Vector: (%.5f, %.5f, %.5f)", directionVector->x, directionVector->y, directionVector->z, "%.5f");
    ImGui::End();
    //std::cout << *deltaTime << std::endl;
}

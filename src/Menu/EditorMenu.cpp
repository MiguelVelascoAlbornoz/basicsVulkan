#include "EditorMenu.h"
#include <SDL3/SDL_dialog.h>
#include <imGUI/imgui.h>
#include <iostream>
/**
 * @file EditorMenu.h
 * @brief StartMenu class declaration and all his features.
 * @author Miguel Velasco
 */

/**
 * @brief Callback function for file selection in the SDL file dialog. This function is called when a file is selected in the dialog, and it receives the selected file list, the filter index, and user data. The function prints the selected file, filter index, and user data to the standard output.
 */

using namespace  ImGui;


void EditorMenu::render()
{

	ImGui::Begin("Camera Control");
	if (CollapsingHeader("World: ")) {
		vec3 playerPosition = *player->getPosition();
		Indent();
		ImGui::Text("Pos:"); ImGui::SameLine();
		if (InputFloat3("##Pos", &playerPosition.x, "%.5f")) {
			player->setPosition(playerPosition);
		}
		vec4 playerRotation = player->getRotation();
		ImGui::Text("Yaw:"); ImGui::SameLine();
		if (SliderFloat("##Yaw", &playerRotation.x, -180, 180)) {
			player->setRotation(playerRotation);
		}
		ImGui::Text("Pitch:"); ImGui::SameLine();
		if (SliderFloat("##Pitch", &playerRotation.y, -89, 89)) {

			player->setRotation(playerRotation);
		}
		ImGui::Text("Roll:"); ImGui::SameLine();
		if (ImGui::SliderFloat("##Roll", &playerRotation.z, -180, 180)) {
			player->setRotation(playerRotation);
		}
		ImGui::Text("Yaw To Roll:"); ImGui::SameLine();
		if (ImGui::SliderFloat("##YawToRoll", &playerRotation.w, -180, 180)) {
			player->setRotation(playerRotation);
		}

	}
	if (CollapsingHeader("Visuals: ")) {
		Indent();
		Player::PlayerCameraSettings cameraSettings = *player->getPlayerCameraSettings();
		ImGui::Text("Fov:"); ImGui::SameLine();
		if (SliderFloat("##Fov", &cameraSettings.fov, 0, 180)) {
			player->setPlayerCameraSettings(cameraSettings);
		}
		ImGui::Text("Far:"); ImGui::SameLine();
		if (DragFloat("##Far", &cameraSettings.farPlane, 1)) {
			player->setPlayerCameraSettings(cameraSettings);		}
		ImGui::Text("Near:"); ImGui::SameLine();
		if (DragFloat("##Near", &cameraSettings.nearPlane, .1)) {
			player->setPlayerCameraSettings(cameraSettings);
		}

		Unindent();
	}
	if (CollapsingHeader("Movement: ")) {
		Indent();
		ImGui::Text("Sensibility:"); ImGui::SameLine();
		if (DragFloat("##Sensibility", &player->rotationSensitivity, .1)) {
			if (player->rotationSensitivity <= 0) {
				player->rotationSensitivity = 1;
			}
		}
		ImGui::Text("Speed:"); ImGui::SameLine();
		if (DragFloat("##Speed", &player->speed, .1)) {
			if (player->speed <= 0) {
				player->speed = 1;
			}
		}
		ImGui::Text("Speed Multiplier:"); ImGui::SameLine();
		if (DragFloat("##Speed Multiplier", &player->speedMultiplier, .1)) {
			if (player->speedMultiplier <= 0) {
				player->speedMultiplier = 1;
			}
		}

		Unindent();

	}
        ImGui::End();
}

#include "EditorMenu.h"
#include <imGUI/imgui.h>

#include "../App/App.h"
#include "../Scene/Player.h"

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
	Player* player = this->app->player;
	ImGui::Begin("Camera Control");

	if (CollapsingHeader("World: ")) {
		vec3 playerPosition = *player->getPosition();
		Indent();
		ImGui::Text("Pos:"); ImGui::SameLine();
		if (InputFloat3("##Pos", &playerPosition.x, "%.5f")) {
			player->setPosition(playerPosition);
			Uniforms::onPlayerRenderUpdate();
		}
		vec4 playerRotation = player->getRotation();
		ImGui::Text("Yaw:"); ImGui::SameLine();
		if (SliderFloat("##Yaw", &playerRotation.x, -180, 180)) {
			player->setRotation(playerRotation);
			Uniforms::onPlayerRenderUpdate();
		}
		ImGui::Text("Pitch:"); ImGui::SameLine();
		if (SliderFloat("##Pitch", &playerRotation.y, -89, 89)) {

			player->setRotation(playerRotation);
			Uniforms::onPlayerRenderUpdate();
		}
		ImGui::Text("Roll:"); ImGui::SameLine();
		if (ImGui::SliderFloat("##Roll", &playerRotation.z, -180, 180)) {
			player->setRotation(playerRotation);
			Uniforms::onPlayerRenderUpdate();
		}
		ImGui::Text("Yaw To Roll:"); ImGui::SameLine();
		if (ImGui::SliderFloat("##YawToRoll", &playerRotation.w, -180, 180)) {
			player->setRotation(playerRotation);
			Uniforms::onPlayerRenderUpdate();
		}

	}
	if (CollapsingHeader("Visuals: ")) {
		Indent();
		Player::PlayerCameraSettings cameraSettings = *player->getPlayerCameraSettings();
		ImGui::Text("Fov:"); ImGui::SameLine();
		if (SliderFloat("##Fov", &cameraSettings.fov, 0, 180)) {
			player->setPlayerCameraSettings(cameraSettings);
			Uniforms::onPlayerRenderUpdate();
		}
		ImGui::Text("Far:"); ImGui::SameLine();
		if (DragFloat("##Far", &cameraSettings.farPlane, .1,cameraSettings.nearPlane)){
			player->setPlayerCameraSettings(cameraSettings);
			Uniforms::onPlayerRenderUpdate();
		}

		ImGui::Text("Near:"); ImGui::SameLine();
		if (DragFloat("##Near", &cameraSettings.nearPlane, .1,0.0000001,cameraSettings.farPlane)) {
			player->setPlayerCameraSettings(cameraSettings);
			Uniforms::onPlayerRenderUpdate();
		}
		ImGui::Text("Max cycles per second:"); ImGui::SameLine();
		if (DragInt("##mcps", &cameraSettings.maxCyclesPerSecond, .1,1)) {
			player->setPlayerCameraSettings(cameraSettings);
		}
		ImGui::Text("Max ticks per second:"); ImGui::SameLine();
		if (DragInt("##mtps", &cameraSettings.maxTicksPerSecond, .1,1)) {
			player->setPlayerCameraSettings(cameraSettings);
		}
		ImGui::Text("FBO resolution ratio:"); ImGui::SameLine();
		if (DragFloat("##FBORes", &cameraSettings.fboResolutionMultiplier, .1,0.000001)) {
			player->setPlayerCameraSettings(cameraSettings);
			app->onFBOResolutionChange();
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

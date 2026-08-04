//
// Created by migue on 17/07/2026.
//


//
// Created by migue on 17/07/2026.
//
#include "Player.h"
#include "../Renderer/Camera.h"
#include  "../Input/Settings.h"
Player::Player(int id) : playerID(id) {
    camera = new Camera();

    settings =new Settings(std::string("player") + std::to_string(playerID), {
                            {&speed, "fspeed"},
                            {&speedMultiplier, "fspeedMultiplier"},
                            {&rotationSensitivity, "frotationSensitivity"},
                            {&yaw, "fyaw"},
                            {&pitch, "fpitch"},
                            {&roll, "froll"},
                            {&yawToRoll, "fyawToRoll"},
                            {&position.x, "fposition.x"},
                            {&position.y, "fposition.y"},
                            {&position.z, "fposition.z"},
        {&cameraSettings.fov, "ffov"},
        {&cameraSettings.aspectRatio, "faspectRatio"},
        {&cameraSettings.nearPlane, "fnearPlane"},
        {&cameraSettings.farPlane, "ffarPlane"},
        {&cameraSettings.maxCyclesPerSecond, "imaxCyclesPerSecond"},
        {&cameraSettings.maxTicksPerSecond, "imaxTicksPerSecond"},
        {&cameraSettings.fboResolutionMultiplier, "ffboResolutionMultiplier"},
                        });

    camera->setPosition(position);
    camera->setRotation(yaw,pitch,roll,yawToRoll);
    camera->setPerspective(cameraSettings.fov,cameraSettings.aspectRatio,cameraSettings.nearPlane,cameraSettings.farPlane);

}

void Player::setRotation(float yaw, float pitch, float roll, float yawToRoll) {

        this->yaw = yaw;
        this->pitch = pitch;
        this->roll = roll;
        this->yawToRoll = yawToRoll;
        camera->setRotation(yaw,pitch,roll,yawToRoll);

}

void Player::setRotation(vec4 &rotation) {
    yaw = rotation.x;
    pitch = rotation.y;
    roll = rotation.z;
    yawToRoll = rotation.w;
    camera->setRotation(yaw,pitch,roll,yawToRoll);
}

void Player::move(vec3 delta) {
    position = position + delta;
    camera->setPosition(position);
}

void Player::setPosition(vec3 &newPos) {
    camera->setPosition(newPos);
    position = newPos;
}

void Player::setPlayerCameraSettings(PlayerCameraSettings& newCameraSettings) {
    cameraSettings = newCameraSettings;
    camera->setPerspective(cameraSettings.fov,cameraSettings.aspectRatio,cameraSettings.nearPlane,cameraSettings.farPlane);
}

Player::~Player() {
    settings->saveSettings();
    delete camera;
    delete settings;
}


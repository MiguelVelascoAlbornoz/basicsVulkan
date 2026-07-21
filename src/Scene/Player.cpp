//
// Created by migue on 17/07/2026.
//


//
// Created by migue on 17/07/2026.
//
#include "Player.h"

Player::Player(int id) : playerID(id) {
    camera = new Camera();

    settings =new Settings("player" + (playerID), {
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
        {&cameraSettings.farPlane, "ffarPlane"}
                        });

    camera->setPosition(position);
    camera->setRotation(yaw,pitch,roll,yawToRoll);
    camera->setPerspective(cameraSettings.fov,cameraSettings.aspectRatio,cameraSettings.nearPlane,cameraSettings.farPlane);

}
Player::~Player() {
    settings->saveSettings();
    delete camera;
    delete settings;
}


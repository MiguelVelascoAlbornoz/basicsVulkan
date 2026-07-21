
#ifndef BASICSVULKAN_PLAYER_H
#define BASICSVULKAN_PLAYER_H

#include "Player.h"
#include "../Input/Settings.h"
#include "../renderer/Camera.h"

class Player {
public:
    Player(int id);
    ~Player();
    Camera* camera;
    float speed = 0.001;
    float speedMultiplier = 2;
    float rotationSensitivity = 0.01;

private:
    struct PlayerCameraSettings {
        float fov;
        float aspectRatio;
        float farPlane;
        float nearPlane;
    };
    PlayerCameraSettings cameraSettings;
    vec3 position;
    float yaw,pitch,roll,yawToRoll;

    const int playerID;
    Settings* settings;
};


#endif //BASICSVULKAN_PLAYER_H

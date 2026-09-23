
#ifndef BASICSVULKAN_PLAYER_H
#define BASICSVULKAN_PLAYER_H

#include "Player.h"
class Settings;
class Camera;
#include <glm/glm.hpp>
using namespace glm;

class Player {
public:
    struct PlayerCameraSettings {
        enum BitsPerChannel
        {
            BITS_8,
            BITS_16,
            BITS_32
        };
        float fov;
        float aspectRatio;
        float farPlane;
        float nearPlane;
        int maxTicksPerSecond;
        int maxCyclesPerSecond;
        float fboResolutionMultiplier = 1;
        int MSAAsamples = 0;
        bool sampleShadding = false;
        BitsPerChannel bitsPerChannel;


    };
    Player(int id);
    ~Player();
    Camera* camera; /** @brief Camera view of the player. @details dont change camera parameter to not cause desincronization with the player**/
    float speed = 0.001;
    float speedMultiplier = 2;
    float rotationSensitivity = 0.01;
    void setRotation(float yaw,float pitch, float roll, float yawToRoll);
    void setRotation(vec4& rotation) ;
    void move(vec3 delta);

    const vec3* getPosition() {
        return &position;
    }

    /**
     *
     * @return  returns a vec4 which the next information (yaw,picth,roll, yawToRoll)
     */
    const vec4 getRotation() {
        return vec4(yaw,pitch,roll,yawToRoll);
    }
    const PlayerCameraSettings* getPlayerCameraSettings() {
        return &cameraSettings;
    }
    void setPosition(vec3& newPos);

    void setPlayerCameraSettings(PlayerCameraSettings& newCameraSettings);

private:

    PlayerCameraSettings cameraSettings;
    vec3 position;
    float yaw,pitch,roll,yawToRoll;

    const int playerID;
    Settings* settings;
};


#endif //BASICSVULKAN_PLAYER_H

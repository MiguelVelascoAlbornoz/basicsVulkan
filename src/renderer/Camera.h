#ifndef CAMERA_H
#define CAMERA_H

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define DEFAULT_WORLD_UP glm::vec3(0.0f, 1.0f, 0.0f) /**< @brief Default up vector for the camera. */
#define DEFAULT_WORLD_FRONT glm::vec3(0.0f, 0.0f, 1.0f) /**< @brief Default front vector for the camera. */
#define DEFAULT_WORLD_RIGHT glm::vec3(1.0f, 0.0f, 0.0f) /**< @brief Default right vector for the camera. */
#define CAMERA_FIELDS_COUNT 1 /**< @brief Number of dirty flags used to track changes in the camera's state. */


using namespace glm;

class Camera {
public:

    // (void*) const es un puntero constante
    // (void const)* es un puntero a datos constantes

    void setRotation(float yaw, float pitch, float roll,float yawToRoll);

    void setPosition(const glm::vec3& position) {
        this->position = position;
        updateViewMatrix();
        updateViewProjectionMatrix();

    }

    void setPerspective(float fov, float aspectRatio, float nearPlane, float farPlane);

    void setOrtho(float left, float right, float up, float down, float mfar, float mnear);

    const mat4* getViewProjectionMatrix()  {
        return &viewProjectionMatrix;
    };
    const vec3* getPosition() {
        return &position;
    }

    /**
     *
     * @return Retorna una matriz 3x3 en que los vectores son las columnas.
     * Columna 0: cameraUp.
     * Columna 1: cameraFront.
     * Columna 2: cameraRight.
     */
    mat3 getWorldMatrix() {
        return {up,front,right};
    }
    /** *
     * @return Retorna un vec4 con toda la informacion de la rotation de la camara del tipo: (yaw,pitch,roll,yawToRoll)
     */
    [[nodiscard]] vec4 getCameraRotation() {
        return {yaw,pitch,roll,yawToRoll};
    }
    const float*  getFov() {
        return &fov;
    }


private:

    void updateViewMatrix();
    void updateViewProjectionMatrix() {
        viewProjectionMatrix = projectionMatrix * viewMatrix;
    }
    glm::vec3 position = glm::vec3(0,0,0); /**< @brief Position of the camera in world space. */
    glm::vec3 front = DEFAULT_WORLD_FRONT; /**< @brief Direction the camera is facing. Its always normalized */
    glm::vec3 up = DEFAULT_WORLD_UP; /**< @brief Up direction of the camera. Its always normalized*/
    glm::vec3 right = DEFAULT_WORLD_RIGHT; /**< @brief Right direction of the camera. Its always normalized */
    float fov = 45.0f; /**< @brief Field of view (in degrees) for the camera's perspective projection. */
    float aspectRatio = 1.0f; /**< @brief Aspect ratio (width/height) for the camera's perspective projection. */
    float nearPlane = 0.1f; /**< @brief Near clipping plane distance for the camera's perspective projection. */
    float farPlane = 100.0f; /**< @brief Far clipping plane distance for the camera's perspective projection. */
    float yaw = 0; /**< @brief Yaw angle (in degrees) for the camera's orientation. */
    float pitch = 0; /**< @brief Pitch angle (in degrees) for the camera's orientation. */
    float roll = 0; /**< @brief Roll angle (in degrees) for the camera's orientation. */
    float yawToRoll = 0; /**< @brief Additional yaw to roll angle (in degrees) */
    float movementSpeed = 0; /**< @brief Speed at which the camera moves through the scene. */
    float mouseSensitivity = 0; /**< @brief Sensitivity of mouse movement for camera rotation. */
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(1.0f, 0.0f, 0.0f))*
                               glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 1.0f, 0.0f))*
                               glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 1.0f, 0.0f))*
                               glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f))*
                               glm::rotate(glm::mat4(1.0f), -0.0f, glm::vec3(0.0f, 1.0f, 0.0f)); /**< @brief Rotation matrix representing the camera's orientation. */
    glm::mat4 viewMatrix = glm::lookAt(position,position+front,up); /**< @brief View matrix used for rendering, calculated from the camera's position and orientation. */
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane); /**< @brief Projection matrix used for rendering, defining how 3D points are projected onto the 2D screen. */
    glm::mat4 viewProjectionMatrix = projectionMatrix*viewMatrix; /**< @brief Combined view-projection matrix used for rendering, product of view and projection matrices. */

};
#endif // CAMERA_H
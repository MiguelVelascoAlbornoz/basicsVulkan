//
// Created by migue on 30/07/2026.
//

#ifndef BASICSVULKAN_MODEL_H
#define BASICSVULKAN_MODEL_H

#include <glm/glm.hpp>

using namespace glm;
class Mesh;

class Model
{
public:

    struct alignas(16) ModelUBO
    {
        alignas(16) glm::mat4 modelMatrix = mat4(1.0f);
        alignas(16) glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
        alignas(16) glm::mat4 rotationMatrix = mat4(1.0f);
        enum Fields
        {
            MODEL_MATRIX,
            SCALE,
            ROTATION_MATRIX
        };
    };
    void setRotation(vec3 angles);
    void setTranslation(vec3 position);
    void setModelMatrix(mat4 modelMatrix);
    void setScale(vec3 scale);
    Mesh* mesh;
    [[nodiscard]] const ModelUBO* getModelUBO() const {return &model;};

private:

    vec3 position;
    vec3 angles; //in form (yaw, pitch, roll)
    ModelUBO model;
    mat4 translationMatrix;
    void calculateModelMatrix();

};


#endif //BASICSVULKAN_MODEL_H

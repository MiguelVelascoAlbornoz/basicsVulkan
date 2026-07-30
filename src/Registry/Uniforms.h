//
// Created by migue on 27/07/2026.
//

#ifndef BASICSVULKAN_UNIFORMS_H
#define BASICSVULKAN_UNIFORMS_H

#include "../renderer/UniformBuffer.h"
#include "../App/Utilitys.h"
#include <glm/glm.hpp>

#define CAMERA_UNIFORM_ID "camera_uniform"
#define LINE_UNIFORM_ID "line_uniform"

class Uniforms
{

public:

    struct alignas(16) LineUBO
    {
        alignas(16) glm::vec3 direction;
        alignas(16) glm::vec3 color;

        enum Fields
        {
            DIRECTION,
            COLOR
        };
    };

    struct CameraUBO
    {
        enum Fields
        {
            VIEW_PROJECTION_MATRIX,
            TIME
        };
    };

    static LineUBO lineUniform;
    static UniformBuffer* cameraUniform;


    static void onPlayerRenderUpdate() {
        Uniforms::cameraUniform->addIndexToQueue(CameraUBO::VIEW_PROJECTION_MATRIX);
    };

    static UniformBuffer* registerUniform(const std::string& id, UniformBuffer* uniform) {
        return registerObject(id, uniform, uniforms);
    };
    static std::unordered_map<std::string, UniformBuffer*> uniforms; /**< @brief Map to store menu rendering functions. */
    static void freeUniforms() {
        for (auto& [name, uniform] : uniforms) {
            delete uniform;
        }
        uniforms.clear();
    };

};


#endif //BASICSVULKAN_UNIFORMS_H

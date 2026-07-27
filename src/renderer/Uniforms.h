//
// Created by migue on 27/07/2026.
//

#ifndef BASICSVULKAN_UNIFORMS_H
#define BASICSVULKAN_UNIFORMS_H

#include "UniformBuffer.h"
#include <string>
#include <unordered_map>
#include "../App/Utilitys.h"

class Uniforms
{

public:
    struct LineUBO
    {
        glm::vec3 color;
        glm::vec3 direction;
        UniformBuffer* uniform;
        enum Fields
        {
            VIEW_PROJECTION_MATRIX,
            DIRECTION,
            COLOR
        };
    };
    static LineUBO lineUniform;
    static UniformBuffer* cameraUniform;


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

#ifndef SCENE_H
#define SCENE_H


#include "../renderer/Mesh/Mesh.h"
#include <memory>
#include <list>
#include <functional>
class Scene {
public:
    /** @details Scene dont free meshes**/
    void addMesh(Mesh* mesh);
    void removeMesh(size_t index);
    void render(VkCommandBuffer cmd) const;
    std::function<void(VkCommandBuffer)> renderFunc;
private:
    struct MeshInstance {
        Mesh* mesh;
    };

    std::list<MeshInstance> objects;


};
#endif
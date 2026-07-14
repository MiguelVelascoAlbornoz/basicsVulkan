#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include "../renderer/Mesh.h"
#include <memory>
#include <list>
class Scene {
public:
    void addMesh(std::shared_ptr<Mesh> mesh);
    void removeMesh(size_t index);
    void render(VkCommandBuffer cmd) const;
private:
    struct MeshInstance {
        std::shared_ptr<Mesh> mesh;  
    };

    std::list<MeshInstance> objects;


};
#endif
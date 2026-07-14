#include "Scene.h"
void Scene::render(VkCommandBuffer cmd) const
{
    for (const auto& instance : objects) {
        instance.mesh->bind(cmd);
        instance.mesh->draw(cmd);
    }
}
void Scene::removeMesh(size_t index)
{
    if (index >= objects.size()) {
        return; // Index out of bounds
    }

    auto it = objects.begin();
    std::advance(it, index);
    objects.erase(it);
}
void Scene::addMesh(std::shared_ptr<Mesh> mesh)
{
    MeshInstance instance;
    instance.mesh = mesh;
    objects.push_back(instance);
}

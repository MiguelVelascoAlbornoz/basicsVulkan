#include "Meshes.h"

std::unordered_map<std::string, Mesh*> Meshes::meshes;

Mesh* Meshes::lineMesh;

void Meshes::initMeshes(VulkanDevice* device)
{
    //Solo se pasa la posicion
    std::vector lineVertices = {0.0f,0.0f,0.0f, 1.0f,1.0f,1.0f};
    const std::vector<uint32_t> lineIndices = {0,1};
    lineMesh = registerMesh("line_mesh",new Mesh(device,lineVertices.data(),sizeof(float)*3,2,lineIndices));
}
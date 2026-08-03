#include "Meshes.h"
#include  "../Renderer/Mesh/Mesh.h"
std::unordered_map<std::string, Mesh*> Meshes::meshes;

void Meshes::freeMeshes() {
    for ( std::pair<std::string,Mesh*> mesh: meshes) {
        delete mesh.second;
    }
    meshes.clear();

    lineMesh = nullptr;
    cubeMesh = nullptr;
    quadMesh = nullptr;
}

/** @brief Mesh del cubo, cada vertice esta duplicado y contiene la normal de la cara a la que pertenece. El lado del cubo es 1 por default y esta centrado en el origen**/
Mesh* Meshes::cubeMesh;
Mesh* Meshes::lineMesh;
Mesh* Meshes::quadMesh;


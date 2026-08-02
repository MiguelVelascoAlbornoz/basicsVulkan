#include "Meshes.h"

std::unordered_map<std::string, Mesh*> Meshes::meshes;

/** @brief Mesh del cubo, cada vertice esta duplicado y contiene la normal de la cara a la que pertenece. El lado del cubo es 1 por default y esta centrado en el origen**/
Mesh* Meshes::cubeMesh;
Mesh* Meshes::lineMesh;
Mesh* Meshes::quadMesh;


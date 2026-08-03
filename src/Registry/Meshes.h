//
// Created by migue on 23/07/2026.
//

//#include "Meshes.h"
//
// Created by migue on 23/07/2026.
//

#ifndef BASICSVULKAN_MESHES_H
#define BASICSVULKAN_MESHES_H

#include "../App/Utilitys.h"
class Mesh;
#define LINE_MESH_ID "line_mesh"
#define CUBE_MESH_ID "cube_mesh"
#define QUAD_MESH_ID "quad_mesh"
class Meshes {
public:
    static Mesh* lineMesh;
    static Mesh* cubeMesh;
    static Mesh* quadMesh;

    static std::unordered_map<std::string, Mesh*> meshes; /**< @brief Map to store menu rendering functions. */

    static Mesh* registerMesh(const std::string& id, Mesh* mesh) {
        return registerObject(id, mesh, meshes);
    };


    static void freeMeshes();
};


#endif //BASICSVULKAN_MESHES_H


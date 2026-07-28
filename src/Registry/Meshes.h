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
#include "../renderer/Mesh/Mesh.h"

#define LINE_MESH_ID "line_mesh"
class Meshes {
public:
    static Mesh* lineMesh;
    static std::unordered_map<std::string, Mesh*> meshes; /**< @brief Map to store menu rendering functions. */

    static Mesh* registerMesh(const std::string& id, Mesh* mesh) {
        return registerObject(id, mesh, meshes);
    };

    static void freeMeshes() {
        for (auto& [name, mesh] : meshes) {
            delete mesh;
        }
        meshes.clear();
    };
};


#endif //BASICSVULKAN_MESHES_H


//
// Created by migue on 23/07/2026.
//

//#include "Meshes.h"
//
// Created by migue on 23/07/2026.
//

#ifndef BASICSVULKAN_MESHES_H
#define BASICSVULKAN_MESHES_H

#include "../../App/Utilitys.cpp"
#include "Mesh.h"


class Meshes {
public:
    static void initMeshes();
    static void registerMesh(const std::string& id, Mesh* mesh) {
        registerObject(id, mesh, meshes);
    };
    static std::unordered_map<std::string, Mesh*> meshes; /**< @brief Map to store menu rendering functions. */
    static void freeMeshes() {
        for (auto& [name, mesh] : meshes) {
            delete mesh;
        }
        meshes.clear();
    };
};


#endif //BASICSVULKAN_MESHES_H


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

void Meshes::generatePlaneMesh(std::vector<float>& vertices,
                                std::vector<uint32_t>& indices,
                                uint32_t resolution)
{
    vertices.clear();
    indices.clear();
    int size = 1.0f;
    if (resolution < 2)
    {
        std::cerr << "generatePlaneMesh(): resolution debe ser >= 2" << std::endl;
        return;
    }

    const size_t vertexCount = static_cast<size_t>(resolution) * resolution;
    const size_t floatsPerVertex = 6; // pos.xyz + normal.xyz
    vertices.reserve(vertexCount * floatsPerVertex);

    const size_t quadCount = static_cast<size_t>(resolution - 1) * (resolution - 1);
    indices.reserve(quadCount * 6);

    const float step = size / static_cast<float>(resolution - 1);
    const float halfSize = size * 0.5f;

    // --- Vértices ---
    for (uint32_t z = 0; z < resolution; ++z)
    {
        const float pz = -halfSize + static_cast<float>(z) * step;
        for (uint32_t x = 0; x < resolution; ++x)
        {
            const float px = -halfSize + static_cast<float>(x) * step;

            // Posición
            vertices.push_back(px);
            vertices.push_back(0.0f);
            vertices.push_back(pz);
            // Normal
            vertices.push_back(0.0f);
            vertices.push_back(1.0f);
            vertices.push_back(0.0f);
        }
    }

    // --- Índices ---
    for (uint32_t z = 0; z < resolution - 1; ++z)
    {
        for (uint32_t x = 0; x < resolution - 1; ++x)
        {
            const uint32_t i0 = z * resolution + x;         // (x,   z)
            const uint32_t i1 = i0 + 1;                      // (x+1, z)
            const uint32_t i2 = i0 + resolution;             // (x,   z+1)
            const uint32_t i3 = i2 + 1;                      // (x+1, z+1)

            // Triángulo 1: i0, i2, i1
            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);

            // Triángulo 2: i1, i2, i3
            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }

    #ifdef _DEBUG
    std::cout << "(MESH) Plano generado: " << vertexCount << " vertices, "
              << indices.size() / 3 << " triangulos." << std::endl;
    #endif
}

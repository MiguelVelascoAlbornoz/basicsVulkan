#ifndef VERTEX_LAYOUT_H
#define VERTEX_LAYOUT_H

#include <vector>
#include "AttribType.h"



class VertexLayout {
public:
    VkVertexInputBindingDescription binding;
    std::vector<VkVertexInputAttributeDescription> attributes;

       /**
     * @brief A una dada lista de tipos de entrada (FLOAT, INT, VEC3, etc) le devuelve un VertexLayout que contiene el binding y los atributos con sus offsets y formatos.
       el binding contiene apenas informacion del stride encuanto los atributos cada atributo vendria siendo un layout.
       @details Construye los atributos iterando por cada input del vector dado y creando el vKVertexInputAttributeDescription
       obteniendo su formato y su tamaño apartir de la tabla.
     */
    explicit VertexLayout(std::vector<AttribType::INPUT_TYPES> inputs);
};
#endif
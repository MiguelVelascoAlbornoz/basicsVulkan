#ifndef VERTEX_LAYOUT_H
#define VERTEX_LAYOUT_H

#include <Vulkan/vulkan.h>
#include <vector>

#define INPUT_TYPE_COUNT 10
#define DATA_COUNT_FROM_INPUT_TYPE 3

class VertexLayout {
public:
    VkVertexInputBindingDescription binding;
    std::vector<VkVertexInputAttributeDescription> attributes;
    
    /**
     * @brief La tabla relaciona el tipo de dato (float, int ,etc) con su tamaño en bytes y con su formato vulkan
     * tal que si haces attribsTable[CHAR][0] te devuelve el tamaño en bytes y attribsTable[CHAR][1] VK_FORMAT_R8_SINT y attribsTable[CHAR][2] te da el align del char segundo std140 que es 4 bytes.
     * @note La tabla se inicializa en Pipeline.cpp
     */
    static int attribsTable[INPUT_TYPE_COUNT][DATA_COUNT_FROM_INPUT_TYPE];
    enum INPUT_TYPES {
        FLOAT,
        INT,
        CHAR,
        UINT,
        INT16,
        UINT16,
        UCHAR,
        VEC3,
        VEC4,
        MAT4
    };
    static size_t getSizeFromInputType(INPUT_TYPES input) {
        return attribsTable[input][0];
    }
    static VkFormat getFormatFromInputType(INPUT_TYPES input) {
        return static_cast<VkFormat>(attribsTable[input][1]);
    }
    static size_t getAlignFromInputType(INPUT_TYPES input) {
        return attribsTable[input][2];
    }
       /**
     * @brief A una dada lista de tipos de entrada (FLOAT, INT, VEC3, etc) le devuelve un VertexLayout que contiene el binding y los atributos con sus offsets y formatos.
       el binding contiene apenas informacion del stride encuanto los atributos cada atributo vendria siendo un layout.
       @details Construye los atributos iterando por cada input del vector dado y creando el vKVertexInputAttributeDescription
       obteniendo su formato y su tamaño apartir de la tabla.
     */
    VertexLayout(std::vector<VertexLayout::INPUT_TYPES> inputs);
};
#endif
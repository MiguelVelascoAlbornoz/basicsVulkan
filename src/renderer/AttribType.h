#ifndef ATTRIB_TYPE_H
#define ATTRIB_TYPE_H

#include <vulkan/vulkan_core.h>
#define INPUT_TYPE_COUNT 10

class AttribType {
public:
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

        INPUT_TYPES type;
        size_t size;
        VkFormat format;
        size_t align;
        static AttribType possibleAttribs[INPUT_TYPE_COUNT];

        static AttribType getFormatFromInputType(INPUT_TYPES input);
};

#endif // ATTRIB_TYPE_H
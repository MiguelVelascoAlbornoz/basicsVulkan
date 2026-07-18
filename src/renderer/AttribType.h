#ifndef ATTRIB_TYPE_H
#define ATTRIB_TYPE_H

#include <vulkan/vulkan_core.h>
#define INPUT_TYPE_COUNT 11

class AttribType {
public:
    enum INPUT_TYPES {
        FLOAT,
        INT,
        CHAR,
        UINT,
        INT16,
        UINT16,
        UINT64,
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

        static const AttribType* getAttribFromInputType(INPUT_TYPES input);

        struct SendableField {
          void* const data;
          const AttribType::INPUT_TYPES inputType;
          const int index;
    };
};

#endif // ATTRIB_TYPE_H
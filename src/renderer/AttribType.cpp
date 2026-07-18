#include "AttribType.h"
AttribType AttribType::possibleAttribs[INPUT_TYPE_COUNT] = {
    { .type = AttribType::FLOAT, .size = sizeof(float), .format = VK_FORMAT_R32_SFLOAT, .align = 4 },
    { .type = AttribType::INT, .size = sizeof(int), .format = VK_FORMAT_R32_SINT, .align = 4 },
    { .type = AttribType::CHAR, .size = sizeof(char), .format = VK_FORMAT_R8_SINT, .align = 4 },
    { .type = AttribType::UINT, .size = sizeof(unsigned int), .format = VK_FORMAT_R32_UINT, .align = 4 },
    { .type = AttribType::INT16, .size = sizeof(int16_t), .format = VK_FORMAT_R16_SINT, .align = 4 },
    { .type = AttribType::UINT16, .size = sizeof(uint16_t), .format = VK_FORMAT_R16_UINT, .align = 4 },
    { .type = AttribType::UINT64, .size = sizeof(uint64_t), .format = VK_FORMAT_R64_SINT, .align = 8 },
    { .type = AttribType::UCHAR, .size = sizeof(unsigned char), .format = VK_FORMAT_R8_UINT, .align = 4 },
    { .type = AttribType::VEC3, .size = 3 * sizeof(float), .format = VK_FORMAT_R32G32B32_SFLOAT, .align = 16 },
    { .type = AttribType::VEC4, .size = 4 * sizeof(float), .format = VK_FORMAT_R32G32B32A32_SFLOAT, .align = 16 },
    { .type = AttribType::MAT4, .size = 16 * sizeof(float), .format = VK_FORMAT_R32G32B32A32_SFLOAT, .align = 16 }
};

const AttribType* AttribType::getAttribFromInputType(INPUT_TYPES input)
{
     
        return &AttribType::possibleAttribs[input];
         
}

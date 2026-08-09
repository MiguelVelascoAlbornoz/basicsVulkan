//
// Created by migue on 09/08/2026.
//

#include "PipelineUtils.h"
#include <memory>
#include <iostream>

/**
 * @brief Reads a binary file and stores its contents in a buffer.
 * @param filename The path to the binary file to read.
 * @param buffer
 */
bool PipelineUtils::readFile(const std::string& filename, std::vector<char> &buffer) {
    FILE* file = nullptr;
    file = fopen(filename.c_str(), "rb");
    if (!file) {
        return false;
    }
    char tempByte;
    //Hasta llegar a eof continuar leyendo un byte a la vez y guardarlo en el buffer
    while (fread(&tempByte, sizeof(char), 1, file) == 1) {
        buffer.push_back(tempByte);
    }
    fclose(file);
    return true;

}

std::string PipelineUtils::execCommand(const std::string& cmd) {
    std::vector<char> buffer;
    std::string result;

#ifdef _WIN32
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"), _pclose);
#else
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
#endif

    if (!pipe) throw std::runtime_error("popen() falló");
    char tempByte;
    while (fread(&tempByte, sizeof(char), 1, pipe.get()) == 1) {
        buffer.push_back(tempByte);
    }
    return std::string(buffer.begin(), buffer.end());
}

VkShaderModule PipelineUtils::createShaderModule(const std::vector<char> &code, const VkDevice device)
{

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        std::cout << ("createShaderModule(): No se pudo crear el shader module.") << std::endl;
        return VK_NULL_HANDLE;
    }
    return shaderModule;
}

bool PipelineUtils::loadShader(std::string shaderName, VkShaderModule &shaderModule, const std::string& shaderType,VkDevice device) {

    const std::string shaderPath = std::string("assets/shaders/"+shaderType+"/") + shaderName + "."+shaderType;

    //Compilar el codigo de cada uno
    const std::string shaderPathSPV = std::string("assets/shaders/compilated/") + shaderName + "."+shaderType+".spv";

#ifdef _DEBUG
    std::cout << "(VULKAN) Compilando shader: " << shaderPath << std::endl;
#endif
    const std::string commandOut = PipelineUtils::execCommand(("glslc "+shaderPath+" -o "+shaderPathSPV).c_str());
    std::cout << commandOut << std::endl;

    // if (result) {
    //     std::cout << ("(VULKAN) Error in loadShaders(): Error al ejecutar el compilador del shader: "+std::string(shaderName))+"."+shaderType << std::endl;
    //     return false;
    // }

    std::vector<char> shaderCode;
    if (const bool result = PipelineUtils::readFile(shaderPathSPV, shaderCode); !result) {
        std::cout << ("(VULKAN) Error in loadShaders(): Shader \""+ std::string(shaderName) +"\" no compilado.") << std::endl;
        return false;
    }
    shaderModule = PipelineUtils::createShaderModule(shaderCode,device);

    return true;
}
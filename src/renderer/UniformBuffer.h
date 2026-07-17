

#ifndef UNIFORM_BUFFER_H
#define UNIFORM_BUFFER_H
#include <utility>
#include "VertexLayout.h"
#include "VulkanDevice.h"
#include <glm/glm.hpp>

class UniformBuffer
{
public:

    ~UniformBuffer() {
        vkUnmapMemory(device->device, memory);
        vkDestroyBuffer(device->device, buffer, nullptr);
        vkFreeMemory(device->device, memory, nullptr);
        for (auto field : fields) {
            delete field;
        }
    }
    UniformBuffer(VulkanDevice *device, std::vector< std::pair<void*,AttribType::INPUT_TYPES>> &inputs);

    /** @brief Mete el dado indice de field en el field queue para que sea actualizada en el shader**/
    void addIndexToQueue(int index)  {
        fieldToUpdateQueue.push_back(fields[index]);
    };

    void updateDescriptorSet(VulkanDevice *device, VkDescriptorSet descriptorSet);

    /** @brief Funcion finalmente actualiza los valores en el shader de los fields que esten en el field queue**/
    void clearQueue();

private:
    struct UniformField {
        AttribType::INPUT_TYPES type = AttribType::CHAR;
        size_t offsetInUniform = 0;
        size_t size = 0;
        void* dataPointer = nullptr;
    };
    /** Cada vez que se modifique una variable y se quiera actualizar su valor en el shader tienen que meterse en este vector su devido UniformField, este UniformField deve estar en fields**/
    std::vector<UniformField*> fieldToUpdateQueue;
    /** @details los fields son como cada variable que se manda al shader**/
    std::vector<UniformField*> fields;
    VkBuffer buffer{};
    VkDeviceMemory memory{};
    void* mappedMemory{};
    VulkanDevice* device;
    /** Numero total de bytes del uniform, con paddings incluidos**/
    int bytesCount;
public:
    /**
    * @brief Obtiene el ultimo elemento en el fieldQueue de la camara y lo remueve, caso no alla mas fields en la queue retorna nullptr
    * **/
    [[nodiscard]] const UniformBuffer::UniformField* getAndPopOfQueue();
    void setRaw(size_t index);
    void setRaw(const UniformField* uniformField);

};


#endif // UNIFORM_BUFFER_H

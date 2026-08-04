//
// Created by migue on 03/08/2026.
//

#ifndef BASICSVULKAN_FRAMEBUFFERS_H
#define BASICSVULKAN_FRAMEBUFFERS_H
#include <string>
#include <unordered_map>
#include "../App/Utilitys.h"
#include <vector>

class FrameBufferObject;


#define DEFAULT_FRAME_BUFFER_ID "default_frame_buffer"
class FrameBuffers
{
public:

    static FrameBufferObject* defaultFrameBuffer;

    static FrameBufferObject* registerFrameBuffer(const std::string& id, FrameBufferObject* frameBuffer) {
        return registerObject(id, frameBuffer, frameBuffers);
    };
    static std::unordered_map<std::string, FrameBufferObject*> frameBuffers; /**< @brief Map to store menu rendering functions. */
    static void freeFrameBuffers();

    static void turnOnFBO(FrameBufferObject* fbo);
    static void turnOffFBO(FrameBufferObject* fbo);
    static std::vector<FrameBufferObject*> activeFBOs;

};


#endif //BASICSVULKAN_FRAMEBUFFERS_H

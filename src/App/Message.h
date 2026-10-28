//
// Created by migue on 27/10/2026.
//

#ifndef BASICSVULKAN_MESSAGE_H
#define BASICSVULKAN_MESSAGE_H
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

/** @brief Bytes que ocupa el status al inicio de cada mensaje. */
static constexpr size_t STATUS_HEADER_SIZE = sizeof(int32_t);
enum PackageHeader
{
    CONNECTION_TRY,
    UNEXPECTED_HEADER,
    CONNECTION_ERROR,
    CONNECTION_SUCCESS,
    MESSAGE,
    HEARTBEAT,
    VIDEO_FRAME
};

class MessageView {
public:
    MessageView(const char* buf, size_t n) : data(buf), len(n) {
        if (n < STATUS_HEADER_SIZE) throw std::runtime_error("Message too short");
    }
    PackageHeader getPackageHeader() const {
        int32_t h; memcpy(&h, data, sizeof(h));
        return static_cast<PackageHeader>(h);
    }
    const char* getHeader() const { return data; }
    const char* getPayload() const { return data + STATUS_HEADER_SIZE; }
    size_t getPayloadSize() const { return len - STATUS_HEADER_SIZE; }
    const char* getData() const { return data; }
    size_t getSize() const { return len; }
private:
    const char* data;
    size_t len;
};

class Message {
public:
    Message(PackageHeader h, size_t payloadSize) : buf(STATUS_HEADER_SIZE + payloadSize) {
        int32_t v = h; memcpy(buf.data(), &v, sizeof(v));
    }
    Message(PackageHeader h, std::string_view p) : Message(h, p.size()) {
        memcpy(getPayload(), p.data(), p.size());
    }
    PackageHeader getPackageHeader() const {
        int32_t h; memcpy(&h, buf.data(), sizeof(h));
        return static_cast<PackageHeader>(h);
    }
    char* getPayload()  { return buf.data() + STATUS_HEADER_SIZE; }
    size_t getPayloadSize() const { return buf.size() - STATUS_HEADER_SIZE; }
    char* getHeader()  { return buf.data(); }
    void resizePayload(size_t n) { buf.resize(STATUS_HEADER_SIZE + n); } // sustituye a filledSize
    size_t getSize() const { return buf.size(); }
    const char* getData() const { return buf.data(); }
    MessageView view() const { return MessageView(buf.data(), buf.size()); }
private:
    std::vector<char> buf;
};
#endif

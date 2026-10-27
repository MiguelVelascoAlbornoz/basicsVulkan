//
// Created by migue on 27/10/2026.
//

#ifndef BASICSVULKAN_MESSAGE_H
#define BASICSVULKAN_MESSAGE_H
#include <cstring>
#include <stdexcept>

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

class AbstractMessage
{
public:
    virtual ~AbstractMessage();
    PackageHeader getHeader();
    size_t getSize() {return size;};
    /**
     * @return Devolve o numero de bytes da mensagem sem o header
     */
    size_t getPayloadSize() {return size-STATUS_HEADER_SIZE;};
    static void intToCharArray(int value, char* buffer, size_t offset);
    static int charArrayToInt(const char* buffer, size_t offset);;
protected:
    const size_t size;//Numero de bytes da mensagem (header+payload)
    char* data;
    explicit AbstractMessage(size_t payloadSize) : size(payloadSize + STATUS_HEADER_SIZE) {};
};
class Message : public AbstractMessage
{
public:

    Message(PackageHeader header,size_t payloadSize) : AbstractMessage(payloadSize)
    {
        data = new char[payloadSize + STATUS_HEADER_SIZE];
        intToCharArray(static_cast<int32_t>(header), data, 0);
        filledSize = payloadSize;
    };
    ~Message()
    {
        delete[] data;
    }
    /*
     * Permite de alguma forma manter registo de quantos bytes "uteis" o payload possui
     */
    void setFilledSize(size_t count) {
    if (count > getPayloadSize())
        throw std::length_error("filled bytes count cannot be bigger than payload size");
    filledSize = count;
    };
    /**
     * @return Permite obter o numero de bytes do payload que sao considerados "uteis", o resto de bytes do payload devem ser considerados como trash data
     */
    size_t getFilledSize() {return filledSize;};
    char* getPayload() {return data + STATUS_HEADER_SIZE;};
    char* getData() {return data;};
private:
    //Numero de bytes do payload que podemos considerar "preenchidos"
    //Por default considera-se que o payload inteiro esta preenchido
    size_t filledSize;

};
class MessageView : public AbstractMessage
{
public:

    MessageView(char* buffer, size_t bufferSize,const bool copy = false) : AbstractMessage(bufferSize-STATUS_HEADER_SIZE), copy(copy)
    {
        if (bufferSize < STATUS_HEADER_SIZE)
        {
            throw std::runtime_error("Message size not enough.");
        }


        data = buffer;
        if (copy)
        {
            data = new char[size];
            memcpy(data, buffer, size);
        }
    };
    ~MessageView() { if (copy) delete[] data; }

    const char* getPayload() {return data + STATUS_HEADER_SIZE;};
    const char* getData() {return data;};

private:
    const bool copy;
};

#endif //BASICSVULKAN_MESSAGE_H

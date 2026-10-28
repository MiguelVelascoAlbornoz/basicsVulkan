//
// Created by migue on 27/10/2026.
//

#ifndef BASICSVULKAN_MESSAGE_H
#define BASICSVULKAN_MESSAGE_H
#include <cstdint>
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
    PackageHeader getPackageHeader() const;
    size_t getSize() const {return size;};
    /**
     * @return Devolve o numero de bytes da mensagem sem o header
     */
    size_t getPayloadSize() const {return size-STATUS_HEADER_SIZE;};
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
    /*
     * @brief altamente ineficiente mas pratico
     */
    Message(PackageHeader header,std::string& payload) : AbstractMessage(payload.size())
    {
        data = new char[payload.size() + STATUS_HEADER_SIZE];
        intToCharArray(static_cast<int32_t>(header), data, 0);
        memcpy(getPayload(), payload.data(), payload.size());
        filledSize = payload.size();
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
    size_t getFilledSize() const {return filledSize;};
    char* getPayload() const {return data + STATUS_HEADER_SIZE;};
    char* getHeader() const {return data;};
    char* getData() const {return data;};
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
    /*
     * @brief Cria so um messageview vazio
     */
    MessageView() : AbstractMessage(0), copy(true)
    {
        data = new char[STATUS_HEADER_SIZE];
        intToCharArray(static_cast<int32_t>(UNEXPECTED_HEADER), data, 0);
    };
    ~MessageView() { if (copy) delete[] data; }
    const char* getHeader() const {return data;};
    const char* getPayload() const {return data + STATUS_HEADER_SIZE;};
    const char* getData() const {return data;};

private:
    const bool copy;
};

#endif //BASICSVULKAN_MESSAGE_H

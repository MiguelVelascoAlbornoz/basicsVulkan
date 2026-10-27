//
// Created by migue on 31/08/2026.
//

#ifndef BASICSVULKAN_NETMANAGER_H
#define BASICSVULKAN_NETMANAGER_H
#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>
#include <sodium/sodium.h>
#include <WinSock2.h>
#include <chrono>
#include <thread>
#include <ws2ipdef.h>

#include "Message.h"
#define MAX_UDP_RECEIVE_BUFFER_SIZE 65536
#define WRONG_PASSWORD "Wrong password."
class NetManager
{


public:

    enum ConexionStatus
    {
        UNEXPECTED_ERROR,
        INVALID_IP,
        WAITING,
        UNDEFINED,
        CONNECTING,
        CONNECTED,
        SHUTTING_DOWN,
        TIMEOUT,
        INVALID_PASSWORD
    };

    static std::string obtainPublicIP();
    static std::vector<std::string> obtainAllPrivateIPs();
    static void assignIntToCharArray(int32_t value, char* buffer, size_t offset);
    static int32_t charArrayToInt(const char* buffer, size_t offset);
    static std::vector<char> buildMessage(PackageHeader status, const std::string& payload);
    static std::vector<char> buildMessage(PackageHeader header, const char* payload, int bytesCount);
    static PackageHeader extractHeader(const char* buffer, int len);
    static const char* extractPayload(const char* buffer, int len);
    Message decryptMessage(MessageView& cryptedMessage);

    bool initServer();
    bool initClient();
    ~NetManager();

private:
    std::vector<char> buildEncryptedMessage(PackageHeader header, const std::string& payload);

    void handleIncomingPacket();
    void sendPackage(const std::string& message, PackageHeader header);
    /**
     *
     * @param messageIn
     * @param pkS
     * @param tag2
     * @param skS
     * @param senderAddr
     * @return true if password is correct and was possible to generate skS
     * @brief If password is not correct this function answers to the sender
     */
    bool tryAuth(MessageView& messageIn, unsigned char* pkS, unsigned char* tag2, unsigned char* skS, sockaddr_in
                 & senderAddr);
    void sendHeartbeat();

    void sendFrame();
    void serverWaitForConnection();
    void startClient();
    [[nodiscard]] int getHostPort() const {return hostPort;};
    std::string getPublicIP() { return publicIP; };
    std::vector<std::string> getPrivateIPs() { return privateIps; };
    [[nodiscard]] ConexionStatus getStatus() const {return status;};
    std::mutex mutex;
    std::condition_variable cv;
    bool shoulTryConnection = false;
    void tryConnection(const std::string& ip, int port, const std::string& password);
    bool manageHeartBeat();
    std::string getConnectionIP() { return connectionIP; };
    sockaddr_in connectionAddr;
    bool init();
    unsigned char pk[crypto_box_PUBLICKEYBYTES]; // 32 bytes
    unsigned char sk[crypto_box_SECRETKEYBYTES]; // 32 bytes

    int heartbeatInterval = 10;
    int heartbeatsMaxTrys = 2;
    int heartbeatsTry = 0;

    std::chrono::steady_clock::time_point lastHeartBeatSendTime;
    bool waitingHeartbeat = false;
    ConexionStatus status = UNDEFINED;
    void serverThread();
    void clientWaitForConnection();
    void clientThread();
    std::string connectionPassword;
    std::vector<std::string> privateIps;
    std::thread redThread;
    std::string publicIP;
    std::string connectionIP;
    SOCKET udpSocket = 0;
    int hostPort = 9000;
    unsigned char rxKey[crypto_kx_SESSIONKEYBYTES]{}; // clave para descifrar lo que recibo
    unsigned char txKey[crypto_kx_SESSIONKEYBYTES]{}; // clave para cifrar lo que envío
    uint64_t sendSeq = 0;      // contador de secuencia propio, para el nonce al enviar
    uint64_t lastRecvSeq = 0;  // último seq aceptado, para rechazar repetidos/viejos
};


#endif //BASICSVULKAN_NETMANAGER_H

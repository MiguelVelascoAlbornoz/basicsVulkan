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
    bool sendMessage(const Message& m, const sockaddr_in& addr);


    bool initServer();
    bool initClient();
    ~NetManager();

private:
    struct HandshakeState {
        unsigned char pk[crypto_box_PUBLICKEYBYTES];
        unsigned char sk[crypto_box_SECRETKEYBYTES];
        unsigned char passwordKey[crypto_generichash_KEYBYTES];

        ~HandshakeState() {
            sodium_memzero(sk, sizeof(sk));
            sodium_memzero(passwordKey, sizeof(passwordKey));
        }
    };
    struct ConnectParams { std::string ip; uint16_t port; std::string password; };

    /*
     * Esta função bloquea el thread hasta que el cliente ponga la IP, puerto y contraseña
     */
    MessageView waitForServerConnectRequest(sockaddr_in& senderAddr);
    Message decryptMessage(const MessageView& cryptedMessage);
    Message encryptMessage(const Message& message);// 1. espera al cliente y copia los datos

    /**
     *
     * @param messageIn
     * @param pkS
     * @param tag2
     * @param skS
     * @param senderAddr
     * @return
     * CONNECTED if everything ok
     * INVALID_PASSWORD wrong password
     * UNEXPECTED_ERROR unexpected error
     * @brief If password is not correct this function answers to the sender
     */
    ConexionStatus tryAuth(MessageView& messageIn, unsigned char* pkS, unsigned char* tag2, unsigned char* skS, sockaddr_in& senderAddr);
    ConexionStatus sendHandShakeResponse(MessageView& messageIn, sockaddr_in& clientAddr);
    ConexionStatus receiveHandshakeResponse(const sockaddr_in& server, char* buf, int& n); // 3. recvfrom + validar origen
    ConexionStatus processHandshakeResponse(const MessageView& handShakeResponse, const sockaddr_in& server, HandshakeState& hs); // 4. validar tag, derivar claves
    ConexionStatus status = UNDEFINED;


    void sendHeartbeat();
    void sendFrame();
    void serverWaitForConnection();
    void tryConnection(const std::string& ip, int port, const std::string& password);
    void serverThread();
    void clientWaitForConnection();
    void clientThread();
    void connectionLoop();
    void waitForClientConnectRequest();
    void handleIncomingPacket();
    void sendPackage(const std::string& message, PackageHeader header);

    int hostPort = 9000;
    int heartbeatInterval = 10;
    int heartbeatsMaxTrys = 2;
    int heartbeatsTry = 0;

    bool shoulTryConnection = false;

    bool waitingHeartbeat = false;
    bool initHandShake(sockaddr_in& destAddr);
    bool initHandShake(sockaddr_in& destAddr, HandshakeState& hs);                             // 2. ya la tienes (envía CONNECTION_TRY)
    bool sendMessage(Message& message);
    bool manageHeartBeat();
    bool init();

    std::string getConnectionIP() { return connectionIP; };
    std::string getPublicIP() { return publicIP; };
    std::string publicIP;
    std::string connectionIP;
    std::string connectionPassword;
    [[nodiscard]] int getHostPort() const {return hostPort;};
    [[nodiscard]] ConexionStatus getStatus() const {return status;};

    std::vector<std::string> getPrivateIPs() { return privateIps; };
    std::vector<std::string> privateIps;

    std::mutex mutex;
    std::condition_variable cv;
    std::thread redThread;

    sockaddr_in connectionAddr;

    std::chrono::steady_clock::time_point lastHeartBeatSendTime;

    SOCKET udpSocket = 0;

    unsigned char rxKey[crypto_kx_SESSIONKEYBYTES]{}; // clave para descifrar lo que recibo
    unsigned char txKey[crypto_kx_SESSIONKEYBYTES]{}; // clave para cifrar lo que envío
    uint64_t sendSeq = 0;      // contador de secuencia propio, para el nonce al enviar
    uint64_t lastRecvSeq = 0;  // último seq aceptado, para rechazar repetidos/viejos
};


#endif //BASICSVULKAN_NETMANAGER_H

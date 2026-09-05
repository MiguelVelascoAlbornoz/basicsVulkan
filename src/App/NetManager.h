//
// Created by migue on 31/08/2026.
//

#ifndef BASICSVULKAN_NETMANAGER_H
#define BASICSVULKAN_NETMANAGER_H
#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>
#include <WinSock2.h>

#include <thread>

#define MAX_UDP_RECEIVE_BUFFER_SIZE 65536
#define WRONG_PASSWORD "Wrong password."
class NetManager
{


public:
    /** @brief Bytes que ocupa el status al inicio de cada mensaje. */
    static constexpr size_t STATUS_HEADER_SIZE = sizeof(int32_t);
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
    enum PackageHeader
    {
        CONNECTION_TRY,
        UNEXPECTED_HEADER,
        CONNECTION_ERROR,
        CONNECTION_SUCCESS,
        MESSAGE,
        HEARTBEAT,
    };
    static std::string obtainPublicIP();
    static std::vector<std::string> obtainAllPrivateIPs();
    static void assignIntToCharArray(int32_t value, char* buffer, size_t offset);
    static int32_t charArrayToInt(const char* buffer, size_t offset);
    static std::vector<char> buildMessage(PackageHeader status, const std::string& payload);
    static std::vector<char> buildMessage(PackageHeader header, const char* payload, int bytesCount);
    static PackageHeader extractHeader(const char* buffer, int len);
    static std::string extractPayload(const char* buffer, int len);
    void handleIncomingPacket() const;
    void sendPackage(const std::string& message, PackageHeader header);
    void sendHeartbeat();
    bool init();
    bool initServer();
    bool initClient();
    ~NetManager();
    void sendFrame();
    void startClient();
    [[nodiscard]] int getHostPort() const {return hostPort;};
    std::string getPublicIP() { return publicIP; };
    std::vector<std::string> getPrivateIPs() { return privateIps; };
    [[nodiscard]] ConexionStatus getStatus() const {return status;};
    std::mutex mutex;
    std::condition_variable cv;
    bool shoulTryConnection = false;
    void tryConnection(const std::string& ip, int port, const std::string& password);
    sockaddr_in connectionAddr;
private:
    ConexionStatus status = UNDEFINED;
    void serverThread();
    void clientThread();
    std::string connectionPassword = "@Milasco13";
    std::vector<std::string> privateIps;
    std::thread redThread;
    std::string publicIP;
    std::string connectionIP;
    SOCKET udpSocket = 0;
    int hostPort = 9000;

};


#endif //BASICSVULKAN_NETMANAGER_H

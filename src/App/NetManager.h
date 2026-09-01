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

class NetManager
{


public:
    enum ConexionStatus
    {
        UNEXPECTED_ERROR,
        INVALID_IP,
        WAITING,
        UNDEFINED,
    };
    static std::string obtainPublicIP();
    static std::vector<std::string> obtainAllPrivateIPs();

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
    void tryConnection(std::string ip, int port, const std::string& password);
private:
    ConexionStatus status = UNDEFINED;
    void serverThread();
    void clientThread();
    std::string connectionPassword;
    std::vector<std::string> privateIps;
    std::thread redThread;
    std::string publicIP;
    std::string connectionIP;
    SOCKET udpSocket = 0;
    int hostPort = 9000;

};


#endif //BASICSVULKAN_NETMANAGER_H

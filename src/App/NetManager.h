//
// Created by migue on 31/08/2026.
//

#ifndef BASICSVULKAN_NETMANAGER_H
#define BASICSVULKAN_NETMANAGER_H
#include <string>
#include <vector>
#include <WinSock2.h>

#include <thread>

class NetManager
{


public:
    enum ConexionStatus
    {
        WAITING,
        UNDEFINED,
    };
    static std::string obtainPublicIP();
    static std::vector<std::string> obtainAllPrivateIPs();

    bool init();
    ~NetManager();
    void sendFrame();
    void startClient();
    [[nodiscard]] int getHostPort() const {return hostPort;};
    std::string getPublicIP() { return publicIP; };
    std::vector<std::string> getPrivateIPs() { return privateIps; };
    [[nodiscard]] ConexionStatus getStatus() const {return status;};
private:
    ConexionStatus status = UNDEFINED;
    void serverThread();
    std::vector<std::string> privateIps;
    std::thread redThread;
    std::string publicIP;
    SOCKET udpSocket = 0;
    int hostPort = 9000;

};


#endif //BASICSVULKAN_NETMANAGER_H

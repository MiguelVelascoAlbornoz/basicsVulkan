//
// Created by migue on 31/08/2026.
//

#include "NetManager.h"

#include <iostream>
#include <iphlpapi.h>
#include <winhttp.h>
#include <Ws2tcpip.h>

std::string NetManager::obtainPublicIP() {
    std::string result;
    HINTERNET hSession = WinHttpOpen(L"MyApp/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return result;

    HINTERNET hConnect = WinHttpConnect(hSession, L"api.ipify.org",
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return result; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/",
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return result; }

    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, nullptr)) {


        do {
            DWORD available = 0;
            WinHttpQueryDataAvailable(hRequest, &available);
            if (available == 0) break;

            std::vector<char> buffer(available + 1, 0);
            DWORD downloaded = 0;
            WinHttpReadData(hRequest, buffer.data(), available, &downloaded);
            result.append(buffer.data(), downloaded);
        } while (true);
        }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return result;
}


 std::vector<std::string> NetManager::obtainAllPrivateIPs() {
    std::vector<std::string> ips;
    ULONG bufferSize = 15000;
    std::vector<char> buffer(bufferSize);
    auto* addresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());

    const ULONG result = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
                                         nullptr, addresses, &bufferSize);
    if (result != NO_ERROR) return ips;

    for (const auto* adapter = addresses; adapter != nullptr; adapter = adapter->Next) {
        if (adapter->OperStatus != IfOperStatusUp) continue; // solo adaptadores activos

        for (const auto* unicast = adapter->FirstUnicastAddress; unicast != nullptr; unicast = unicast->Next) {
            const auto* addr = reinterpret_cast<sockaddr_in*>(unicast->Address.lpSockaddr);
            char ipStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr->sin_addr, ipStr, sizeof(ipStr));
            ips.emplace_back(ipStr);
        }
    }
    return ips;
}

void NetManager::sendFrame()
{
    // Datos a enviar
    const char* message = "Hola mundo";
    int messageLen = static_cast<int>(strlen(message));

    // Dirección de destino
    sockaddr_in destAddr{};
    destAddr.sin_family = AF_INET;
    destAddr.sin_port   = htons(hostPort);                     // puerto destino
    inet_pton(AF_INET, "192.168.1.146", &destAddr.sin_addr); // IP destino

    int bytesSent = sendto(
        udpSocket,
        message, messageLen,
        0,                                   // flags
        (sockaddr*)&destAddr, sizeof(destAddr)
    );

    if (bytesSent == SOCKET_ERROR) {
        std::cerr << "sendto() falló: " << WSAGetLastError() << std::endl;
    } else {
        // bytesSent == messageLen si se envió completo (UDP no fragmenta la llamada)
    }
}
void NetManager::startClient()
{


}

void NetManager::serverThread()
{

    //Lo primero que hace el server es esperar un intento de conexxion, la conexion solo se va a considerar valida si el mensaje de conexion es la contraseña, ahi se guarda la IP de quien se conecto y se entra en modo
    //envio de frame
    char buffer[65536]; // suficiente para el datagrama UDP más grande posible
    sockaddr_in senderAddr{};
    int senderAddrLen = sizeof(senderAddr);
    status = WAITING;
    //Espera hasta recibir la conexion
    int bytesReceived = recvfrom(
        udpSocket,
        buffer, sizeof(buffer),
        0,                                  // flags
        (sockaddr*)&senderAddr, &senderAddrLen  // te dice quién lo mandó
    );

    if (bytesReceived == SOCKET_ERROR) {
        std::cerr << "recvfrom() falló: " << WSAGetLastError() << std::endl;
    } else {

        char senderIP[INET_ADDRSTRLEN];

        inet_ntop(
            AF_INET,
            &senderAddr.sin_addr,
            senderIP,
            INET_ADDRSTRLEN
        );
        std::cout << "Conexion realizada con: " << senderIP << std::endl;
        std::cout << "Mensaje enviado: " << std::endl;
        for (int i = 0; i <bytesReceived; i++)
        {
            std::cout << buffer[i];
        }
        std::cout << std::endl;
    }
}

bool NetManager::init()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);

    // TCP: SOCK_STREAM | UDP: SOCK_DGRAM
    udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;  // escuchar en todas las interfaces
    serverAddr.sin_port = htons(hostPort);         // puerto elegido

    if (bind(udpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "bind() falló: " << WSAGetLastError() << std::endl;
        closesocket(udpSocket);
        udpSocket = INVALID_SOCKET;
        WSACleanup();
        return false;
    }
    privateIps = obtainAllPrivateIPs();
    publicIP = obtainPublicIP();

    redThread = std::thread(&NetManager::serverThread, this);
    return true;
}
NetManager::~NetManager()
{
    // Cerrar el socket si es válido
    if (udpSocket != INVALID_SOCKET) {
        closesocket(udpSocket);
        udpSocket = INVALID_SOCKET;
    }
    redThread.join();
    // Liberar los recursos de Winsock
    WSACleanup();
}

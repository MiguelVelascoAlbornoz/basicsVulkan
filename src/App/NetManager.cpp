//
// Created by migue on 31/08/2026.
//

#include "NetManager.h"

#include <cstring>
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
void NetManager::assignIntToCharArray(int32_t value, char* buffer, size_t offset)
{
    // htonl para que el formato en el cable sea el mismo sin importar el
    // endianness de cada máquina (importante en una conexión entre dos PCs distintos).
    const uint32_t netValue = htonl(static_cast<uint32_t>(value));
    memcpy(buffer + offset, &netValue, sizeof(netValue));
}

int32_t NetManager::charArrayToInt(const char* buffer, size_t offset)
{
    uint32_t netValue;
    memcpy(&netValue, buffer + offset, sizeof(netValue));
    return static_cast<int32_t>(ntohl(netValue));
}


std::vector<char> NetManager::buildMessage(PackageHeader header, const std::string& payload)
{
    std::vector<char> message(STATUS_HEADER_SIZE + payload.size());
    assignIntToCharArray(static_cast<int32_t>(header), message.data(), 0);
    memcpy(message.data() + STATUS_HEADER_SIZE, payload.data(), payload.size());
    return message;
}
std::vector<char> NetManager::buildMessage(PackageHeader header, const char* payload, int bytesCount)
{
    std::vector<char> message(STATUS_HEADER_SIZE + bytesCount);
    assignIntToCharArray(static_cast<int32_t>(header), message.data(), 0);
    memcpy(message.data() + STATUS_HEADER_SIZE, payload, bytesCount);
    return message;
}
NetManager::PackageHeader NetManager::extractHeader(const char* buffer, int len)
{
    if (len < static_cast<int>(STATUS_HEADER_SIZE)) {
        return UNEXPECTED_HEADER;
    }
    return static_cast<PackageHeader>(charArrayToInt(buffer, 0));
}

std::string NetManager::extractPayload(const char* buffer, int len)
{
    if (len <= static_cast<int>(STATUS_HEADER_SIZE)) {
        return {};
    }
    return { buffer + STATUS_HEADER_SIZE, static_cast<size_t>(len) - STATUS_HEADER_SIZE };
}

void NetManager::handleIncomingPacket() const
{

    char buffer[MAX_UDP_RECEIVE_BUFFER_SIZE];
    sockaddr_in replyAddr{};
    int replyAddrLen = sizeof(replyAddr);
    int bytesReceived = recvfrom(udpSocket, buffer, sizeof(buffer), 0,
                                  reinterpret_cast<sockaddr*>(&replyAddr), &replyAddrLen);

    //Validar si la respuesta se recibio
    if (bytesReceived == SOCKET_ERROR)
    {
        return;
    }
    if (replyAddr.sin_addr.s_addr != connectionAddr.sin_addr.s_addr ||
        replyAddr.sin_port != connectionAddr.sin_port) {
        return; // respuesta de origen no esperado, descartar
        }
    if (bytesReceived < MAX_UDP_RECEIVE_BUFFER_SIZE)
    {
        buffer[bytesReceived] = '\0';
    } else
    {
        std::cout << "<Mensaje truncado>" << std::endl;
        buffer[MAX_UDP_RECEIVE_BUFFER_SIZE-1] = '\0';
    }
    //PackageHeader packageHeader = extractHeader(buffer, bytesReceived);
    std::string payload = extractPayload(buffer, bytesReceived);
    std::cout << connectionIP + ": " << payload << std::endl;

}

void NetManager::sendPackage(const std::string& message, PackageHeader header)
{
    bool result = false;
    int trys = 0;
    int maxTrys = 5;

    const std::vector<char> m = buildMessage(header, message);
    do
    {
        const int bytesSent = sendto(udpSocket,m.data(), m.size(),0,reinterpret_cast<sockaddr*>(&connectionAddr), sizeof(connectionAddr));

        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "sendto() falló: " << WSAGetLastError() << std::endl;
        } else if (bytesSent != static_cast<int>(m.size())) {
            std::cout << "Message has been fragmented." << std::endl;
        } else {
            result = true; // envío exitoso a nivel de socket
        }
        trys++;
    } while (!result  && trys < maxTrys);
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


void NetManager::tryConnection(const std::string& ip, const int port, const std::string& password)
{
    sockaddr_in addr{};
    int result = inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    if (result == 1)
    {
        connectionIP = ip;
        hostPort = port;
        connectionPassword = password;
        {
            std::lock_guard lock(mutex);
            shoulTryConnection = true;
        }

        cv.notify_one();
    }
    else if (result == 0)
    {
        status = INVALID_IP;
    }
    else
    {
        status = UNEXPECTED_ERROR;
    }
}

void NetManager::serverThread()
{
    while (status != CONNECTED && status != SHUTTING_DOWN)
    {
        //Esperar por una conexion
        char buffer[MAX_UDP_RECEIVE_BUFFER_SIZE]; // suficiente para el datagrama UDP más grande posible
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

        //Una vez recibida valida si fue exitosa
        if (bytesReceived == SOCKET_ERROR || bytesReceived >= MAX_UDP_RECEIVE_BUFFER_SIZE) {
            std::cerr << "recvfrom() falló: " << WSAGetLastError() << std::endl;
            continue;
        }

        //Transformar el mensaje recibido en string
        PackageHeader clientStatus = extractHeader(buffer, bytesReceived);
        if (clientStatus != PackageHeader::CONNECTION_TRY) continue;
        std::string payload = extractPayload(buffer, bytesReceived);
        //Comparar si el mensaje es igual a la contraseña
        //Si lo es entonces el status pasa a ser conected y se guarda la IP del sender
        //Si no hay que volveral status de WAITING y a la situacion inicial
        if (strcmp(connectionPassword.c_str(), payload.c_str()) != 0){
            //Enviar mensaje diciendo que la contraseña es invalida
            std::vector<char> message = buildMessage(CONNECTION_ERROR, "Wrong password.");
            sendto(udpSocket,message.data(), message.size(),0, (sockaddr*)&senderAddr, sizeof(senderAddr));
            continue;
        }
        //Obtener la IP de quien sea que se conecto
        char senderIP[INET_ADDRSTRLEN];

        inet_ntop(
        AF_INET,
        &senderAddr.sin_addr,
        senderIP,
        INET_ADDRSTRLEN);


        std::vector<char> message = buildMessage(CONNECTION_SUCCESS, "You have been connected.");
        //Enviar mensaje diciendo que se permite la conexion
        int bytesSent = sendto(udpSocket,message.data(), message.size(),0, (sockaddr*)&senderAddr, sizeof(senderAddr));
        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "sendto() falló: " << WSAGetLastError() << std::endl;
            status = UNEXPECTED_ERROR;
            continue;
        }
        status = CONNECTED;
        connectionIP = senderIP;
        connectionAddr = senderAddr;
    }
    // Esperar datos en el socket, pero como máximo X ms
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(udpSocket, &readfds);
    timeval tv{ 0, 100 }; // 100ms

    int result = select(0, &readfds, nullptr, nullptr, &tv);

    if (result > 0 && FD_ISSET(udpSocket, &readfds)) {
        // Llegó algo -> recvfrom() ya no bloquea, procesar mensaje
        handleIncomingPacket();
    }
    while (status == CONNECTED)
    {
        // Esperar datos en el socket, pero como máximo X ms
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(udpSocket, &readfds);
        timeval tv{ 0, 10000 }; // 100ms

        int result = select(0, &readfds, nullptr, nullptr, &tv);

        if (result > 0 && FD_ISSET(udpSocket, &readfds)) {
            // Llegó algo -> recvfrom() ya no bloquea, procesar mensaje
            handleIncomingPacket();
        }
        sendPackage("HOLAA",MESSAGE);
    }
}


void NetManager::clientThread()
{
    while (status != CONNECTED && status != SHUTTING_DOWN)
    {
        //Esperar a que el cliente ponga la IP, puerto y contraseña
        shoulTryConnection = false;
        std::unique_lock lock(mutex);

        cv.wait(lock, [this] {
            return shoulTryConnection;
        });
        // Despierta cuando el cliente meta en "conectar"

        // Manda el mensaje a la IP dada
        status = CONNECTING;

        std::vector<char> message = buildMessage(CONNECTION_TRY, connectionPassword);
        // Dirección de destino
        sockaddr_in destAddr{};
        destAddr.sin_family = AF_INET;
        destAddr.sin_port   = htons(hostPort);                     // puerto destino
        inet_pton(AF_INET, connectionIP.c_str(), &destAddr.sin_addr); // IP destino

        const int bytesSent = sendto(udpSocket,message.data(), message.size(),0,(sockaddr*)&destAddr, sizeof(destAddr));

        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "sendto() falló: " << WSAGetLastError() << std::endl;
            status = UNEXPECTED_ERROR;
            continue;
        }
        // bytesSent == messageLen si se envió completo (UDP no fragmenta la llamada)
        if (bytesSent != static_cast<int>(message.size())){
            std::cout << "Message has been fragmented." << std::endl;
            status = UNEXPECTED_ERROR;
            continue;
        }

        //Espera por una respuesta del server
        DWORD timeout = 10000; // 1000 ms = 1 segundo

        setsockopt(
            udpSocket,
            SOL_SOCKET,
            SO_RCVTIMEO,
            (char*)&timeout,
            sizeof(timeout)
        );
        sockaddr_in serverAddr = destAddr;
        char buffer[65536];
        sockaddr_in replyAddr{};
        int replyAddrLen = sizeof(replyAddr);
        int bytesReceived = recvfrom(udpSocket, buffer, sizeof(buffer), 0,
                                      (sockaddr*)&replyAddr, &replyAddrLen);


        //Validar si la respuesta se recibio
        if (bytesReceived == SOCKET_ERROR)
        {
            int error = WSAGetLastError();

            if (error == WSAETIMEDOUT)
            {
                status = TIMEOUT;
            } else
            {
                status = UNEXPECTED_ERROR;
            }
            continue;
        }
        if (replyAddr.sin_addr.s_addr != serverAddr.sin_addr.s_addr ||
            replyAddr.sin_port != serverAddr.sin_port) {
            status = UNEXPECTED_ERROR;
            continue; // respuesta de origen no esperado, descartar
        }
        //Validar contraseña

        PackageHeader packageHeader = extractHeader(buffer, bytesReceived);
        std::string payload = extractPayload(buffer, bytesReceived);
        if (packageHeader != CONNECTION_SUCCESS)
        {
            if (payload == "Wrong password.")
            {
                status = INVALID_PASSWORD;
            } else
            {
                status = UNEXPECTED_ERROR;
            }
        } else
        {
            if (payload == "You have been connected.")
            {
                status = CONNECTED;
                connectionAddr = serverAddr;
            } else
            {
                status = UNEXPECTED_ERROR;
            }

        }

    }
    while (status == CONNECTED)
    {
        // Esperar datos en el socket, pero como máximo X ms
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(udpSocket, &readfds);
        timeval tv{ 0, 100 }; // 100ms

        int result = select(0, &readfds, nullptr, nullptr, &tv);

        if (result > 0 && FD_ISSET(udpSocket, &readfds)) {
            // Llegó algo -> recvfrom() ya no bloquea, procesar mensaje
            handleIncomingPacket();
        }

    }
}

void NetManager::startClient()
{
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

    return true;
}
bool NetManager::initServer()
{
    bool success = init();
    redThread = std::thread(&NetManager::serverThread, this);
    return success;
}
bool NetManager::initClient()
{
    bool success = init();
    redThread = std::thread(&NetManager::clientThread, this);
    return success;
}
NetManager::~NetManager()
{
    status = SHUTTING_DOWN;
    // Cerrar el socket si es válido
    if (udpSocket != INVALID_SOCKET) {
        closesocket(udpSocket);
        udpSocket = INVALID_SOCKET;
    }

    redThread.join();
    // Liberar los recursos de Winsock
    WSACleanup();
}

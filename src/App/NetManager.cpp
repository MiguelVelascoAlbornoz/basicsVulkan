//
// Created by migue on 31/08/2026.
//

#include "NetManager.h"

#include <cstring>
#include <future>
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
bool NetManager::decryptMessage(const char* buffer, int len, PackageHeader& outHeader, std::string& outPayload)
{
    const size_t minSize = STATUS_HEADER_SIZE + crypto_aead_xchacha20poly1305_ietf_NPUBBYTES + crypto_aead_xchacha20poly1305_ietf_ABYTES;
    if (static_cast<size_t>(len) < minSize) {
        return false; // paquete demasiado corto para ser válido
    }

    outHeader = static_cast<PackageHeader>(charArrayToInt(buffer, 0));

    const auto* nonce = reinterpret_cast<const unsigned char*>(buffer + STATUS_HEADER_SIZE);
    const auto* ciphertext = reinterpret_cast<const unsigned char*>(buffer)
                              + STATUS_HEADER_SIZE + crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
    size_t ciphertextLen = len - STATUS_HEADER_SIZE - crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;

    // Rechazar paquetes repetidos o viejos (replay protection)
    uint64_t recvSeqBigEndian;
    memcpy(&recvSeqBigEndian, nonce, sizeof(recvSeqBigEndian));
    uint64_t recvSeq = _byteswap_uint64(recvSeqBigEndian);
    if (recvSeq <= lastRecvSeq) {
        std::cerr << "Paquete repetido o fuera de orden, descartado (seq=" << recvSeq << ")" << std::endl;
        return false;
    }

    std::vector<unsigned char> decrypted(ciphertextLen); // sobra espacio (el tag ocupa parte), no importa
    unsigned long long decryptedLen = 0;

    if (crypto_aead_xchacha20poly1305_ietf_decrypt(
            decrypted.data(), &decryptedLen, nullptr,
            ciphertext, ciphertextLen,
            reinterpret_cast<const unsigned char*>(buffer), STATUS_HEADER_SIZE, // AAD
            nonce, rxKey) != 0)
    {
        std::cerr << "Fallo de autenticación: paquete corrupto o manipulado." << std::endl;
        return false;
    }

    lastRecvSeq = recvSeq;
    outPayload.assign(reinterpret_cast<char*>(decrypted.data()), decryptedLen);
    return true;
}
std::vector<char> NetManager::buildEncryptedMessage(PackageHeader header, const std::string& payload)
{
    // 1. Header en claro (va a ser el AAD, autenticado pero no cifrado)
    char headerBytes[STATUS_HEADER_SIZE];
    assignIntToCharArray(static_cast<int32_t>(header), headerBytes, 0);

    // 2. Nonce = sendSeq expandido a 24 bytes (los primeros 8 bytes llevan el contador, el resto en 0)
    unsigned char nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES] = {0};
    uint64_t seqBigEndian = _byteswap_uint64(sendSeq); // MinGW/Windows: usa esto en vez de htobe64
    memcpy(nonce, &seqBigEndian, sizeof(seqBigEndian));

    // 3. Encriptar
    std::vector<unsigned char> ciphertext(payload.size() + crypto_aead_xchacha20poly1305_ietf_ABYTES);
    unsigned long long ciphertextLen = 0;

    crypto_aead_xchacha20poly1305_ietf_encrypt(
        ciphertext.data(), &ciphertextLen,
        reinterpret_cast<const unsigned char*>(payload.data()), payload.size(),
        reinterpret_cast<const unsigned char*>(headerBytes), STATUS_HEADER_SIZE, // AAD
        nullptr, nonce, txKey);

    sendSeq++; // avanzar el contador, SIEMPRE, incluso si algo falla después

    // 4. Armar el paquete final: header + nonce + ciphertext
    std::vector<char> message;
    message.reserve(STATUS_HEADER_SIZE + sizeof(nonce) + ciphertextLen);
    message.insert(message.end(), headerBytes, headerBytes + STATUS_HEADER_SIZE);
    message.insert(message.end(), reinterpret_cast<char*>(nonce), reinterpret_cast<char*>(nonce) + sizeof(nonce));
    message.insert(message.end(), reinterpret_cast<char*>(ciphertext.data()), reinterpret_cast<char*>(ciphertext.data()) + ciphertextLen);
    return message;
}
std::string NetManager::extractPayload(const char* buffer, int len)
{
    if (len <= static_cast<int>(STATUS_HEADER_SIZE)) {
        return {};
    }
    return { buffer + STATUS_HEADER_SIZE, static_cast<size_t>(len) - STATUS_HEADER_SIZE };
}

void NetManager::handleIncomingPacket()
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
        //Obtener la IP de quien sea que se conecto
        char senderIP[INET_ADDRSTRLEN];

        inet_ntop(
        AF_INET,
        &replyAddr.sin_addr,
        senderIP,
        INET_ADDRSTRLEN);
        std::cout << "Package from unknown sender: " <<  senderIP << std::endl;
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
    PackageHeader packageHeader;
    std::string payload;
    if (!decryptMessage(buffer, bytesReceived, packageHeader, payload)) {
        std::cout << "Invalid package recivec"<< std::endl;
        return; // paquete inválido, corrupto, repetido o manipulado — se descarta silenciosamente
    }
    if (packageHeader == HEARTBEAT)
    {
        if (strcmp(payload.c_str(), "START") == 0)
        {
            sendPackage("ANSWER",HEARTBEAT);
        } else
        {
            waitingHeartbeat = false;
            auto ping = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()- lastHeartBeatSendTime);
            std::cout << "ping: " << ping.count() << " ms" << std::endl;
        }

    } else if (packageHeader == MESSAGE)
    {
        std::cout << connectionIP + ": " << payload << std::endl;
    }

}

void NetManager::sendPackage(const std::string& message, PackageHeader header)
{
    bool result = false;
    int trys = 0;
    int maxTrys = 5;

    const std::vector<char> m = buildEncryptedMessage(header, message);
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
void NetManager::serverWaitForConnection()
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


        unsigned char passwordKey[crypto_generichash_KEYBYTES];
        crypto_generichash(passwordKey, sizeof(passwordKey),
                            reinterpret_cast<const unsigned char*>(connectionPassword.data()), connectionPassword.size(),
                            nullptr, 0);

        std::string payload = extractPayload(buffer, bytesReceived);
        if (payload.size() != crypto_box_PUBLICKEYBYTES + crypto_auth_BYTES) {
            continue; // paquete con tamaño inválido, descartar
        }

        // Verificar: "¿este tag es válido para este pk, dado que yo conozco el password?"
        if (crypto_auth_verify(reinterpret_cast<const unsigned char*>(payload.data()+crypto_box_PUBLICKEYBYTES), reinterpret_cast<const unsigned char*>(payload.data()), 32, passwordKey) != 0) {
            //Enviar mensaje diciendo que la contraseña es invalida
            std::vector<char> message = buildMessage(CONNECTION_ERROR, WRONG_PASSWORD);
            sendto(udpSocket,message.data(), message.size(),0, (sockaddr*)&senderAddr, sizeof(senderAddr));
            continue;
        }

        // 1. Extraer el pk del cliente (ya sabemos que el tag es válido, así que este pk es de confianza)
        unsigned char pkC[crypto_box_PUBLICKEYBYTES];
        memcpy(pkC, payload.data(), crypto_box_PUBLICKEYBYTES);
        // 2. Generar MI par de claves efímero (del servidor, para esta conexión)
        unsigned char pkS[crypto_box_PUBLICKEYBYTES];
        unsigned char skS[crypto_box_SECRETKEYBYTES];
        crypto_box_keypair(pkS, skS);
        // 3. Firmar pkC || pkS juntos (así el cliente sabe que esta respuesta es
        //    específicamente para SU intento de conexión, no una respuesta vieja reciclada)
        unsigned char combined[crypto_box_PUBLICKEYBYTES * 2];
        memcpy(combined, pkC, crypto_box_PUBLICKEYBYTES);
        memcpy(combined + crypto_box_PUBLICKEYBYTES, pkS, crypto_box_PUBLICKEYBYTES);
        unsigned char tag2[crypto_auth_BYTES];
        crypto_auth(tag2, combined, sizeof(combined), passwordKey);

        // 4. Calcular las claves de sesión YA MISMO (aunque el cliente todavía no confirmó nada)
        if (crypto_kx_server_session_keys(rxKey, txKey, pkS, skS, pkC) != 0) {
            std::cerr << "Error: clave pública del cliente inválida, abortando handshake." << std::endl;
            sodium_memzero(skS, sizeof(skS));
            status = UNEXPECTED_ERROR;
            continue;
        }
        // 5. Armar y mandar la respuesta: pkS (32) + tag2 (32)
        std::vector<char> responsePayload(crypto_box_PUBLICKEYBYTES + crypto_auth_BYTES);
        memcpy(responsePayload.data(), pkS, crypto_box_PUBLICKEYBYTES);
        memcpy(responsePayload.data() + crypto_box_PUBLICKEYBYTES, tag2, crypto_auth_BYTES);


        std::vector<char> message = buildMessage(CONNECTION_SUCCESS,  responsePayload.data(),static_cast<int>(responsePayload.size()));
        //Enviar mensaje diciendo que se permite la conexion
        int bytesSent = sendto(udpSocket,message.data(), message.size(),0, (sockaddr*)&senderAddr, sizeof(senderAddr));
        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "sendto() falló: " << WSAGetLastError() << std::endl;
            status = UNEXPECTED_ERROR;
            continue;
        }
        // 6. Limpiar la clave privada de memoria, ya no se necesita
        sodium_memzero(skS, sizeof(skS));

        //Obtener la IP de quien sea que se conecto
        char senderIP[INET_ADDRSTRLEN];

        inet_ntop(
        AF_INET,
        &senderAddr.sin_addr,
        senderIP,
        INET_ADDRSTRLEN);

        sendSeq = 0;
        lastRecvSeq = 0;

        status = CONNECTED;
        connectionIP = senderIP;
        connectionAddr = senderAddr;
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

bool NetManager::manageHeartBeat()
{
    auto now = std::chrono::steady_clock::now();

   if ( std::chrono::duration_cast<std::chrono::seconds>(now - lastHeartBeatSendTime).count() > heartbeatInterval )
   {
       if (!waitingHeartbeat)
       {
           firstHeartBeatTime = now;
       } else
       {
           heartbeatsTry++;
           std::cerr << "Missed heartbeat, try count: " << heartbeatsTry << std::endl;
       }
       lastHeartBeatSendTime =  now;
       waitingHeartbeat = true;
       sendPackage("START",HEARTBEAT);
   }

    if ( heartbeatsTry > heartbeatsMaxTrys )
    {
        std::cout << "Disconnected from: " << connectionIP << std::endl;
        status = TIMEOUT;
        connectionIP = "";
        connectionAddr = {};
        return false;
    }
    return true;
}

void NetManager::serverThread()
{
    while (status != SHUTTING_DOWN)
    {
        connectionPassword = "@Milasco13";
        serverWaitForConnection();
        std::cout << "Connected to client: " << connectionIP << std::endl;
        while (status == CONNECTED)
        {
            // Esperar datos en el socket, pero como máximo X ms
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(udpSocket, &readfds);
            timeval tv{ 0, 1000000*heartbeatInterval }; // 10s

            int result = select(0, &readfds, nullptr, nullptr, &tv);

            if (result > 0 && FD_ISSET(udpSocket, &readfds)) {
                // Llegó algo -> recvfrom() ya no bloquea, procesar mensaje
                handleIncomingPacket();
            }
            manageHeartBeat();
        }
    }
}

void NetManager::clientWaitForConnection()
{
    while (status != CONNECTED && status != SHUTTING_DOWN)
    {
        //Esperar a que el cliente ponga la IP, puerto y contraseña
        shoulTryConnection = false;
        std::unique_lock lock(mutex);

        cv.wait(lock, [this] {
            return shoulTryConnection;
        });
        shoulTryConnection = false;

        // 1. Generar un par de claves NUEVO, random, solo para esta conexión
        crypto_box_keypair(pk, sk);
        // pk = "mi clave pública de esta sesión" (32 bytes random, no es secreto)
        // sk = "mi clave privada de esta sesión" (32 bytes random, ESTO SÍ es secreto, nunca se manda)

        // 2. Convertir el password en una clave criptográfica de 32 bytes
        unsigned char passwordKey[crypto_generichash_KEYBYTES];
        crypto_generichash(passwordKey, sizeof(passwordKey),
                            reinterpret_cast<const unsigned char*>(connectionPassword.data()), connectionPassword.size(),
                            nullptr, 0);
        // passwordKey = una "huella" del password, en el formato que pide crypto_auth
        // (crypto_auth no acepta un string cualquiera como key, necesita 32 bytes exactos)

        // 3. "Firmar" mi clave pública usando el password como llave de la firma
        unsigned char tag[crypto_auth_BYTES]; // 32 bytes
        crypto_auth(tag, pk, sizeof(pk), passwordKey);
        // tag = prueba de que "yo generé este pk, y conozco el password"

        // Despierta cuando el cliente meta en "conectar"

        // Manda el mensaje a la IP dada
        status = CONNECTING;
        char payload0[crypto_generichash_KEYBYTES+crypto_box_PUBLICKEYBYTES];
        memcpy(payload0, pk, crypto_box_PUBLICKEYBYTES);
        memcpy(payload0+crypto_box_PUBLICKEYBYTES,tag,crypto_box_PUBLICKEYBYTES);
        std::vector<char> message = buildMessage(CONNECTION_TRY, payload0,crypto_generichash_KEYBYTES+crypto_box_PUBLICKEYBYTES);
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
            if (strcmp(payload.c_str(),WRONG_PASSWORD) == 0)
            {
                status = INVALID_PASSWORD;
            } else
            {
                status = UNEXPECTED_ERROR;
            }
        } else
        {
            // payload = pkS (32) + tag2 (32)
            if (payload.size() != crypto_box_PUBLICKEYBYTES + crypto_auth_BYTES) {
                status = UNEXPECTED_ERROR;
                continue;
            }

            unsigned char pkS[crypto_box_PUBLICKEYBYTES];
            memcpy(pkS, payload.data(), crypto_box_PUBLICKEYBYTES);

            // Reconstruir lo mismo que firmó el servidor: pk (mío) || pkS (suyo)
            unsigned char combined[crypto_box_PUBLICKEYBYTES * 2];
            memcpy(combined, pk, crypto_box_PUBLICKEYBYTES);
            memcpy(combined + crypto_box_PUBLICKEYBYTES, pkS, crypto_box_PUBLICKEYBYTES);

            const auto* receivedTag2 = reinterpret_cast<const unsigned char*>(payload.data() + crypto_box_PUBLICKEYBYTES);

            if (crypto_auth_verify(receivedTag2, combined, sizeof(combined), passwordKey) != 0) {
                std::cerr << "Respuesta del servidor no autenticada, posible ataque." << std::endl;
                status = UNEXPECTED_ERROR;
                continue;
            }

            if (crypto_kx_client_session_keys(rxKey, txKey, pk, sk, pkS) != 0) {
                std::cerr << "Error: clave pública del servidor inválida." << std::endl;
                status = UNEXPECTED_ERROR;
                continue;
            }

            sodium_memzero(sk, sizeof(sk)); // ya no se necesita, no dejarla en memoria
            sendSeq = 0;
            lastRecvSeq = 0;
            status = CONNECTED;
            connectionAddr = serverAddr;
        }

    }
}

void NetManager::clientThread()
{
    while (status != SHUTTING_DOWN)
    {
        clientWaitForConnection();
        std::cout << "Connected to client: " << connectionIP << std::endl;
        while (status == CONNECTED)
        {
            // Esperar datos en el socket, pero como máximo X ms
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(udpSocket, &readfds);
            timeval tv{ 0, 1000000*heartbeatInterval }; // 100ms

            int result = select(0, &readfds, nullptr, nullptr, &tv);

            if (result > 0 && FD_ISSET(udpSocket, &readfds)) {
                // Llegó algo -> recvfrom() ya no bloquea, procesar mensaje
                handleIncomingPacket();
            }
            manageHeartBeat();
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

    if (sodium_init() < 0) {
        std::cerr << "Error: no se pudo inicializar libsodium." << std::endl;
        return false;
    }
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

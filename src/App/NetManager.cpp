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
#include "Logger.h"

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

bool NetManager::sendMessage(const Message& m, const sockaddr_in& addr)
{
    int mSize = static_cast<int>(m.getSize());
    int sent = sendto(udpSocket, m.getData(), mSize, 0,
                      reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    return sent == mSize;
}

Message NetManager::decryptMessage(const MessageView& cryptedMessage)
{
    const size_t minSize = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES + crypto_aead_xchacha20poly1305_ietf_ABYTES;
    if (static_cast<size_t>(cryptedMessage.getPayloadSize()) < minSize) {
        return Message(PackageHeader::UNEXPECTED_HEADER,0); // paquete demasiado corto para ser válido
    }


    const auto* nonce = reinterpret_cast<const unsigned char*>(cryptedMessage.getPayload());
    // Rechazar paquetes repetidos o viejos (replay protection)
    uint64_t recvSeqBigEndian;
    memcpy(&recvSeqBigEndian, nonce, sizeof(recvSeqBigEndian));
    uint64_t recvSeq = _byteswap_uint64(recvSeqBigEndian);
    if (recvSeq <= lastRecvSeq) {
        stderr_logger.log("Paquete repetido o fuera de orden, descartado (seq=", recvSeq, ")");
        return Message(PackageHeader::UNEXPECTED_HEADER,0);
    }

    const auto* ciphertext = reinterpret_cast<const unsigned char*>(cryptedMessage.getPayload()) + crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
    Message out(cryptedMessage.getPackageHeader(),cryptedMessage.getPayloadSize() - crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);

    unsigned long long decryptedLen = 0;
    if (crypto_aead_xchacha20poly1305_ietf_decrypt(
            reinterpret_cast<unsigned char*>(out.getPayload()), &decryptedLen, nullptr,
            ciphertext, out.getPayloadSize(),
            reinterpret_cast<const unsigned char*>(cryptedMessage.getData()), STATUS_HEADER_SIZE, // AAD
            nonce, rxKey) != 0)
    {
        stderr_logger.log("Fallo de autenticación: paquete corrupto o manipulado.");
        return Message(PackageHeader::UNEXPECTED_HEADER,0);
    }

    lastRecvSeq = recvSeq;
    out.setFilledSize(decryptedLen);
    return out;
}
Message NetManager::encryptMessage(const Message& message)
{
    // 2. Nonce = sendSeq expandido a 24 bytes (los primeros 8 bytes llevan el contador, el resto en 0)
    const size_t payloadSize = message.getPayloadSize();
    Message out(message.getPackageHeader(), crypto_aead_xchacha20poly1305_ietf_NPUBBYTES + payloadSize + crypto_aead_xchacha20poly1305_ietf_ABYTES);
    unsigned char* nonce = reinterpret_cast<unsigned char*>(out.getPayload());

    uint64_t seqBigEndian = _byteswap_uint64(sendSeq); // MinGW/Windows: usa esto en vez de htobe64
    memcpy(nonce, &seqBigEndian, sizeof(seqBigEndian));

    // 3. Encriptar
    unsigned long long ciphertextLen = 0;
    unsigned char* ciphertext = reinterpret_cast<unsigned char*>(out.getHeader() + crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
    crypto_aead_xchacha20poly1305_ietf_encrypt(
        ciphertext, &ciphertextLen,
        reinterpret_cast<const unsigned char*>(message.getPayload()), payloadSize,
        reinterpret_cast<const unsigned char*>(message.getHeader()), STATUS_HEADER_SIZE, // AAD
        nullptr, nonce, txKey);

    sendSeq++; // avanzar el contador, SIEMPRE, incluso si algo falla después

    // 4. Armar el paquete final: header + nonce + ciphertext
    out.setFilledSize(crypto_aead_xchacha20poly1305_ietf_NPUBBYTES + ciphertextLen);
    return out;
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
        stdout_logger.log("Package from unknown sender: ", senderIP);
        return; // respuesta de origen no esperado, descartar
        }
    if (bytesReceived < MAX_UDP_RECEIVE_BUFFER_SIZE)
    {
        buffer[bytesReceived] = '\0';
    } else
    {
        stdout_logger.log("<Mensaje truncado>");
        buffer[MAX_UDP_RECEIVE_BUFFER_SIZE-1] = '\0';
    }
    MessageView messageIn = MessageView(buffer, bytesReceived);
    Message decryptedMessage = decryptMessage(messageIn);
    if (decryptedMessage.getPayloadSize() == 0) {
        stdout_logger.log("Invalid package recivec");
        return; // paquete inválido, corrupto, repetido o manipulado — se descarta silenciosamente
    }
    switch (decryptedMessage.getPackageHeader()   )
    {
        case HEARTBEAT:
            if (strcmp(decryptedMessage.getPayload(), "START") == 0)
            {
            sendPackage("ANSWER",HEARTBEAT);
            } else
            {
            waitingHeartbeat = false;
            auto ping = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()- lastHeartBeatSendTime);
            stdout_logger.log("ping: ", ping.count(), " ms");
            }
        case MESSAGE:
        stdout_logger.log(connectionIP + ": ", decryptedMessage.getPayload());
        default:
            return;
    }
}

bool NetManager::sendMessage(Message& message)
{
    return sendMessage(message,connectionAddr);
}

NetManager::ConexionStatus NetManager::tryAuth(MessageView& messageIn,unsigned char* pkS,unsigned char* tag2,unsigned char* skS,sockaddr_in& senderAddr)
{
    unsigned char passwordKey[crypto_generichash_KEYBYTES];
    crypto_generichash(passwordKey, sizeof(passwordKey),
                        reinterpret_cast<const unsigned char*>(connectionPassword.data()), connectionPassword.size(),
                        nullptr, 0);


    if (messageIn.getPayloadSize() != crypto_box_PUBLICKEYBYTES + crypto_auth_BYTES) {
        return UNEXPECTED_ERROR; // paquete con tamaño inválido, descartar
    }
    // Verificar: "¿este tag es válido para este pk, dado que yo conozco el password?"
    if (crypto_auth_verify(reinterpret_cast<const unsigned char*>(messageIn.getPayload()+crypto_box_PUBLICKEYBYTES), reinterpret_cast<const unsigned char*>(messageIn.getPayload()), 32, passwordKey) != 0) {
        return INVALID_PASSWORD;
    }

    // 1. Extraer el pk del cliente (ya sabemos que el tag es válido, así que este pk es de confianza)
    unsigned char pkC[crypto_box_PUBLICKEYBYTES];
    memcpy(pkC, messageIn.getPayload(), crypto_box_PUBLICKEYBYTES);
    // 2. Generar MI par de claves efímero (del servidor, para esta conexión)


    crypto_box_keypair(pkS, skS);
    // 3. Firmar pkC || pkS juntos (así el cliente sabe que esta respuesta es
    //    específicamente para SU intento de conexión, no una respuesta vieja reciclada)
    unsigned char combined[crypto_box_PUBLICKEYBYTES * 2];
    memcpy(combined, pkC, crypto_box_PUBLICKEYBYTES);
    memcpy(combined + crypto_box_PUBLICKEYBYTES, pkS, crypto_box_PUBLICKEYBYTES);

    crypto_auth(tag2, combined, sizeof(combined), passwordKey);

    // 4. Calcular las claves de sesión YA MISMO (aunque el cliente todavía no confirmó nada)
    if (crypto_kx_server_session_keys(rxKey, txKey, pkS, skS, pkC) != 0) {
        stderr_logger.log("Error: clave pública del cliente inválida, abortando handshake.");
        sodium_memzero(skS, sizeof(skS));
        status = UNEXPECTED_ERROR;

    }
    return CONNECTED;
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
        stderr_logger.log("sendto() falló: ", WSAGetLastError());
    } else {
        // bytesSent == messageLen si se envió completo (UDP no fragmenta la llamada)
    }
}

MessageView NetManager::waitForServerConnectRequest(sockaddr_in& senderAddr)
{
    //Esperar por una conexion
    char buffer[MAX_UDP_RECEIVE_BUFFER_SIZE]; // suficiente para el datagrama UDP más grande posible
    while (status != SHUTTING_DOWN)
    {
        int senderAddrLen = sizeof(senderAddr);
        status = WAITING;

        //Espera hasta recibir la conexion
        int bytesReceived = recvfrom(udpSocket,buffer, sizeof(buffer),0,  (sockaddr*)&senderAddr, &senderAddrLen );
        //Una vez recibida valida si fue exitosa
        if (bytesReceived == SOCKET_ERROR || bytesReceived >= MAX_UDP_RECEIVE_BUFFER_SIZE) {
            stderr_logger.log("recvfrom() falló: ", WSAGetLastError());
            continue;
        }
        return MessageView(buffer, bytesReceived);;
    }
    return MessageView();
}

NetManager::ConexionStatus NetManager::sendHandShakeResponse(MessageView& messageIn, sockaddr_in& clientAddr)
{
    Message messageOut(CONNECTION_SUCCESS,crypto_box_PUBLICKEYBYTES + crypto_auth_BYTES);
    unsigned char skS[crypto_box_SECRETKEYBYTES];
    unsigned char*  pkS = reinterpret_cast<unsigned char*>(messageOut.getPayload());
    unsigned char* tag2 = reinterpret_cast<unsigned char*>(messageOut.getPayload() + crypto_box_PUBLICKEYBYTES);

    NetManager::ConexionStatus s= (tryAuth(messageIn,pkS,tag2,skS, clientAddr));
    if (s != CONNECTED)
    {
        std::string m;
        switch(s) {
        case INVALID_PASSWORD:
            m = WRONG_PASSWORD;
            break;
        default:
            m = "Unexpected error.";
            break;
        }
        Message m2(CONNECTION_ERROR,m);
        sendMessage(m2, clientAddr);
        return s;
    }
    //Enviar mensaje diciendo que se permite la conexion
    int bytesSent = sendto(udpSocket,messageOut.getData(), messageOut.getSize(),0, (sockaddr*)&clientAddr, sizeof(clientAddr));
    if (bytesSent == SOCKET_ERROR) {
        stderr_logger.log("sendto() falló: ", WSAGetLastError());
        return UNEXPECTED_ERROR;
    }
    // 6. Limpiar la clave privada de memoria, ya no se necesita
    sodium_memzero(skS, sizeof(skS));
    return CONNECTED;
}
void NetManager::serverWaitForConnection()
{
    while (status != CONNECTED && status != SHUTTING_DOWN)
    {
        sockaddr_in clientAddr{};
        MessageView messageIn = waitForServerConnectRequest(clientAddr);
        if (messageIn.getPackageHeader() != PackageHeader::CONNECTION_TRY) continue;

       status = sendHandShakeResponse(messageIn, clientAddr);
        if (status != CONNECTED) continue;

        //Obtener la IP de quien sea que se conecto
        inet_ntop(AF_INET,&clientAddr.sin_addr,connectionIP.data(),INET_ADDRSTRLEN);

        sendSeq = 0;
        lastRecvSeq = 0;
        connectionAddr = clientAddr;
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
       if (waitingHeartbeat)
       {
           heartbeatsTry++;
           stderr_logger.log("Missed heartbeat, try count: ", heartbeatsTry);

       }
       lastHeartBeatSendTime =  now;
       waitingHeartbeat = true;
       sendPackage("START",HEARTBEAT);
   }

    if ( heartbeatsTry > heartbeatsMaxTrys )
    {
        stdout_logger.log("Disconnected from: ", connectionIP);
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
        stdout_logger.log("Connected to client: ", connectionIP);
        connectionLoop();
    }
}

bool NetManager::initHandShake(sockaddr_in& destAddr,HandshakeState& hs)
{
    // 1. Generar un par de claves NUEVO, random, solo para esta conexión
    crypto_box_keypair(hs.pk, hs.sk);
    // pk = "mi clave pública de esta sesión" (32 bytes random, no es secreto)
    // sk = "mi clave privada de esta sesión" (32 bytes random, ESTO SÍ es secreto, nunca se manda)

    // 2. Convertir el password en una clave criptográfica de 32 bytes
    crypto_generichash(hs.passwordKey, sizeof(hs.passwordKey),
                        reinterpret_cast<const unsigned char*>(connectionPassword.data()), connectionPassword.size(),
                        nullptr, 0);
    // passwordKey = una "huella" del password, en el formato que pide crypto_auth
    // (crypto_auth no acepta un string cualquiera como key, necesita 32 bytes exactos)
    // 3. "Firmar" mi clave pública usando el password como llave de la firma
    status = CONNECTING;
    Message message(CONNECTION_TRY, crypto_box_PUBLICKEYBYTES + crypto_auth_BYTES);
    unsigned char* tag = reinterpret_cast<unsigned char*>(message.getPayload() +crypto_box_PUBLICKEYBYTES); // 32 bytes
    memcpy(message.getPayload(), hs.pk, crypto_box_PUBLICKEYBYTES);
    crypto_auth(tag, hs.pk, sizeof(hs.pk), hs.passwordKey);



    if (!sendMessage(message,destAddr))
    {
        stderr_logger.log("sendto() falló: ", WSAGetLastError());
        status = UNEXPECTED_ERROR;
        return false;
    }
    return true;
}

void NetManager::waitForClientConnectRequest()
{
    //Esperar a que el cliente ponga la IP, puerto y contraseña
    shoulTryConnection = false;
    std::unique_lock lock(mutex);

    cv.wait(lock, [this] {
        return shoulTryConnection;
    });
    shoulTryConnection = false;
}
                           // 2. ya la tienes (envía CONNECTION_TRY)
NetManager::ConexionStatus NetManager::receiveHandshakeResponse(const sockaddr_in& server, char* buf, int& n)
{
    //Espera por una respuesta del server
    DWORD timeout = 10000; // 1000 ms = 1 segundo
    setsockopt( udpSocket,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));
    char buffer[65536];
    sockaddr_in replyAddr{};
    int replyAddrLen = sizeof(replyAddr);
    int bytesReceived = recvfrom(udpSocket, buffer, sizeof(buffer), 0,(sockaddr*)&replyAddr, &replyAddrLen);
    //Validar si la respuesta se recibio
    if (bytesReceived == SOCKET_ERROR)
    {
        int error = WSAGetLastError();

        if (error == WSAETIMEDOUT)
        {
           return status = TIMEOUT;
        } else
        {
           return status = UNEXPECTED_ERROR;
        }

    }
    if (replyAddr.sin_addr.s_addr != server.sin_addr.s_addr ||replyAddr.sin_port != server.sin_port)  return UNEXPECTED_ERROR;

    return CONNECTED;
}
NetManager::ConexionStatus NetManager::processHandshakeResponse(const MessageView& handShakeResponse, const sockaddr_in& server, HandshakeState& hs)
{
    if (handShakeResponse.getPackageHeader() != CONNECTION_SUCCESS)
    {
        if (strcmp(handShakeResponse.getPayload(),WRONG_PASSWORD) == 0) return INVALID_PASSWORD;
        else return UNEXPECTED_ERROR;

    }
    // payload = pkS (32) + tag2 (32)
    if (handShakeResponse.getSize() != crypto_box_PUBLICKEYBYTES + crypto_auth_BYTES) return  UNEXPECTED_ERROR;

    // Reconstruir lo mismo que firmó el servidor: pk (mío) || pkS (suyo)
    unsigned char combined[crypto_box_PUBLICKEYBYTES * 2];
    memcpy(combined, hs.pk, crypto_box_PUBLICKEYBYTES);
    memcpy(combined + crypto_box_PUBLICKEYBYTES, handShakeResponse.getPayload(), crypto_box_PUBLICKEYBYTES);

    const auto* receivedTag2 = reinterpret_cast<const unsigned char*>(handShakeResponse.getPayload() + crypto_box_PUBLICKEYBYTES);
    if (crypto_auth_verify(receivedTag2, combined, sizeof(combined), hs.passwordKey) != 0) {
        stderr_logger.log("Respuesta del servidor no autenticada, posible ataque.");
        return UNEXPECTED_ERROR;
    }

    if (crypto_kx_client_session_keys(rxKey, txKey, hs.pk, hs.sk, reinterpret_cast<unsigned const char*>(handShakeResponse.getPayload())) != 0) {
        stderr_logger.log("Error: clave pública del servidor inválida.");
        return UNEXPECTED_ERROR;
    }

    sodium_memzero(hs.sk, sizeof(hs.sk)); // ya no se necesita, no dejarla en memoria

    return CONNECTED;
}
void NetManager::clientWaitForConnection()
{
    while (status != CONNECTED && status != SHUTTING_DOWN)
    {
        waitForClientConnectRequest();
        status = CONNECTING;
        // Dirección de destino
        sockaddr_in server{};
        server.sin_family = AF_INET;
        server.sin_port   = htons(hostPort);                     // puerto destino
        inet_pton(AF_INET, connectionIP.c_str(), &server.sin_addr); // IP destino

        HandshakeState hs;
        if (!initHandShake(server,hs)) { status = UNEXPECTED_ERROR; continue; }

        char buf[65536];
        int n = 0;
        ConexionStatus s = receiveHandshakeResponse(server, buf, n);
        if (s != CONNECTED) { status = s; continue; }

        status = processHandshakeResponse(MessageView(buf, n), server, hs);
        if (status != CONNECTED) continue;

        sendSeq = 0;
        lastRecvSeq = 0;
        connectionAddr = server;
    }
}

void NetManager::clientThread()
{
    while (status != SHUTTING_DOWN)
    {
        clientWaitForConnection();
        stdout_logger.log("Connected to client: ", connectionIP);
        connectionLoop();
    }
}
void NetManager::connectionLoop()
{
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


bool NetManager::init()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);

    if (sodium_init() < 0) {
        stderr_logger.log("Error: no se pudo inicializar libsodium.");
        return false;
    }
    // TCP: SOCK_STREAM | UDP: SOCK_DGRAM
    udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;  // escuchar en todas las interfaces
    serverAddr.sin_port = htons(hostPort);         // puerto elegido

    if (bind(udpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        stderr_logger.log("bind() falló: ", WSAGetLastError());
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


/**
 * @file App.cpp
 * @brief App class implementation.
 * @author Miguel Velasco
 */
#include "App.h"

#include <WinSock2.h>
#include <Ws2tcpip.h>

#include <iphlpapi.h>

#include <imGUI/imgui_impl_sdl3.h>
#include "../Registry/ImGuiFonts.h"
#include "DesktopDuplicatorManager.h"
#include "../Renderer/Renderer.h"
#include "../Registry/Scenes.h"
#include "../Renderer/Window.h"
#include "../Scene/Model.h"
#include "../Renderer/VulkanDevice.h"
#include <winhttp.h>
#include "../Registry/Images.h"
#include "../Renderer/Image.h"
#include "../Renderer/Pipeline.h"

static std::string getPublicIP() {
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
static std::vector<std::string> getAllPrivateIPs() {
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
void App::startServer()
{
    type = App::HOST;
    if (desktopDuplicatorManager)
    {
        std::cerr << "StartServer(): A server is already existing." << std::endl;
        runnig = false;
    }
    desktopDuplicatorManager = new DesktopDuplicatorManager();
    if (!desktopDuplicatorManager->createDesktopDuplicator())
    {
        runnig = false;
    }
    desktopImage =Image::importFromD3D11Handle(renderer->getVulkanDevice(),desktopDuplicatorManager->getHandle(),desktopDuplicatorManager->getWidth(),desktopDuplicatorManager->getHeight(),DesktopDuplicatorManager::dxgiToVulkanFormat(desktopDuplicatorManager->getFormat()));


    VkCommandBuffer cmd = renderer->getVulkanDevice()->beginSingleTimeCommands();
    desktopImage->transitionLayout(cmd,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    renderer->getVulkanDevice()->endSingleTimeCommands(cmd);
    desktopImage->setKeyedMutexSync(/*acquireKey=*/1, /*releaseKey=*/0);


    auto cfg = Pipelines::defaultPipeline->getConfig();
    cfg.images[0].image   = desktopImage->getView();
    cfg.images[0].sampler = desktopImage->getSampler();
    cfg.images[0].layout  = desktopImage->getCurrentLayout();
    Pipelines::defaultPipeline->updateDescriptorSet(cfg.uniformObjects, cfg.images);

    renderer->setSharedCaptureImage(desktopImage);
    FrameBuffers::turnOnFBO(FrameBuffers::defaultFrameBuffer);

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);

    // TCP: SOCK_STREAM | UDP: SOCK_DGRAM
    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;  // escuchar en todas las interfaces
    serverAddr.sin_port = htons(hostPort);         // puerto elegido

    if (bind(udpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "bind() falló: " << WSAGetLastError() << std::endl;
    }
    privateIps = getAllPrivateIPs();
    publicIP = getPublicIP();
    /**char buffer[65536]; // suficiente para el datagrama UDP más grande posible
    sockaddr_in senderAddr{};
    int senderAddrLen = sizeof(senderAddr);

    int bytesReceived = recvfrom(
        udpSocket,
        buffer, sizeof(buffer),
        0,                                  // flags
        (sockaddr*)&senderAddr, &senderAddrLen  // te dice quién lo mandó
    );

    if (bytesReceived == SOCKET_ERROR) {
        std::cerr << "recvfrom() falló: " << WSAGetLastError() << std::endl;
    } else {
        // buffer[0..bytesReceived) tiene el contenido de UN datagrama completo
    }*/

}

void App::startClient()
{
}

App::App(const std::function<void(App*)>& registryCallback) {
    //Initizialise window
    window = new Window();
    if (window->isError()) {
        std::cerr << "Failed to initialize window." << std::endl;
        return;
    }
    //Initialize renderer
    renderer = new Renderer(window);
    if (renderer->error) {
        std::cerr << "Failed to initialize renderer." << std::endl;
        return;
    }
    #ifdef _DEBUG
    std::cout << "Renderer initialized successfully." << std::endl;
    #endif

    player = new Player(0);



    registryCallback(this);



    Menus::openMenu(CHOOSE_APP_TYPE_MENU_ID);

    //Finally execution loop
    executionLoop();




}



App::~App()
{
    vkDeviceWaitIdle(renderer->getVulkanDevice()->device);
    delete desktopImage;
    ImGuiFonts::freeFonts();
    delete desktopDuplicatorManager;
    Meshes::freeMeshes();
    delete player;
    Uniforms::freeUniforms();
    Images::freeImages();
    delete renderer;
    delete window;

    Menus::freeMenus();
    }



void App::executionLoop()
{

    Uint64 timeAcc = 0;
    Uint64 lastCycleTimeNS = SDL_GetTicksNS(); // Tiempo del último frame en segundos




    unsigned int maxTicksUntilOverflow = 5;

    Uint64 lastSecond = 0;
    Uint64 cyclesCounter = 0;
    Uint64 ticksCounter = 0;

    //unsigned int lastSecond = 0;
    //unsigned int cyclesCounter = 0;
    while (runnig) {
        auto minNsPerCycle = 1000000000/player->getPlayerCameraSettings()->maxCyclesPerSecond;
        auto minNSPerTick = (1000000000/player->getPlayerCameraSettings()->maxTicksPerSecond);

        //Prepare time variables
        cycleStartTimeNS = SDL_GetTicksNS(); // Convertir a segundos
        cycleDeltaTimeNS = cycleStartTimeNS - lastCycleTimeNS; //Tiempo entre el inicio del ciclo interior y el inicio de este ciclo
        lastCycleTimeNS = cycleStartTimeNS;
        timeAcc += cycleDeltaTimeNS;


        //Get evenys
        manageEvents();
        //Call update to logic
        unsigned int ticks = 0;

        while (timeAcc >= static_cast<Uint64>(minNSPerTick) && ticks < maxTicksUntilOverflow )
        {
            tickDeltaTimeNS = SDL_GetTicksNS()- tickStartTimeNS;
            tickStartTimeNS = SDL_GetTicksNS();

            timeAcc -= minNSPerTick;
            ticks++;
            ticksCounter++;
        }

        //Render GUI
        renderGUI();

       // + + + + +  VULKAN RENDER + + + + +
        if (type == HOST)
        {
            desktopDuplicatorManager->writeDestinyResource();

        } else
        {

        }

        renderer->update();
        //Uniforms::cameraUniform->addIndexToQueue(Uniforms::CameraUBO::TIME);
        //Uniforms::cameraUniform->clearQueue();


        // - - - - - VULKAN RENDER END - - - -

        //For debug count the number off ticks and cycles in a second
        uint64_t secondTimer = SDL_GetTicks();
        unsigned int currentSecond = secondTimer / 1000;
        if (currentSecond != lastSecond)
        {// this code happens every second
            const unsigned int numSeconds =currentSecond- lastSecond ;


            cyclesPerSecond = static_cast<int>(cyclesCounter / numSeconds);
            ticksPerSecond = static_cast<int>(ticksCounter / numSeconds);

            lastSecond = currentSecond;
            cyclesCounter = 0;
            ticksCounter = 0;
        }
        cyclesCounter++;

        //Garantir um maximo de ciclos por segundo
        int  cycleMissingTime = static_cast<int>(minNsPerCycle-(SDL_GetTicksNS() - cycleStartTimeNS));
        if (0 <  cycleMissingTime)
        {
            SDL_DelayNS(cycleMissingTime);
        }

    }
    //debugTestFBO(this);

}

void App::renderGUI() {
    // 1. Preparar frame de ImGui (antes de tocar el command buffer)
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();


    Menus::drawMenus();

    ImGui::Render();
}

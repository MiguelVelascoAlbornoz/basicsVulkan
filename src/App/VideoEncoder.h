//
// Created by migue on 08/09/2026.
//

#ifndef BASICSVULKAN_VIDEOENCODER_H
#define BASICSVULKAN_VIDEOENCODER_H

#include <vector>
#include <queue>

#define WIN32_LEAN_AND_MEAN

#include <d3d11.h>
#include "mfobjects.h"
#include "mftransform.h"
#include "mfidl.h" // IMFMediaEventGenerator, IMFMediaEvent

class VideoEncoder
{
public:
    VideoEncoder() = default;
    ~VideoEncoder();

    bool init(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height);

    /** @brief Intenta meter el frame capturado al encoder AHORA MISMO.
     *  Si el MFT todavia no pidio input (no hay "cupo" async disponible),
     *  el frame se descarta silenciosamente -- para streaming en vivo esto
     *  es preferible a bloquear o forzar la entrada, ya que el proximo tick
     *  va a capturar una imagen de pantalla mas reciente de todas formas.
     *  @return true si el frame entro al encoder, false si se descarto. */
    bool submitFrame(ID3D11Texture2D* bgraFrame, LONGLONG timestamp100ns);

    /** @brief Debe llamarse una vez por tick, ANTES de submitFrame().
     *  Drena TODOS los eventos async pendientes del MFT (METransformNeedInput /
     *  METransformHaveOutput) sin bloquear, actualizando el contador de cupos
     *  de input disponibles y encolando cualquier frame comprimido nuevo. */
    void update();

    /** @brief Saca un frame comprimido de la cola interna, si hay alguno listo.
     *  Se puede (y se debe) llamar en un while hasta que devuelva false --
     *  cada llamada exitosa da UN solo access unit H.264 completo, nunca
     *  varios concatenados. */
    bool popEncodedFrame(std::vector<char>& outFrame);

    static void dumpD3D11DebugMessages(ID3D11Device* device);

private:
    bool configureMediaTypes() const;
    bool initColorConverter();
    bool convertToNV12(ID3D11Texture2D* bgraSource) const;

    void handleNeedInput();
    void handleHaveOutput();

    int width = 0, height = 0;
    LONGLONG frameDuration100ns = 0;

    IMFTransform* encoderMFT = nullptr;
    IMFMediaEventGenerator* eventGenerator = nullptr;
    IMFDXGIDeviceManager* dxgiDeviceManager = nullptr;

    ID3D11VideoDevice* videoDevice = nullptr;
    ID3D11VideoContext* videoContext = nullptr;
    ID3D11VideoProcessor* videoProcessor = nullptr;
    ID3D11VideoProcessorEnumerator* videoProcessorEnum = nullptr;
    ID3D11Texture2D* nv12Texture = nullptr;
    ID3D11Texture2D* encoderInputTexture = nullptr;

    ID3D11Device* d3dDevice = nullptr;
    ID3D11DeviceContext* d3dContext = nullptr;

    /** @brief Cuantos samples de input puede aceptar el MFT AHORA MISMO,
     *  segun los eventos METransformNeedInput ya procesados. Un MFT async
     *  suele pedir varios de una vez al arrancar, por su lookahead interno. */
    int inputSlotsAvailable = 0;

    /** @brief Frames comprimidos listos para enviar, ya separados uno por uno
     *  (nunca concatenados). App.cpp los va sacando con popEncodedFrame(). */
    std::queue<std::vector<char>> readyFrames;
};

#endif //BASICSVULKAN_VIDEOENCODER_H
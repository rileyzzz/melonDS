#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#include <vector>
#include <string>
#include <algorithm>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#ifndef _WIN32
#include <unistd.h>
#include <sys/socket.h>
#include <signal.h>
#endif

#include <SDL.h>

#ifdef OGLRENDERER_ENABLED
#include "OpenGLSupport.h"
#endif

#include "main.h"
#include "Input.h"
//#include "CheatsDialog.h"
//#include "EmuSettingsDialog.h"
//#include "InputConfigDialog.h"
//#include "VideoSettingsDialog.h"
//#include "AudioSettingsDialog.h"
//#include "WifiSettingsDialog.h"
//#include "InterfaceSettingsDialog.h"

#include "types.h"
#include "version.h"

#include "FrontendUtil.h"
#include "OSD.h"

#include "NDS.h"
#include "GBACart.h"
#include "GPU.h"
#include "SPU.h"
#include "Wifi.h"
#include "Platform.h"
#include "Config.h"
#include "PlatformConfig.h"

#include "Savestate.h"

#include "main_shaders.h"


#ifdef _MSC_VER 
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

// TODO: uniform variable spelling

bool RunningSomething;

EmuThread* emuThread;

int autoScreenSizing = 0;

int videoRenderer;
GPU::RenderSettings videoSettings;
bool videoSettingsDirty;

SDL_AudioDeviceID audioDevice;
int audioFreq;
SDL_cond* audioSync;
SDL_mutex* audioSyncLock;

SDL_AudioDeviceID micDevice;
s16 micExtBuffer[2048];
u32 micExtBufferWritePos;

u32 micWavLength;
s16* micWavBuffer;

SDL_Window* mainWindow;
SDL_GLContext glcontext;
SDL_Renderer* sdl_renderer;

ScreenPanelGL* panelGL;

bool hasOGL = true;

void audioCallback(void* data, Uint8* stream, int len)
{
    //return;

    len /= (sizeof(s16) * 2);

    // resample incoming audio to match the output sample rate

    int len_in = Frontend::AudioOut_GetNumSamples(len);
    s16 buf_in[1024 * 2];
    int num_in;

    SDL_LockMutex(audioSyncLock);
    num_in = SPU::ReadOutput(buf_in, len_in);
    SDL_CondSignal(audioSync);
    SDL_UnlockMutex(audioSyncLock);

    if (num_in < 1)
    {
        memset(stream, 0, len * sizeof(s16) * 2);
        return;
    }

    int margin = 6;
    if (num_in < len_in - margin)
    {
        int last = num_in - 1;

        for (int i = num_in; i < len_in - margin; i++)
            ((u32*)buf_in)[i] = ((u32*)buf_in)[last];

        num_in = len_in - margin;
    }

    Frontend::AudioOut_Resample(buf_in, num_in, (s16*)stream, len, Config::AudioVolume);
}

void micCallback(void* data, Uint8* stream, int len)
{
    return;

    s16* input = (s16*)stream;
    len /= sizeof(s16);

    int maxlen = sizeof(micExtBuffer) / sizeof(s16);

    if ((micExtBufferWritePos + len) > maxlen)
    {
        u32 len1 = maxlen - micExtBufferWritePos;
        memcpy(&micExtBuffer[micExtBufferWritePos], &input[0], len1 * sizeof(s16));
        memcpy(&micExtBuffer[0], &input[len1], (len - len1) * sizeof(s16));
        micExtBufferWritePos = len - len1;
    }
    else
    {
        memcpy(&micExtBuffer[micExtBufferWritePos], input, len * sizeof(s16));
        micExtBufferWritePos += len;
    }
}

void micProcess()
{
    int type = Config::MicInputType;
    bool cmd = Input::HotkeyDown(HK_Mic);

    if (type != 1 && !cmd)
    {
        type = 0;
    }

    switch (type)
    {
    case 0: // no mic
        Frontend::Mic_FeedSilence();
        break;

    case 1: // host mic
    case 3: // WAV
        Frontend::Mic_FeedExternalBuffer();
        break;

    case 2: // white noise
        Frontend::Mic_FeedNoise();
        break;
    }
}

EmuThread::EmuThread()
{
    EmuStatus = 0;
    EmuRunning = 2;
    EmuPause = 0;
    RunningSomething = false;

    //connect(this, SIGNAL(windowUpdate()), mainWindow->panel, SLOT(update()));
    //connect(this, SIGNAL(windowTitleChange(QString)), mainWindow, SLOT(onTitleUpdate(QString)));
    //connect(this, SIGNAL(windowEmuStart()), mainWindow, SLOT(onEmuStart()));
    //connect(this, SIGNAL(windowEmuStop()), mainWindow, SLOT(onEmuStop()));
    //connect(this, SIGNAL(windowEmuPause()), mainWindow->actPause, SLOT(trigger()));
    //connect(this, SIGNAL(windowEmuReset()), mainWindow->actReset, SLOT(trigger()));
    //connect(this, SIGNAL(windowLimitFPSChange()), mainWindow->actLimitFramerate, SLOT(trigger()));
    //connect(this, SIGNAL(screenLayoutChange()), mainWindow->panel, SLOT(onScreenLayoutChanged()));
    //connect(this, SIGNAL(windowFullscreenToggle()), mainWindow, SLOT(onFullscreenToggled()));
    //connect(this, SIGNAL(swapScreensToggle()), mainWindow->actScreenSwap, SLOT(trigger()));

    if (hasOGL) initOpenGL();
}

void EmuThread::initOpenGL()
{

    //QOpenGLContext* windowctx = mainWindow->getOGLContext();
    //QSurfaceFormat format = windowctx->format();

    //format.setSwapInterval(0);

    oglContext = SDL_GL_CreateContext(mainWindow);

    //oglSurface = new QOffscreenSurface();
    //oglSurface->setFormat(format);
    //oglSurface->create();
    //if (!oglSurface->isValid())
    //{
    //    // TODO handle this!
    //    printf("oglSurface shat itself :(\n");
    //    delete oglSurface;
    //    return;
    //}

    //oglContext = new QOpenGLContext();
    //oglContext->setFormat(oglSurface->format());
    //oglContext->setShareContext(windowctx);
    //if (!oglContext->create())
    //{
    //    // TODO handle this!
    //    printf("oglContext shat itself :(\n");
    //    delete oglContext;
    //    delete oglSurface;
    //    return;
    //}



    //oglContext->moveToThread(this);
}

void EmuThread::deinitOpenGL()
{
    //delete oglContext;
    //delete oglSurface;
}

void main_loop()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_KEYDOWN:
                keyPressEvent(&event.key);
                break;

            case SDL_KEYUP:
                keyReleaseEvent(&event.key);
                break;

            case SDL_MOUSEBUTTONDOWN:
                onMousePress(&event.button);
                break;

            case SDL_MOUSEBUTTONUP:
                onMouseRelease(&event.button);
                break;

            case SDL_MOUSEMOTION:
                onMouseMove(&event.motion);
                break;

            case SDL_WINDOWEVENT:
            {
                switch (event.window.event)
                {
                    case SDL_WINDOWEVENT_RESIZED:
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        printf("WINDOW RESIZE\n");
                        // SDL_Log("Window %d resized to %dx%d",
                        //         event->window.windowID, event->window.data1,
                        //         event->window.data2);
                        panelGL->setupScreenLayout();
                        break;
                }
                break;
            }
        }
    }
    panelGL->paintGL();
    //emuThread->frame();

    //if(emuThread)
        //emuThread->renderLoop();
    
    //panelGL->paintGL();

    //SDL_Delay(1000);
}

void EmuThread::start(const char* file)
{
    emscripten_set_main_loop(main_loop, 0, 0);

    emuRun();
    //run();
    _thread = new std::thread( [this, file] { this->run(file); } );
    _thread->detach();
    
}

void EmuThread::run(const char* file)
{
    //bool hasOGL = mainWindow->hasOGL;
    u32 mainScreenPos[3];

    NDS::Init();

    mainScreenPos[0] = 0;
    mainScreenPos[1] = 0;
    mainScreenPos[2] = 0;
    autoScreenSizing = 0;

    videoSettingsDirty = false;
    videoSettings.Soft_Threaded = Config::Threaded3D != 0;
    videoSettings.GL_ScaleFactor = Config::GL_ScaleFactor;

#ifdef OGLRENDERER_ENABLED
    if (hasOGL)
    {
        SDL_GL_MakeCurrent(mainWindow, oglContext);
        //oglContext->makeCurrent(oglSurface);
        videoRenderer = Config::_3DRenderer;
    }
    else
#endif
    {
        videoRenderer = 0;
    }

    GPU::InitRenderer(videoRenderer);
    GPU::SetRenderSettings(videoRenderer, videoSettings);

    Input::Init();

    u32 nframes = 0;
    double perfCountsSec = 1.0 / SDL_GetPerformanceFrequency();
    double lastTime = SDL_GetPerformanceCounter() * perfCountsSec;
    double frameLimitError = 0.0;
    double lastMeasureTime = lastTime;

    char melontitle[100];


    //char* file = "mkds.nds";

    int res = Frontend::LoadROM(file, Frontend::ROMSlot_NDS);
    if (res == Frontend::Load_OK)
    {
        //emuThread->emuRun();
    }

    while (EmuRunning != 0)
    {
        frame();
    }

    // EmuStatus = 0;

    // GPU::DeInitRenderer();
    // NDS::DeInit();
    // //Platform::LAN_DeInit();

    // if (hasOGL)
    // {
    //     //oglContext->doneCurrent();
    //     deinitOpenGL();
    // }
}

void EmuThread::frame()
{
    if(EmuRunning == 0)
        return;

    Input::Process();

    //if (Input::HotkeyPressed(HK_FastForwardToggle)) emit windowLimitFPSChange();

    //if (Input::HotkeyPressed(HK_Pause)) emit windowEmuPause();
    //if (Input::HotkeyPressed(HK_Reset)) emit windowEmuReset();

    //if (Input::HotkeyPressed(HK_FullscreenToggle)) emit windowFullscreenToggle();

    //if (Input::HotkeyPressed(HK_SwapScreens)) emit swapScreensToggle();

    //if (GBACart::CartInserted && GBACart::HasSolarSensor)
    //{
    //    if (Input::HotkeyPressed(HK_SolarSensorDecrease))
    //    {
    //        if (GBACart_SolarSensor::LightLevel > 0) GBACart_SolarSensor::LightLevel--;
    //        char msg[64];
    //        sprintf(msg, "Solar sensor level set to %d", GBACart_SolarSensor::LightLevel);
    //        OSD::AddMessage(0, msg);
    //    }
    //    if (Input::HotkeyPressed(HK_SolarSensorIncrease))
    //    {
    //        if (GBACart_SolarSensor::LightLevel < 10) GBACart_SolarSensor::LightLevel++;
    //        char msg[64];
    //        sprintf(msg, "Solar sensor level set to %d", GBACart_SolarSensor::LightLevel);
    //        OSD::AddMessage(0, msg);
    //    }
    //}

    if (EmuRunning == 1)
    {
        EmuStatus = 1;

        // update render settings if needed
//            if (videoSettingsDirty)
//            {
//                if (hasOGL != mainWindow->hasOGL)
//                {
//                    hasOGL = mainWindow->hasOGL;
//#ifdef OGLRENDERER_ENABLED
//                    if (hasOGL)
//                    {
//                        oglContext->makeCurrent(oglSurface);
//                        videoRenderer = Config::_3DRenderer;
//                    }
//                    else
//#endif
//                    {
//                        videoRenderer = 0;
//                    }
//                }
//                else
//                    videoRenderer = hasOGL ? Config::_3DRenderer : 0;
//
//                videoSettingsDirty = false;
//
//                videoSettings.Soft_Threaded = Config::Threaded3D != 0;
//                videoSettings.GL_ScaleFactor = Config::GL_ScaleFactor;
//                videoSettings.GL_BetterPolygons = Config::GL_BetterPolygons;
//
//                GPU::SetRenderSettings(videoRenderer, videoSettings);
//            }

        // process input and hotkeys
        NDS::SetKeyMask(Input::InputMask);

        if (Input::HotkeyPressed(HK_Lid))
        {
            bool lid = !NDS::IsLidClosed();
            NDS::SetLidClosed(lid);
            OSD::AddMessage(0, lid ? "Lid closed" : "Lid opened");
        }

        // microphone input
        micProcess();

        // auto screen layout
        if (Config::ScreenSizing == 3)
        {
            mainScreenPos[2] = mainScreenPos[1];
            mainScreenPos[1] = mainScreenPos[0];
            mainScreenPos[0] = NDS::PowerControl9 >> 15;

            int guess;
            if (mainScreenPos[0] == mainScreenPos[2] &&
                mainScreenPos[0] != mainScreenPos[1])
            {
                // constant flickering, likely displaying 3D on both screens
                // TODO: when both screens are used for 2D only...???
                guess = 0;
            }
            else
            {
                if (mainScreenPos[0] == 1)
                    guess = 1;
                else
                    guess = 2;
            }

            if (guess != autoScreenSizing)
            {
                autoScreenSizing = guess;
                //emit screenLayoutChange();
            }
        }

#ifdef OGLRENDERER_ENABLED
        if (videoRenderer == 1)
        {
            FrontBufferLock.lock();
            if (FrontBufferReverseSyncs[FrontBuffer ^ 1])
                glWaitSync(FrontBufferReverseSyncs[FrontBuffer ^ 1], 0, GL_TIMEOUT_IGNORED);
            FrontBufferLock.unlock();
        }
#endif

        // emulate
        static unsigned long frame = 0;
        //printf("frame %d\n", frame++);

        u32 nlines = NDS::RunFrame();

        FrontBufferLock.lock();
        FrontBuffer = GPU::FrontBuffer;
#ifdef OGLRENDERER_ENABLED
        if (videoRenderer == 1)
        {
            if (FrontBufferSyncs[FrontBuffer])
                glDeleteSync(FrontBufferSyncs[FrontBuffer]);
            FrontBufferSyncs[FrontBuffer] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
            // this is hacky but this is the easiest way to call
            // this function without dealling with a ton of
            // macro mess
            //epoxy_glFlush();
            glFlush();
        }
#endif
        FrontBufferLock.unlock();

#ifdef MELONCAP
        MelonCap::Update();
#endif // MELONCAP

        if (EmuRunning == 0) return;

        //emit windowUpdate();
        //panelGL->paintGL();

        bool fastforward = Input::HotkeyDown(HK_FastForward);

        if (Config::AudioSync && (!fastforward) && audioDevice)
        {
           //SDL_LockMutex(audioSyncLock);
           while (SPU::GetOutputSize() > 1024)
           {
               int ret = SDL_CondWaitTimeout(audioSync, audioSyncLock, 500);
               if (ret == SDL_MUTEX_TIMEDOUT) break;
           }
           //SDL_UnlockMutex(audioSyncLock);
        }

        double frametimeStep = nlines / (60.0 * 263.0);

        {
            bool limitfps = Config::LimitFPS && !fastforward;

            double practicalFramelimit = limitfps ? frametimeStep : 1.0 / 1000.0;

            double curtime = SDL_GetPerformanceCounter() * perfCountsSec;

            frameLimitError += practicalFramelimit - (curtime - lastTime);
            if (frameLimitError < -practicalFramelimit)
                frameLimitError = -practicalFramelimit;
            if (frameLimitError > practicalFramelimit)
                frameLimitError = practicalFramelimit;

            if (round(frameLimitError * 1000.0) > 0.0)
            {
                SDL_Delay(round(frameLimitError * 1000.0));
                double timeBeforeSleep = curtime;
                curtime = SDL_GetPerformanceCounter() * perfCountsSec;
                frameLimitError -= curtime - timeBeforeSleep;
            }

            lastTime = curtime;
        }

        nframes++;
        if (nframes >= 30)
        {
            double time = SDL_GetPerformanceCounter() * perfCountsSec;
            double dt = time - lastMeasureTime;
            lastMeasureTime = time;

            u32 fps = round(nframes / dt);
            nframes = 0;

            float fpstarget = 1.0 / frametimeStep;

            sprintf(melontitle, "[%d/%.0f] melonDS " MELONDS_VERSION, fps, fpstarget);
            changeWindowTitle(melontitle);
        }
    }
    else
    {
        // paused
        nframes = 0;
        lastTime = SDL_GetPerformanceCounter() * perfCountsSec;
        lastMeasureTime = lastTime;

        //emit windowUpdate();

        EmuStatus = EmuRunning;

        sprintf(melontitle, "melonDS " MELONDS_VERSION);
        changeWindowTitle(melontitle);

        SDL_Delay(75);
    }
}

void EmuThread::changeWindowTitle(char* title)
{
    //emit windowTitleChange(QString(title));
}

void EmuThread::emuRun()
{
    EmuRunning = 1;
    EmuPause = 0;
    RunningSomething = true;

    // checkme
    //emit windowEmuStart();
    if (audioDevice) SDL_PauseAudioDevice(audioDevice, 0);
    if (micDevice)   SDL_PauseAudioDevice(micDevice, 0);

    //run();
}

void EmuThread::emuPause()
{
    EmuPause++;
    if (EmuPause > 1) return;

    PrevEmuStatus = EmuRunning;
    EmuRunning = 2;
    while (EmuStatus != 2);

    //if (audioDevice) SDL_PauseAudioDevice(audioDevice, 1);
    //if (micDevice)   SDL_PauseAudioDevice(micDevice, 1);
}

void EmuThread::emuUnpause()
{
    if (EmuPause < 1) return;

    EmuPause--;
    if (EmuPause > 0) return;

    EmuRunning = PrevEmuStatus;

    if (audioDevice) SDL_PauseAudioDevice(audioDevice, 0);
    if (micDevice)   SDL_PauseAudioDevice(micDevice, 0);
}

void EmuThread::emuStop()
{
    EmuRunning = 0;
    EmuPause = 0;

    //if (audioDevice) SDL_PauseAudioDevice(audioDevice, 1);
    //if (micDevice)   SDL_PauseAudioDevice(micDevice, 1);
}

//bool EmuThread::emuIsRunning()
//{
//    return (EmuRunning == 1);
//}

ScreenPanelGL::ScreenPanelGL()
{
    touching = false;

    initializeGL();
    setupScreenLayout();
}

void ScreenPanelGL::screenSetupLayout(int w, int h)
{
    int sizing = Config::ScreenSizing;
    if (sizing == 3) sizing = autoScreenSizing;

    float aspectRatios[] =
    {
        1.f,
        (16.f / 9) / (4.f / 3),
        (21.f / 9) / (4.f / 3),
        ((float)w / h) / (4.f / 3)
    };

    Frontend::SetupScreenLayout(w, h,
        Config::ScreenLayout,
        Config::ScreenRotation,
        sizing,
        Config::ScreenGap,
        Config::IntegerScaling != 0,
        Config::ScreenSwap != 0,
        aspectRatios[Config::ScreenAspectTop],
        aspectRatios[Config::ScreenAspectBot]);

    numScreens = Frontend::GetScreenTransforms(screenMatrix[0], screenKind);
}

void ScreenPanelGL::setupScreenLayout()
{
    int w, h;
    SDL_GetWindowSize(mainWindow, &w, &h);
    //int w = Config::WindowWidth;
    //int h = Config::WindowHeight;

    screenSetupLayout(w, h);
}

EM_BOOL fullscreenchange_callback(int eventType, const EmscriptenFullscreenChangeEvent *e, void *userData)
{
//   printf("%s, isFullscreen: %d, fullscreenEnabled: %d, fs element nodeName: \"%s\", fs element id: \"%s\". New size: %dx%d pixels. Screen size: %dx%d pixels.\n",
//     emscripten_event_type_to_string(eventType), e->isFullscreen, e->fullscreenEnabled, e->nodeName, e->id, e->elementWidth, e->elementHeight, e->screenWidth, e->screenHeight);
//   ++callCount;
//   if (callCount == 1) { // Transitioned to fullscreen.
//     if (!e->isFullscreen) {
//       report_result(1);
//     }
//   } else if (callCount == 2) { // Transitioned to windowed, we must be back to the default pixel size 300x150.
//     if (e->isFullscreen || e->elementWidth != 300 || e->elementHeight != 150) {
//       report_result(1);
//     } else {
//       report_result(0);
//     }
//   }
    //EM_ASM(alert("fullscreen"););
    //printf("fullscreen\n");
    //emscripten_request_fullscreen("canvas", 1);
    //int width = 640;
    //int height = 480;
    //SDL_SetWindowSize(mainWindow, width, height);
    panelGL->setupScreenLayout();
    //EM_ASM(setTimeout(function(){var canvas = document.getElementById('canvas'); canvas.width = $0; canvas.height = $1;}, 50), width, height);
    return 0;
}

void ScreenPanelGL::initializeGL()
{
    //initializeOpenGLFunctions();

    if (glewInit() != GLEW_OK)
    {
        printf("glew broke\n");
        //assert(false);
    }

    const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
    const GLubyte* version = glGetString(GL_VERSION); // version as a string
    printf("OpenGL: renderer: %s\n", renderer);
    printf("OpenGL: version: %s\n", version);

    glClearColor(0, 0, 0, 1);

    screenShader = new Shader(kScreenVS, kScreenFS);
    //screenShader = new QOpenGLShaderProgram(this);
    //screenShader->addShaderFromSourceCode(QOpenGLShader::Vertex, kScreenVS);
    //screenShader->addShaderFromSourceCode(QOpenGLShader::Fragment, kScreenFS);

    GLuint pid = screenShader->ID;
    glBindAttribLocation(pid, 0, "vPosition");
    glBindAttribLocation(pid, 1, "vTexcoord");
    glBindFragDataLocation(pid, 0, "oColor");

    //screenShader->link();

    screenShader->use();
    //screenShader->setUniformValue("ScreenTex", (GLint)0);
    screenShader->setInt("ScreenTex", (GLint)0);
    screenShader->release();

    // to prevent bleeding between both parts of the screen
    // with bilinear filtering enabled
    const int paddedHeight = 192 * 2 + 2;
    const float padPixels = 1.f / paddedHeight;

    const float vertices[] =
    {
        0.f,   0.f,    0.f, 0.f,
        0.f,   192.f,  0.f, 0.5f - padPixels,
        256.f, 192.f,  1.f, 0.5f - padPixels,
        0.f,   0.f,    0.f, 0.f,
        256.f, 192.f,  1.f, 0.5f - padPixels,
        256.f, 0.f,    1.f, 0.f,

        0.f,   0.f,    0.f, 0.5f + padPixels,
        0.f,   192.f,  0.f, 1.f,
        256.f, 192.f,  1.f, 1.f,
        0.f,   0.f,    0.f, 0.5f + padPixels,
        256.f, 192.f,  1.f, 1.f,
        256.f, 0.f,    1.f, 0.5f + padPixels
    };

    glGenBuffers(1, &screenVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, screenVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &screenVertexArray);
    glBindVertexArray(screenVertexArray);
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * 4, (void*)(0));
    glEnableVertexAttribArray(1); // texcoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * 4, (void*)(2 * 4));

    glGenTextures(1, &screenTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, paddedHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    // fill the padding
    u8 zeroData[256 * 4 * 4];
    memset(zeroData, 0, sizeof(zeroData));
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 192, 256, 2, GL_RGBA, GL_UNSIGNED_BYTE, zeroData);

    //OSD::Init(this);
    OSD::Init();
}

void ScreenPanelGL::paintGL()
{
    //printf("painting\n");
    SDL_GL_MakeCurrent(mainWindow, glcontext);

    // int w = Config::WindowWidth;
    // int h = Config::WindowHeight;
    int w, h;
    SDL_GetWindowSize(mainWindow, &w, &h);
    //float factor = devicePixelRatioF();
    float factor = 1.0f;

    //glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(0, 0, w * factor, h * factor);

    if (emuThread)
    {
        screenShader->use();

        screenShader->setVec2("uScreenSize", (float)w, (float)h);
        screenShader->setFloat("uScaleFactor", factor);

        //screenShader->setUniformValue("uScreenSize", (float)w, (float)h);
        //screenShader->setUniformValue("uScaleFactor", factor);

        emuThread->FrontBufferLock.lock();
        int frontbuf = emuThread->FrontBuffer;
        glActiveTexture(GL_TEXTURE0);

#ifdef OGLRENDERER_ENABLED
        if (GPU::Renderer != 0)
        {
            if (emuThread->FrontBufferSyncs[emuThread->FrontBuffer])
                glWaitSync(emuThread->FrontBufferSyncs[emuThread->FrontBuffer], 0, GL_TIMEOUT_IGNORED);
            // hardware-accelerated render
            GPU::CurGLCompositor->BindOutputTexture(frontbuf);
        }
        else
#endif
        {
            // regular render
            glBindTexture(GL_TEXTURE_2D, screenTexture);

            if (GPU::Framebuffer[frontbuf][0] && GPU::Framebuffer[frontbuf][1])
            {
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 192, GL_RGBA,
                    GL_UNSIGNED_BYTE, GPU::Framebuffer[frontbuf][0]);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 192 + 2, 256, 192, GL_RGBA,
                    GL_UNSIGNED_BYTE, GPU::Framebuffer[frontbuf][1]);
            }
        }

        GLint filter = Config::ScreenFilter ? GL_LINEAR : GL_NEAREST;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

        glBindBuffer(GL_ARRAY_BUFFER, screenVertexBuffer);
        glBindVertexArray(screenVertexArray);

        GLint transloc = screenShader->uniformLocation("uTransform");

        for (int i = 0; i < numScreens; i++)
        {
            glUniformMatrix2x3fv(transloc, 1, GL_TRUE, screenMatrix[i]);
            glDrawArrays(GL_TRIANGLES, screenKind[i] == 0 ? 0 : 2 * 3, 2 * 3);
        }

        screenShader->release();

        if (emuThread->FrontBufferReverseSyncs[emuThread->FrontBuffer])
            glDeleteSync(emuThread->FrontBufferReverseSyncs[emuThread->FrontBuffer]);
        emuThread->FrontBufferReverseSyncs[emuThread->FrontBuffer] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        emuThread->FrontBufferLock.unlock();
    }

    //OSD::Update(this);
    //OSD::DrawGL(this, w * factor, h * factor);

    //OSD::Update();
    //OSD::DrawGL(w * factor, h * factor);

    SDL_GL_SwapWindow(mainWindow);
}

void keyPressEvent(SDL_KeyboardEvent* event)
{
    //if (event->repeat) return;

    // TODO!! REMOVE ME IN RELEASE BUILDS!!
    //if (event->key() == Qt::Key_F11) NDS::debug(0);

    Input::KeyPress(event);
}

void keyReleaseEvent(SDL_KeyboardEvent* event)
{
    //if (event->repeat) return;

    Input::KeyRelease(event);
}

void onMousePress(SDL_MouseButtonEvent* event)
{
    //event->accept();
    //if (event->button() != Qt::LeftButton) return;
    if(event->button != SDL_BUTTON_LEFT) return;

    int x = event->x;
    int y = event->y;

    if (Frontend::GetTouchCoords(x, y))
    {
        panelGL->touching = true;
        NDS::TouchScreen(x, y);
    }
}

void onMouseRelease(SDL_MouseButtonEvent* event)
{
    //event->accept();
    //if (event->button() != Qt::LeftButton) return;
    if(event->button != SDL_BUTTON_LEFT) return;

    if (panelGL->touching)
    {
        panelGL->touching = false;
        NDS::ReleaseScreen();
    }
}

void onMouseMove(SDL_MouseMotionEvent* event)
{
    //event->accept();
    
    //showCursor();

    if (!(event->state & SDL_BUTTON_LMASK)) return;
    if (!panelGL->touching) return;

    int x = event->x;
    int y = event->y;

    Frontend::GetTouchCoords(x, y);

    // clamp to screen range
    if (x < 0) x = 0;
    else if (x > 255) x = 255;
    if (y < 0) y = 0;
    else if (y > 191) y = 191;

    NDS::TouchScreen(x, y);
}

//https://emscripten.org/docs/porting/connecting_cpp_and_javascript/Interacting-with-code.html
extern "C"
{
    int file_drag(const char* str)
    {
        emuThread->start(str);
        return 0;
    }

    int triggerFullscreen()
    {
        emscripten_request_fullscreen("canvas", 1);
        return 0;
    }
}

int main(int argc, char** argv)
{
    srand(time(NULL));

    printf("melonDS " MELONDS_VERSION "\n");
    printf(MELONDS_URL "\n");

    Platform::Init(argc, argv);

    //QApplication melon(argc, argv);
    //melon.setWindowIcon(QIcon(":/melon-icon"));

    // http://stackoverflow.com/questions/14543333/joystick-wont-work-using-sdl
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    if (SDL_Init(SDL_INIT_HAPTIC) < 0)
    {
        printf("SDL couldn't init rumble\n");
    }
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0)
    {
        //QMessageBox::critical(NULL, "melonDS", "SDL shat itself :(");
        printf("SDL shat itself :(\n");
        return 1;
    }

    SDL_JoystickEventState(SDL_ENABLE);

    Config::Load();

#define SANITIZE(var, min, max)  { var = std::clamp(var, min, max); }
    SANITIZE(Config::ConsoleType, 0, 1);
    SANITIZE(Config::_3DRenderer, 0, 1);
    SANITIZE(Config::ScreenVSyncInterval, 1, 20);
    SANITIZE(Config::GL_ScaleFactor, 1, 16);
    SANITIZE(Config::AudioVolume, 0, 256);
    SANITIZE(Config::MicInputType, 0, 3);
    SANITIZE(Config::ScreenRotation, 0, 3);
    SANITIZE(Config::ScreenGap, 0, 500);
    SANITIZE(Config::ScreenLayout, 0, 3);
    SANITIZE(Config::ScreenSizing, 0, 5);
    SANITIZE(Config::ScreenAspectTop, 0, 4);
    SANITIZE(Config::ScreenAspectBot, 0, 4);
#undef SANITIZE

    audioSync = SDL_CreateCond();
    audioSyncLock = SDL_CreateMutex();

    audioFreq = 48000; // TODO: make configurable?
    SDL_AudioSpec whatIwant, whatIget;
    memset(&whatIwant, 0, sizeof(SDL_AudioSpec));
    whatIwant.freq = audioFreq;
    whatIwant.format = AUDIO_S16LSB;
    whatIwant.channels = 2;
    whatIwant.samples = 1024;
    whatIwant.callback = audioCallback;
    audioDevice = SDL_OpenAudioDevice(NULL, 0, &whatIwant, &whatIget, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (!audioDevice)
    {
        printf("Audio init failed: %s\n", SDL_GetError());
    }
    else
    {
        audioFreq = whatIget.freq;
        printf("Audio output frequency: %d Hz\n", audioFreq);
        //SDL_PauseAudioDevice(audioDevice, 1);
    }

    memset(&whatIwant, 0, sizeof(SDL_AudioSpec));
    whatIwant.freq = 44100;
    whatIwant.format = AUDIO_S16LSB;
    whatIwant.channels = 1;
    whatIwant.samples = 1024;
    whatIwant.callback = micCallback;
    micDevice = SDL_OpenAudioDevice(NULL, 1, &whatIwant, &whatIget, 0);
    if (!micDevice)
    {
        printf("Mic init failed: %s\n", SDL_GetError());
    }
    else
    {
        //SDL_PauseAudioDevice(micDevice, 1);
    }


    memset(micExtBuffer, 0, sizeof(micExtBuffer));
    micExtBufferWritePos = 0;
    micWavBuffer = nullptr;

    Frontend::Init_ROM();
    Frontend::EnableCheats(Config::EnableCheats != 0);

    Frontend::Init_Audio(audioFreq);

    if (Config::MicInputType == 1)
    {
        Frontend::Mic_SetExternalBuffer(micExtBuffer, sizeof(micExtBuffer) / sizeof(s16));
    }
    //else if (Config::MicInputType == 3)
    //{
    //    micLoadWav(Config::MicWavPath);
    //    Frontend::Mic_SetExternalBuffer(micWavBuffer, micWavLength);
    //}

    Input::JoystickID = Config::JoystickID;
    Input::OpenJoystick();

    //mainWindow = new MainWindow();
    mainWindow = SDL_CreateWindow("emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, Config::WindowWidth, Config::WindowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetSwapInterval(0);
    glcontext = SDL_GL_CreateContext(mainWindow);
    SDL_GL_MakeCurrent(mainWindow, glcontext);
    if (glewInit() != GLEW_OK)
    {
        std::cout << "GLEW failed to init.\n";
    }
    sdl_renderer = SDL_CreateRenderer(mainWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

    panelGL = new ScreenPanelGL();

    //SDL_SetWindowSize(mainWindow, 640, 480);
    emscripten_set_fullscreenchange_callback("canvas", 0, 1, fullscreenchange_callback);
    //emscripten_request_fullscreen("canvas", 1);

    emuThread = new EmuThread();
    

    EM_ASM(
        function dragover_handler(event) {
            event.preventDefault();
            event.dataTransfer.dropEffect = 'move';
        }

        function dropHandler(event) {
            event.preventDefault();
            //Module._file_drag();
            //alert('drag event');
            let loadedFiles = 0;
            let maxFiles = event.dataTransfer.files.length;

            //for(let i = 0; i < event.dataTransfer.files.length; i++) {
            const file = event.dataTransfer.files[0];
            let reader = new FileReader();
            reader.readAsArrayBuffer(file);

            reader.onload = function() {
                let data = new Uint8Array(reader.result);
                console.log("filename " + file.name);
                let filename = file.name;

                let stream = FS.open(filename, 'w+');
                FS.write(stream, data, 0, data.length, 0);
                FS.close(stream);

                if(++loadedFiles == maxFiles) {
                    console.log("Load complete.");
                    Module.ccall('file_drag', // name of C function
                        'number', // return type
                        ['string'], // argument types
                        [filename]);
                }

                console.log("Loaded file " + loadedFiles.toString() + "/" + maxFiles.toString());
            };
            //}
            //alert("drag data " + data);
        }
        let em_canvas = document.getElementById('canvas');

        em_canvas.addEventListener('dragover', dragover_handler, false);
        em_canvas.addEventListener('drop', dropHandler, false);
        

        let fs_button = document.getElementById('fullscreen');
        if(fs_button) {
            fs_button.onclick = function() {
                Module.ccall('triggerFullscreen');
            }
        }
    );

    //emuThread->emuPause();

    //QObject::connect(&melon, &QApplication::applicationStateChanged, mainWindow, &MainWindow::onAppStateChanged);

    //char* file = "G:/melonDS-emscripten/melonDS/build/Debug/mkds.nds";

    //int res = Frontend::LoadROM(file, Frontend::ROMSlot_NDS);
    //if (res == Frontend::Load_OK)
    //{
    //    emuThread->emuRun();
    //}
        

    //int ret = melon.exec();

    //emuThread->emuStop();
    ////emuThread->wait();
    //delete emuThread;

    //Input::CloseJoystick();

    //Frontend::DeInit_ROM();

    //if (audioDevice) SDL_CloseAudioDevice(audioDevice);
    //if (micDevice)   SDL_CloseAudioDevice(micDevice);

    //SDL_DestroyCond(audioSync);
    //SDL_DestroyMutex(audioSyncLock);

    //if (micWavBuffer) delete[] micWavBuffer;

    //Config::Save();

    //SDL_Quit();
    //Platform::DeInit();
    //return ret;
    return 0;
}
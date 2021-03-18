#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifdef OGLRENDERER_ENABLED
#include "OpenGLSupport.h"
#endif

#include "main.h"

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

#include "main_shaders.h"

bool RunningSomething;

class EmuThread;
class ScreenPanelGL;

EmuThread* emuThread;
ScreenPanelGL* panelGL;

int autoScreenSizing = 0;

int videoRenderer;
GPU::RenderSettings videoSettings;
bool videoSettingsDirty;

SDL_Window* window;
SDL_GLContext glcontext;

SDL_AudioDeviceID audioDevice;
int audioFreq;
SDL_cond* audioSync;
SDL_mutex* audioSyncLock;

SDL_AudioDeviceID micDevice;
s16 micExtBuffer[2048];
u32 micExtBufferWritePos;

u32 micWavLength;
s16* micWavBuffer;

ScreenPanelGL::ScreenPanelGL()
{
    printf("Initialize gl panel\n");
    touching = false;
    //screen = SDL_SetVideoMode(Config::WindowWidth, Config::WindowHeight, 0, SDL_OPENGL);
    
}

ScreenPanelGL::~ScreenPanelGL()
{
    //mouseTimer->stop();
    SDL_GL_MakeCurrent(window, glcontext);
    OSD::DeInit();

    glDeleteTextures(1, &screenTexture);

    glDeleteVertexArrays(1, &screenVertexArray);
    glDeleteBuffers(1, &screenVertexBuffer);

    delete screenShader;
}

void ScreenPanelGL::initializeGL()
{
    const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
    const GLubyte* version = glGetString(GL_VERSION); // version as a string
    printf("OpenGL: renderer: %s\n", renderer);
    printf("OpenGL: version: %s\n", version);

    glClearColor(0, 0, 0, 1);

    //screenShader = new QOpenGLShaderProgram(this);
    //screenShader->addShaderFromSourceCode(QOpenGLShader::Vertex, kScreenVS);
    //screenShader->addShaderFromSourceCode(QOpenGLShader::Fragment, kScreenFS);
    screenShader = new Shader(kScreenVS, kScreenFS);

    GLuint pid = screenShader->ID;
    glBindAttribLocation(pid, 0, "vPosition");
    glBindAttribLocation(pid, 1, "vTexcoord");
    glBindFragDataLocation(pid, 0, "oColor");

    //screenShader->link();

    screenShader->use();
    screenShader->setInt("ScreenTex", (GLint)0);
    screenShader->release();

    // to prevent bleeding between both parts of the screen
    // with bilinear filtering enabled
    const int paddedHeight = 192*2+2;
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
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*4, (void*)(0));
    glEnableVertexAttribArray(1); // texcoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*4, (void*)(2*4));

    glGenTextures(1, &screenTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, paddedHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    // fill the padding
    u8 zeroData[256*4*4];
    memset(zeroData, 0, sizeof(zeroData));
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 192, 256, 2, GL_RGBA, GL_UNSIGNED_BYTE, zeroData);

    OSD::Init();
}

void ScreenPanelGL::paintGL()
{
    int w = Config::WindowWidth;
    int h = Config::WindowHeight;
    float factor = 1.0f;

    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(0, 0, w*factor, h*factor);

    if (emuThread)
    {
        screenShader->use();

        screenShader->setVec2("uScreenSize", (float)w, (float)h);
        screenShader->setFloat("uScaleFactor", factor);

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
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 192+2, 256, 192, GL_RGBA,
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
            glDrawArrays(GL_TRIANGLES, screenKind[i] == 0 ? 0 : 2*3, 2*3);
        }

        screenShader->release();

        if (emuThread->FrontBufferReverseSyncs[emuThread->FrontBuffer])
            glDeleteSync(emuThread->FrontBufferReverseSyncs[emuThread->FrontBuffer]);
        emuThread->FrontBufferReverseSyncs[emuThread->FrontBuffer] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        emuThread->FrontBufferLock.unlock();
    }

    OSD::Update();
    OSD::DrawGL(w*factor, h*factor);
}

void ScreenPanelGL::resizeGL(int w, int h)
{

}

void ScreenPanelGL::setupScreenLayout()
{

}

void windowUpdate()
{
    //SDL_GL_SwapBuffers();
}

EmuThread()
{
    EmuStatus = 0;
    EmuRunning = 2;
    EmuPause = 0;
    RunningSomething = false;
    _thread = nullptr;

    initOpenGL();
}

void EmuThread::initOpenGL()
{
    glcontext = SDL_GL_CreateContext(window);
    panelGL = new ScreenPanelGL();
}

void EmuThread::deinitOpenGL()
{
    delete panelGL;
    SDL_GL_DeleteContext(glcontext);
}

void EmuThread::start()
{
    printf("starting emulator thread\n");
    _thread = new std::thread( [this] { this->run(); } );
}

void EmuThread::run()
{
    bool hasOGL = true;
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
        //oglContext->makeCurrent(oglSurface);
        SDL_GL_MakeCurrent(window, glcontext);
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

    while (EmuRunning != 0)
    {
        Input::Process();

        // if (Input::HotkeyPressed(HK_FastForwardToggle)) emit windowLimitFPSChange();

        // if (Input::HotkeyPressed(HK_Pause)) emit windowEmuPause();
        // if (Input::HotkeyPressed(HK_Reset)) emit windowEmuReset();

        // if (Input::HotkeyPressed(HK_FullscreenToggle)) emit windowFullscreenToggle();

        // if (Input::HotkeyPressed(HK_SwapScreens)) emit swapScreensToggle();

        // if (GBACart::CartInserted && GBACart::HasSolarSensor)
        // {
        //     if (Input::HotkeyPressed(HK_SolarSensorDecrease))
        //     {
        //         if (GBACart_SolarSensor::LightLevel > 0) GBACart_SolarSensor::LightLevel--;
        //         char msg[64];
        //         sprintf(msg, "Solar sensor level set to %d", GBACart_SolarSensor::LightLevel);
        //         OSD::AddMessage(0, msg);
        //     }
        //     if (Input::HotkeyPressed(HK_SolarSensorIncrease))
        //     {
        //         if (GBACart_SolarSensor::LightLevel < 10) GBACart_SolarSensor::LightLevel++;
        //         char msg[64];
        //         sprintf(msg, "Solar sensor level set to %d", GBACart_SolarSensor::LightLevel);
        //         OSD::AddMessage(0, msg);
        //     }
        // }

        if (EmuRunning == 1)
        {
            EmuStatus = 1;

            // update render settings if needed
            if (videoSettingsDirty)
            {
                if (hasOGL != mainWindow->hasOGL)
                {
                    hasOGL = mainWindow->hasOGL;
#ifdef OGLRENDERER_ENABLED
                    if (hasOGL)
                    {
                        //oglContext->makeCurrent(oglSurface);
                        SDL_GL_MakeCurrent(window, glcontext);
                        videoRenderer = Config::_3DRenderer;
                    }
                    else
#endif
                    {
                        videoRenderer = 0;
                    }
                }
                else
                    videoRenderer = hasOGL ? Config::_3DRenderer : 0;

                videoSettingsDirty = false;

                videoSettings.Soft_Threaded = Config::Threaded3D != 0;
                videoSettings.GL_ScaleFactor = Config::GL_ScaleFactor;
                videoSettings.GL_BetterPolygons = Config::GL_BetterPolygons;

                GPU::SetRenderSettings(videoRenderer, videoSettings);
            }

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
                epoxy_glFlush();
            }
#endif
            FrontBufferLock.unlock();

#ifdef MELONCAP
            MelonCap::Update();
#endif // MELONCAP

            if (EmuRunning == 0) break;

            windowUpdate();

            bool fastforward = Input::HotkeyDown(HK_FastForward);

            if (Config::AudioSync && (!fastforward) && audioDevice)
            {
                SDL_LockMutex(audioSyncLock);
                while (SPU::GetOutputSize() > 1024)
                {
                    int ret = SDL_CondWaitTimeout(audioSync, audioSyncLock, 500);
                    if (ret == SDL_MUTEX_TIMEDOUT) break;
                }
                SDL_UnlockMutex(audioSyncLock);
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

                float fpstarget = 1.0/frametimeStep;

                sprintf(melontitle, "[%d/%.0f] melonDS " MELONDS_VERSION, fps, fpstarget);
                //changeWindowTitle(melontitle);
                emscripten_set_window_title(melontitle);
            }
        }
        else
        {
            // paused
            nframes = 0;
            lastTime = SDL_GetPerformanceCounter() * perfCountsSec;
            lastMeasureTime = lastTime;

            windowUpdate();

            EmuStatus = EmuRunning;

            sprintf(melontitle, "melonDS " MELONDS_VERSION);
            //changeWindowTitle(melontitle);
            emscripten_set_window_title(melontitle);

            SDL_Delay(75);
        }
    }

    EmuStatus = 0;

    GPU::DeInitRenderer();
    NDS::DeInit();
    //Platform::LAN_DeInit();

    if (hasOGL)
    {
        //oglContext->doneCurrent();
        //deinitOpenGL();
    }
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
}

void EmuThread::emuPause()
{
    EmuPause++;
    if (EmuPause > 1) return;

    PrevEmuStatus = EmuRunning;
    EmuRunning = 2;
    while (EmuStatus != 2);

    if (audioDevice) SDL_PauseAudioDevice(audioDevice, 1);
    if (micDevice)   SDL_PauseAudioDevice(micDevice, 1);
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

    if (audioDevice) SDL_PauseAudioDevice(audioDevice, 1);
    if (micDevice)   SDL_PauseAudioDevice(micDevice, 1);
}


void audioCallback(void* data, Uint8* stream, int len)
{
    len /= (sizeof(s16) * 2);

    // resample incoming audio to match the output sample rate

    // int len_in = Frontend::AudioOut_GetNumSamples(len);
    // s16 buf_in[1024*2];
    // int num_in;

    // SDL_LockMutex(audioSyncLock);
    // num_in = SPU::ReadOutput(buf_in, len_in);
    // SDL_CondSignal(audioSync);
    // SDL_UnlockMutex(audioSyncLock);

    // if (num_in < 1)
    // {
    //     memset(stream, 0, len*sizeof(s16)*2);
    //     return;
    // }

    // int margin = 6;
    // if (num_in < len_in-margin)
    // {
    //     int last = num_in-1;

    //     for (int i = num_in; i < len_in-margin; i++)
    //         ((u32*)buf_in)[i] = ((u32*)buf_in)[last];

    //     num_in = len_in-margin;
    // }

    //Frontend::AudioOut_Resample(buf_in, num_in, (s16*)stream, len, Config::AudioVolume);
}

void micCallback(void* data, Uint8* stream, int len)
{
    s16* input = (s16*)stream;
    len /= sizeof(s16);

    int maxlen = sizeof(micExtBuffer) / sizeof(s16);

    if ((micExtBufferWritePos + len) > maxlen)
    {
        u32 len1 = maxlen - micExtBufferWritePos;
        memcpy(&micExtBuffer[micExtBufferWritePos], &input[0], len1*sizeof(s16));
        memcpy(&micExtBuffer[0], &input[len1], (len - len1)*sizeof(s16));
        micExtBufferWritePos = len - len1;
    }
    else
    {
        memcpy(&micExtBuffer[micExtBufferWritePos], input, len*sizeof(s16));
        micExtBufferWritePos += len;
    }
}

void emuStop()
{
    RunningSomething = false;

    Frontend::UnloadROM(Frontend::ROMSlot_NDS);
    Frontend::UnloadROM(Frontend::ROMSlot_GBA);

    //emit emuThread->windowEmuStop();

    printf("Shutdown");
}

int main(int argc, char** argv)
{
    srand(time(NULL));

    printf("melonDS " MELONDS_VERSION "\n");
    //printf(MELONDS_URL "\n");
    
    Platform::Init(argc, argv);

    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    if (SDL_Init(SDL_INIT_HAPTIC) < 0)
    {
        printf("SDL couldn't init rumble\n");
    }
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0)
    {
        printf("SDL BROKE!!!!!!!\n");
        return 1;
    }
    window = SDL_CreateWindow("emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, Config::WindowWidth, Config::windowHeight, SDL_WINDOW_OPENGL);
    
    SDL_JoystickEventState(SDL_ENABLE);

    Config::Load();

#define SANITIZE(var, min, max)  { var = std::clamp(var, min, max); }
    SANITIZE(Config::ConsoleType, 0, 1);
    SANITIZE(Config::_3DRenderer,
    0,
    0 // Minimum, Software renderer
    #ifdef OGLRENDERER_ENABLED
    + 1 // OpenGL Renderer
    #endif
    );
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
        SDL_PauseAudioDevice(audioDevice, 1);
    }
    printf("init mic\n");
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
        SDL_PauseAudioDevice(micDevice, 1);
    }
    printf("end init mic\n");

    memset(micExtBuffer, 0, sizeof(micExtBuffer));
    micExtBufferWritePos = 0;
    micWavBuffer = nullptr;

    printf("Platform init\n");
    Platform::Init(argc, argv);

    Frontend::Init_ROM();
    Frontend::EnableCheats(Config::EnableCheats != 0);

    Frontend::Init_Audio(audioFreq);

    // if (Config::MicInputType == 1)
    // {
    //     Frontend::Mic_SetExternalBuffer(micExtBuffer, sizeof(micExtBuffer)/sizeof(s16));
    // }
    // else if (Config::MicInputType == 3)
    // {
    //     micLoadWav(Config::MicWavPath);
    //     Frontend::Mic_SetExternalBuffer(micWavBuffer, micWavLength);
    // }

    //Input::JoystickID = Config::JoystickID;
    //Input::OpenJoystick();

    //screenPanel = new ScreenPanelGL();

    emuThread = new EmuThread();
    emuThread->start();
    emuThread->emuPause();


    printf("begin load\n");
    int res = Frontend::LoadROM("mkds.nds", Frontend::ROMSlot_NDS);

    if (res == Frontend::Load_OK)
    {
        emuThread->emuRun();
        printf("ROM loaded, run emulator\n");
    }
    else
        printf("ROM load failed, error %d\n", res);


    //delete ScreenPanel;
    printf("ROM load end\n");
}
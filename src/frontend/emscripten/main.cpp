#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#include <vector>
#include <string>
#include <algorithm>

#include <SDL2/SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifdef OGLRENDERER_ENABLED
#include "OpenGLSupport.h"
#endif

#include "FrontendUtil.h"

#include "NDS.h"
#include "GBACart.h"
#include "GPU.h"
#include "SPU.h"
#include "Wifi.h"
#include "Platform.h"
#include "Config.h"
//#include "PlatformConfig.h"

bool RunningSomething;

//EmuThread* emuThread;

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


#define AUDIO_VOLUME 128

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

    //Frontend::AudioOut_Resample(buf_in, num_in, (s16*)stream, len, AUDIO_VOLUME);
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

int main(int argc, char** argv)
{
    srand(time(NULL));

    printf("melonDS " MELONDS_VERSION "\n");
    //printf(MELONDS_URL "\n");
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
    SDL_JoystickEventState(SDL_ENABLE);

    //Config::Load();

#define SANITIZE(var, min, max)  { var = std::clamp(var, min, max); }
    // SANITIZE(Config::ConsoleType, 0, 1);
    // SANITIZE(Config::_3DRenderer,
    // 0,
    // 0 // Minimum, Software renderer
    // #ifdef OGLRENDERER_ENABLED
    // + 1 // OpenGL Renderer
    // #endif
    // );
    // SANITIZE(Config::ScreenVSyncInterval, 1, 20);
    // SANITIZE(Config::GL_ScaleFactor, 1, 16);
    // SANITIZE(Config::AudioVolume, 0, 256);
    // SANITIZE(Config::MicInputType, 0, 3);
    // SANITIZE(Config::ScreenRotation, 0, 3);
    // SANITIZE(Config::ScreenGap, 0, 500);
    //SANITIZE(Config::ScreenLayout, 0, 3);
    //SANITIZE(Config::ScreenSizing, 0, 5);
    //SANITIZE(Config::ScreenAspectTop, 0, 4);
    //SANITIZE(Config::ScreenAspectBot, 0, 4);
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


    memset(micExtBuffer, 0, sizeof(micExtBuffer));
    micExtBufferWritePos = 0;
    micWavBuffer = nullptr;
    //Platform::Init(argc, argv);

    Frontend::Init_ROM();
    Frontend::EnableCheats(0);

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


    int res = Frontend::LoadROM("mkds.nds", Frontend::ROMSlot_NDS);

    if (res == Frontend::Load_OK)
    {
        //emuThread->emuRun();
        printf("ROM loaded, run emulator\n");
    }
}
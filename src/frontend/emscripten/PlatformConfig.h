
#ifndef PLATFORMCONFIG_H
#define PLATFORMCONFIG_H

#include "Config.h"

enum
{
    HK_Lid = 0,
    HK_Mic,
    HK_Pause,
    HK_Reset,
    HK_FastForward,
    HK_FastForwardToggle,
    HK_FullscreenToggle,
    HK_SwapScreens,
    HK_SolarSensorDecrease,
    HK_SolarSensorIncrease,
    HK_MAX
};

namespace Config
{

extern int KeyMapping[12];
extern int JoyMapping[12];

extern int HKKeyMapping[HK_MAX];
extern int HKJoyMapping[HK_MAX];

extern int JoystickID;

extern int WindowWidth;
extern int WindowHeight;
extern int WindowMaximized;

extern int ScreenRotation;
extern int ScreenGap;
extern int ScreenLayout;
extern int ScreenSwap;
extern int ScreenSizing;
extern int ScreenAspectTop;
extern int ScreenAspectBot;
extern int IntegerScaling;
extern int ScreenFilter;

extern int ScreenUseGL;
extern int ScreenVSync;
extern int ScreenVSyncInterval;

extern int _3DRenderer;
extern int Threaded3D;

extern int GL_ScaleFactor;
extern int GL_BetterPolygons;

extern int LimitFPS;
extern int AudioSync;
extern int ShowOSD;

extern int ConsoleType;
extern int DirectBoot;

extern int SocketBindAnyAddr;
extern char LANDevice[128];
extern int DirectLAN;

extern int SavestateRelocSRAM;

extern int AudioVolume;
extern int MicInputType;
extern char MicWavPath[1024];

extern char LastROMFolder[1024];

extern char RecentROMList[10][1024];

extern int EnableCheats;

extern int MouseHide;
extern int MouseHideSeconds;
extern int PauseLostFocus;

}

#endif
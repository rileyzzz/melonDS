
#include <vector>
#include <string>
#include <algorithm>
#include <thread>

#include <SDL2/SDL.h>

#include "Shader.h"
#include "FrontendUtil.h"

#ifndef MAIN_H
#define MAIN_H

class ScreenPanelGL
{
    float screenMatrix[Frontend::MaxScreenTransforms][6];
    int screenKind[Frontend::MaxScreenTransforms];
    int numScreens;

    bool touching;

    Shader* screenShader;
    GLuint screenVertexBuffer;
    GLuint screenVertexArray;
    GLuint screenTexture;

public:
    ScreenPanelGL();
    ~ScreenPanelGL();

    void initializeGL();
    void paintGL();
    //void resizeEvent(QResizeEvent* event) override;
    void resizeGL(int w, int h);
private:
    void setupScreenLayout();
};

class EmuThread
{
private:
    volatile int EmuStatus;
    int PrevEmuStatus;
    int EmuRunning;
    int EmuPause;
    std::thread* _thread;

public:
    int FrontBuffer = 0;
    std::mutex FrontBufferLock;

    GLsync FrontBufferReverseSyncs[2] = {nullptr, nullptr};
    GLsync FrontBufferSyncs[2] = {nullptr, nullptr};

    EmuThread();

    void initOpenGL();
    void deinitOpenGL();

    void start();
    void run();

    void emuRun();
    void emuPause();
    void emuUnpause();
    void emuStop();

    inline bool emuIsRunning() { return (EmuRunning == 1); }
};
#endif
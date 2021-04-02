
#include <vector>
#include <string>
#include <algorithm>
#include <thread>
#include <mutex>

//#include <epoxy/gl.h>
#include <SDL.h>

#include "Shader.h"
#include "FrontendUtil.h"

#ifndef MAIN_H
#define MAIN_H

extern SDL_mutex* save_mutex;
extern bool syncRequired;

void keyPressEvent(SDL_KeyboardEvent* event);
void keyReleaseEvent(SDL_KeyboardEvent* event);
void onMousePress(SDL_MouseButtonEvent* event);
void onMouseRelease(SDL_MouseButtonEvent* event);
void onMouseMove(SDL_MouseMotionEvent* event);

int startEmuMain();

class ScreenPanelGL
{
    float screenMatrix[Frontend::MaxScreenTransforms][6];
    int screenKind[Frontend::MaxScreenTransforms];
    int numScreens;

    Shader* screenShader;
    GLuint screenVertexBuffer;
    GLuint screenVertexArray;
    GLuint screenTexture;

public:
bool touching;

    ScreenPanelGL();
    ~ScreenPanelGL();

    void initializeGL();
    void paintGL();
    //void resizeEvent(QResizeEvent* event) override;
    void resizeGL(int w, int h);

    void setupScreenLayout();
    void screenSetupLayout(int w, int h);
};

class EmuThread
{
private:
    volatile int EmuStatus;
    int PrevEmuStatus;
    int EmuRunning;
    int EmuPause;
    std::thread* _thread;


    //render variables
    bool hasOGL = true;
    u32 mainScreenPos[3];
    u32 nframes;
    double perfCountsSec;
    double lastTime;
    double frameLimitError;
    double lastMeasureTime;
    char melontitle[100];
    SDL_GLContext oglContext;

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
    void frame();

    //void renderLoop();

    void changeWindowTitle(char* title);

    void emuRun();
    void emuPause();
    void emuUnpause();
    void emuStop();

    inline bool emuIsRunning() { return (EmuRunning == 1); }
};
#endif
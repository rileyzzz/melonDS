
#ifndef INPUT_H
#define INPUT_H

#include "types.h"

namespace Input
{

extern int JoystickID;
extern SDL_Joystick* Joystick;

extern u32 InputMask;

void Init();

// set joystickID before calling openJoystick()
void OpenJoystick();
void CloseJoystick();

//void KeyPress(QKeyEvent* event);
//void KeyRelease(QKeyEvent* event);

void Process();

bool HotkeyDown(int id);
bool HotkeyPressed(int id);
bool HotkeyReleased(int id);

//bool IsRightModKey(QKeyEvent* event);

}

#endif // INPUT_H
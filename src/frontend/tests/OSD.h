
#ifndef OSD_H
#define OSD_H

namespace OSD
{

bool Init();
void DeInit();

void AddMessage(u32 color, const char* text);

void Update();
void DrawNative();
void DrawGL(float w, float h);

}

#endif // OSD_H
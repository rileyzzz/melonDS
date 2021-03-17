#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <filesystem>

#define socket_t    int
#define sockaddr_t  struct sockaddr
#define closesocket close

#include "Platform.h"
//#include "PlatformConfig.h"

#ifndef INVALID_SOCKET
    #define INVALID_SOCKET  (socket_t)-1
#endif

char* EmuDirectory;

void emuStop();


namespace Platform
{

socket_t MPSocket;
sockaddr_t MPSendAddr;
u8 PacketBuffer[2048];

#define NIFI_VER 1

void Init(int argc, char** argv)
{
    EmuDirectory = "/";
}

void DeInit()
{
    delete[] EmuDirectory;
}

void StopEmu()
{
    emuStop();
}

FILE* OpenFile(const char* path, const char* mode, bool mustexist)
{
    std::filesystem::path f(path);
    if (mustexist && !std::filesystem::exists(path))
    {
        return nullptr;
    }

    QFile f(path);

    if (mustexist && !f.exists())
    {
        return nullptr;
    }

    QIODevice::OpenMode qmode;
    if (strlen(mode) > 1 && mode[0] == 'r' && mode[1] == '+')
    {
		qmode = QIODevice::OpenModeFlag::ReadWrite;
	}
	else if (strlen(mode) > 1 && mode[0] == 'w' && mode[1] == '+')
    {
    	qmode = QIODevice::OpenModeFlag::Truncate | QIODevice::OpenModeFlag::ReadWrite;
	}
	else if (mode[0] == 'w')
    {
        qmode = QIODevice::OpenModeFlag::Truncate | QIODevice::OpenModeFlag::WriteOnly;
    }
    else
    {
        qmode = QIODevice::OpenModeFlag::ReadOnly;
    }

    f.open(qmode);
    FILE* file = fdopen(dup(f.handle()), mode);
    f.close();

    return file;
}

}
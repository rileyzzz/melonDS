#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _MSC_VER
#include <unistd.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#endif

#ifdef _MSC_VER

#include <winsock2.h>
#include <WS2tcpip.h>
#endif

//#include <boost/interprocess/sync/interprocess_semaphore.hpp>
//#include <boost/sync/interprocess_semaphore.hpp>
#include <QSemaphore>

#include <fstream>
#include <filesystem>
#include <thread>
#include <mutex>
//#include <semaphore>

//#define bsemaphore boost::interprocess::interprocess_semaphore


#define socket_t    int
#define sockaddr_t  struct sockaddr

#ifndef _MSC_VER
#define closesocket close
#endif

#include "Platform.h"
#include "PlatformConfig.h"

#ifndef INVALID_SOCKET
#define INVALID_SOCKET  (socket_t)-1
#endif

char* EmuDirectory;

void emuStop();

#ifndef _MSC_VER
#define PLATFORM_SEPARATOR std::filesystem::path::preferred_separator
#else
#define PLATFORM_SEPARATOR "/"
#endif

namespace Platform
{

    socket_t MPSocket;
    sockaddr_t MPSendAddr;
    u8 PacketBuffer[2048];

#define NIFI_VER 1

    void Init(int argc, char** argv)
    {
        EmuDirectory = new char[255];
        strcpy(EmuDirectory, "G:/melonDS-emscripten/melonDS/build/Debug");

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
        printf("request open file %s\n", path);

        std::filesystem::path f(path);
        if (mustexist && !std::filesystem::exists(path))
        {
            return nullptr;
        }

        // std::ios::openmode smode;
        // if (strlen(mode) > 1 && mode[0] == 'r' && mode[1] == '+')
        //     smode = std::ios::in | std::ios::out;
        // else if (strlen(mode) > 1 && mode[0] == 'w' && mode[1] == '+')
        //     smode = std::ios::trunc | std::ios::in | std::ios::out;
        // else if (mode[0] == 'w')
        //     smode = std::ios::trunc | std::ios::out;
        // else
        //     smode = std::ios::in;

        // std::ifstream str(f, smode);
        // FILE* file = fdopen(dup(str.rdbuf()->fd(), mode);
        // str.close();
        FILE* file = fopen(path, mode);

        return file;
    }

    FILE* OpenLocalFile(const char* path, const char* mode)
    {
        std::filesystem::path dir(path);
        std::string fullpath;

        if (dir.is_absolute())
        {
            // If it's an absolute path, just open that.
            fullpath = path;
        }
        else
        {
            fullpath = std::string(EmuDirectory) + PLATFORM_SEPARATOR + path;
        }

        return OpenFile(fullpath.c_str(), mode, mode[0] != 'w');
    }

    Thread* Thread_Create(std::function<void()> func)
    {
        std::thread* t = new std::thread(func);
        //QThread* t = QThread::create(func);
        //t->start();
        return (Thread*)t;
    }

    void Thread_Free(Thread* thread)
    {
        std::thread* t = (std::thread*) thread;
        //t->terminate();
        delete t;
    }

    void Thread_Wait(Thread* thread)
    {
        ((std::thread*) thread)->join();
    }

    // class ESemaphore
    // {
    // private:
    //     std::mutex mtx;
    //     std::condition_variable cv;
    //     int count;

    // public:
    //     ESemaphore(int count_ = 0) : count(count_) { }

    //     inline void notify( int tid )
    //     {
    //         std::unique_lock<std::mutex> lock(mtx);
    //         count++;
    //         //notify the waiting thread
    //         cv.notify_one();
    //     }

    //     inline void wait( int tid )
    //     {
    //         std::unique_lock<std::mutex> lock(mtx);
    //         while(count == 0) {
    //             //wait on the mutex until notify is called
    //             cv.wait(lock);
    //             //cout << "thread " << tid << " run" << endl;
    //         }
    //         count--;
    //     }
    // };


    Semaphore* Semaphore_Create()
    {
        return (Semaphore*)new QSemaphore();
    }

    void Semaphore_Free(Semaphore* sema)
    {
        delete (QSemaphore*)sema;
    }

    void Semaphore_Reset(Semaphore* sema)
    {
        QSemaphore* s = (QSemaphore*)sema;

        s->acquire(s->available());
    }

    void Semaphore_Wait(Semaphore* sema)
    {
        ((QSemaphore*)sema)->acquire();
    }

    void Semaphore_Post(Semaphore* sema, int count)
    {
        ((QSemaphore*)sema)->release(count);
    }

    Mutex* Mutex_Create()
    {
        return (Mutex*)new std::mutex();
    }

    void Mutex_Free(Mutex* mutex)
    {
        delete (std::mutex*) mutex;
    }

    void Mutex_Lock(Mutex* mutex)
    {
        ((std::mutex*) mutex)->lock();
    }

    void Mutex_Unlock(Mutex* mutex)
    {
        ((std::mutex*) mutex)->unlock();
    }

    bool Mutex_TryLock(Mutex* mutex)
    {
        return ((std::mutex*) mutex)->try_lock();
    }


    bool MP_Init()
    {
        int opt_true = 1;
        int res;

#ifdef __WIN32__
        WSADATA wsadata;
        if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
        {
            return false;
        }
#endif // __WIN32__

        MPSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (MPSocket < 0)
        {
            return false;
        }

        res = setsockopt(MPSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt_true, sizeof(int));
        if (res < 0)
        {
            closesocket(MPSocket);
            MPSocket = INVALID_SOCKET;
            return false;
        }

        sockaddr_t saddr;
        saddr.sa_family = AF_INET;
        *(u32*)&saddr.sa_data[2] = htonl(Config::SocketBindAnyAddr ? INADDR_ANY : INADDR_LOOPBACK);
        *(u16*)&saddr.sa_data[0] = htons(7064);
        res = bind(MPSocket, &saddr, sizeof(sockaddr_t));
        if (res < 0)
        {
            closesocket(MPSocket);
            MPSocket = INVALID_SOCKET;
            return false;
        }

        res = setsockopt(MPSocket, SOL_SOCKET, SO_BROADCAST, (const char*)&opt_true, sizeof(int));
        if (res < 0)
        {
            closesocket(MPSocket);
            MPSocket = INVALID_SOCKET;
            return false;
        }

        MPSendAddr.sa_family = AF_INET;
        *(u32*)&MPSendAddr.sa_data[2] = htonl(INADDR_BROADCAST);
        *(u16*)&MPSendAddr.sa_data[0] = htons(7064);

        return true;
    }

    void MP_DeInit()
    {
        if (MPSocket >= 0)
            closesocket(MPSocket);

#ifdef __WIN32__
        WSACleanup();
#endif // __WIN32__
    }

    int MP_SendPacket(u8* data, int len)
    {
        if (MPSocket < 0)
            return 0;

        if (len > 2048 - 8)
        {
            printf("MP_SendPacket: error: packet too long (%d)\n", len);
            return 0;
        }

        *(u32*)&PacketBuffer[0] = htonl(0x4946494E); // NIFI
        PacketBuffer[4] = NIFI_VER;
        PacketBuffer[5] = 0;
        *(u16*)&PacketBuffer[6] = htons(len);
        memcpy(&PacketBuffer[8], data, len);

        int slen = sendto(MPSocket, (const char*)PacketBuffer, len + 8, 0, &MPSendAddr, sizeof(sockaddr_t));
        if (slen < 8) return 0;
        return slen - 8;
    }

    int MP_RecvPacket(u8* data, bool block)
    {
        if (MPSocket < 0)
            return 0;

        fd_set fd;
        struct timeval tv;

        FD_ZERO(&fd);
        FD_SET(MPSocket, &fd);
        tv.tv_sec = 0;
        tv.tv_usec = block ? 5000 : 0;

        if (!select(MPSocket + 1, &fd, 0, 0, &tv))
        {
            return 0;
        }

        sockaddr_t fromAddr;
        socklen_t fromLen = sizeof(sockaddr_t);
        int rlen = recvfrom(MPSocket, (char*)PacketBuffer, 2048, 0, &fromAddr, &fromLen);
        if (rlen < 8 + 24)
        {
            return 0;
        }
        rlen -= 8;

        if (ntohl(*(u32*)&PacketBuffer[0]) != 0x4946494E)
        {
            return 0;
        }

        if (PacketBuffer[4] != NIFI_VER)
        {
            return 0;
        }

        if (ntohs(*(u16*)&PacketBuffer[6]) != rlen)
        {
            return 0;
        }

        memcpy(data, &PacketBuffer[8], rlen);
        return rlen;
    }



    bool LAN_Init()
    {
        //throw "LAN NOT SUPPORTED";
        return false;
        // if (Config::DirectLAN)
        // {
        //     if (!LAN_PCap::Init(true))
        //         return false;
        // }
        // else
        // {
        //     if (!LAN_Socket::Init())
        //         return false;
        // }

        // return true;
    }

    void LAN_DeInit()
    {
        //throw "LAN NOT SUPPORTED";
        // LAN_PCap::DeInit();
        // LAN_Socket::DeInit();
    }

    int LAN_SendPacket(u8* data, int len)
    {
        //throw "LAN NOT SUPPORTED";
        return 0;
        // if (Config::DirectLAN)
        //     return LAN_PCap::SendPacket(data, len);
        // else
        //     return LAN_Socket::SendPacket(data, len);
    }

    int LAN_RecvPacket(u8* data)
    {
        //throw "LAN NOT SUPPORTED";
        return 0;
        // if (Config::DirectLAN)
        //     return LAN_PCap::RecvPacket(data);
        // else
        //     return LAN_Socket::RecvPacket(data);
    }

    void Sleep(u64 usecs)
    {
        //microseconds
        //QThread::usleep(usecs);
        std::this_thread::sleep_for(std::chrono::microseconds(usecs));
    }

}

//Thread* Thread_Create(std::function<void()> func)
//{
//    QThread* t = QThread::create(func);
//    t->start();
//    return (Thread*)t;
//}
//
//void Thread_Free(Thread* thread)
//{
//    QThread* t = (QThread*)thread;
//    t->terminate();
//    delete t;
//}
//
//void Thread_Wait(Thread* thread)
//{
//    ((QThread*)thread)->wait();
//}
//
//Semaphore* Semaphore_Create()
//{
//    return (Semaphore*)new QSemaphore();
//}
//
//void Semaphore_Free(Semaphore* sema)
//{
//    delete (QSemaphore*)sema;
//}
//
//void Semaphore_Reset(Semaphore* sema)
//{
//    QSemaphore* s = (QSemaphore*)sema;
//
//    s->acquire(s->available());
//}
//
//void Semaphore_Wait(Semaphore* sema)
//{
//    ((QSemaphore*)sema)->acquire();
//}
//
//void Semaphore_Post(Semaphore* sema, int count)
//{
//    ((QSemaphore*)sema)->release(count);
//}
//
//Mutex* Mutex_Create()
//{
//    return (Mutex*)new QMutex();
//}
//
//void Mutex_Free(Mutex* mutex)
//{
//    delete (QMutex*)mutex;
//}
//
//void Mutex_Lock(Mutex* mutex)
//{
//    ((QMutex*)mutex)->lock();
//}
//
//void Mutex_Unlock(Mutex* mutex)
//{
//    ((QMutex*)mutex)->unlock();
//}
//
//bool Mutex_TryLock(Mutex* mutex)
//{
//    return ((QMutex*)mutex)->try_lock();
//}

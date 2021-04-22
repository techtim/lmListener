//
// Created by Timofey Tavlintsev on 8/15/17.
// based on openFramework of ofxUDPManager
//

#pragma once

//////////////////////////////////////////////////////////////////////////////////////
// Original author: ???????? we think Christian Naglhofer
// Crossplatform port by: Theodore Watson May 2007 - update Jan 2008
// Changes: Mac (and should be nix) equivilant functions and data types for
// win32 calls, as well as non blocking option SetNonBlocking(bool nonBlocking);
//
//////////////////////////////////////////////////////////////////////////////////////

/*-----------------------------------------------------------------------------------
USAGE
-------------------------------------------------------------------------------------

!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!! LINK WITH ws2_32.lib !!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!

UDP Socket Client (sending):
------------------

1) create()
2) connect()
3) send()
...
x) close()

optional:
SetTimeoutSend()

UDP Multicast (sending):
--------------

1) Create()
2) ConnectMcast()
3) Send()
...
x) Close()

extra optional:
SetTTL() - default is 1 (current subnet)

UDP Socket Server (receiving):
------------------

1) create()
2) bind()
3) receive()
...
x) close()

optional:
SetTimeoutReceive()

UDP Multicast (receiving):
--------------

1) Create()
2) BindMcast()
3) Receive()
...
x) Close()

--------------------------------------------------------------------------------*/
//#include "ofConstants.h"
//#include "ofxUDPSettings.h"
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <wchar.h>

#ifndef WIN32

// unix includes - works for osx should be same for *nix
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

//#ifdef TARGET_LINUX
// linux needs this:
#include <netinet/tcp.h> /* for TCP_MAXSEG value */
#include <netinet/in.h>
#include <netinet/ip.h>
//#endif


#define SO_MAX_MSG_SIZE TCP_MAXSEG
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define FAR

#else
// windows includes
#include <winsock2.h>
#include <ws2tcpip.h> // TCP/IP annex needed for multicasting
#endif

/// Socket constants.
#define SOCKET_TIMEOUT SOCKET_ERROR - 1

#define NO_TIMEOUT 0xFFFF
#define OF_UDP_DEFAULT_TIMEOUT NO_TIMEOUT

namespace LedMapper {

class UdpSettings {
public:
    void sendTo(std::string address, unsigned short port)
    {
        sendAddress = address;
        sendPort = port;
    }

    void receiveOn(std::string address, unsigned short port)
    {
        bindAddress = address;
        bindPort = port;
    }

    void receiveOn(unsigned short port) { bindPort = port; }


    std::string sendAddress;
    unsigned short sendPort = 0;
    std::string bindAddress = "127.0.0.1";
    unsigned short bindPort = 0;

    bool blocking = false;
    bool reuse = false;

    int sendTimeout = OF_UDP_DEFAULT_TIMEOUT;
    int sendBufferSize = 0;
    int receiveTimeout = OF_UDP_DEFAULT_TIMEOUT;
    int receiveBufferSize = 0;
    int ttl = 1;

    bool broadcast = false;
    bool multicast = false;
};

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

class UdpManager {
public:
    // constructor
    UdpManager();

    // destructor
    virtual ~UdpManager()
    {
        if (HasSocket())
            Close();
    }

    bool HasSocket() const { return (m_hSocket) && (m_hSocket != INVALID_SOCKET); }
    bool Close();
    bool Setup(const UdpSettings &settings);
    bool Create();
    bool Connect(const char *pHost, unsigned short usPort);
    bool ConnectMcast(char *pMcast, unsigned short usPort);
    bool Bind(unsigned short usPort);
    bool BindMcast(char *pMcast, unsigned short usPort);
    int Send(const char *pBuff, const int iSize);
    // all data will be sent guaranteed.
    int SendAll(const char *pBuff, const int iSize);
    int PeekReceive(); //	return number of bytes waiting
    int Receive(char *pBuff, const int iSize);
    void SetTimeoutSend(int timeoutInSeconds);
    void SetTimeoutReceive(int timeoutInSeconds);
    int GetTimeoutSend();
    int GetTimeoutReceive();
    bool GetRemoteAddr(
        std::string &address, int &port) const; //	gets the IP and port of last received packet
    bool GetListenAddr(std::string &address, int &port) const; //	get our bound IP and port
    bool SetReceiveBufferSize(int sizeInByte);
    bool SetSendBufferSize(int sizeInByte);
    int GetReceiveBufferSize();
    int GetSendBufferSize();
    bool SetReuseAddress(bool allowReuse);
    bool SetEnableBroadcast(bool enableBroadcast);
    bool SetNonBlocking(bool useNonBlocking);
    int GetMaxMsgSize();
    /// returns -1 on failure
    int GetTTL();
    bool SetTTL(int nTTL);

protected:
#ifdef TARGET_WIN32
    SOCKET m_hSocket;
#else
    int m_hSocket;
#endif

    int WaitReceive(time_t timeoutSeconds, time_t timeoutMillis);
    int WaitSend(time_t timeoutSeconds, time_t timeoutMillis);

    unsigned long m_dwTimeoutReceive;
    unsigned long m_dwTimeoutSend;

    bool nonBlocking;

    struct sockaddr_in saServer;
    struct sockaddr_in saClient;

    static bool m_bWinsockInit;
    bool canGetRemoteAddress;
};

} // namespace LedMapper
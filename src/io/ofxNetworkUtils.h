/*
 NetworkUtils.h based of ofxNetworkUtils.h from openFrameworks, which is distributed under the MIT License.
 Modified on: 13/5/2019 by Tim Tavlintsev

 Copyright (c) 2004 - openFrameworks Community

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 persons to whom the Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "easylogging++.h"
#include <array>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
#include "targetver.h"
#include <tchar.h>

#include <winsock2.h>
#define WSAENOMEM WSA_NOT_ENOUGH_MEMORY
#define OFXNETWORK_ERROR(name) WSAE##name

#define IGMP_MEMBERSHIP_QUERY 0x11 /* membership query         */
#define IGMP_V1_MEMBERSHIP_REPORT 0x12 /* Ver. 1 membership report */
#define IGMP_V2_MEMBERSHIP_REPORT 0x16 /* Ver. 2 membership report */
#define IGMP_V2_LEAVE_GROUP 0x17 /* Leave-group message	    */

#else
#define OFXNETWORK_ERROR(name) E##name
#include <netinet/igmp.h>
#endif

namespace LedMapper {

struct NetworkConfig {
    std::string ip = "127.0.0.1";
    std::string mask = "255.255.255.255";
};

namespace Utils {

/// checks if the first number of IP address >= 224 then returns true
bool CheckMulticastIp(const std::string &ip) noexcept;
/// checks if the last number of IP address == 255 then returns true
bool CheckBroadcastIp(const std::string &ip) noexcept;
bool CheckBroadcastIp(uint32_t ip) noexcept;

/// Depending on network class
std::string GetBroadcastIp(uint32_t ip) noexcept;

uint32_t ConvertStringToIPv4(const std::string &strIPaddress, bool networkByteOrder = true);
std::string ConvertIPv4ToString(uint32_t ipv4, bool networkByteOrder = true);
std::string SacnMulticastIpFromUniverse(uint16_t universe) noexcept;
/// WIN: returns first local IP different from 127.0.0.1 if exists or 127.0.0.1
/// LINUX: returns first local IP for "eth0" device or 127.0.0.1
std::string GetIpAddressLocal();
std::vector<NetworkConfig> GetLocalNetworkConfigs();
std::array<uint8_t, 6> GetMacAddress();

/// IGMP Support
enum IGMP_MESSAGE_TYPE {
    IGMP_MESSAGE_MEMBERSHIP_QUERY = IGMP_MEMBERSHIP_QUERY, /// poll from router
    IGMP_MESSAGE_LEAVE = IGMP_V2_LEAVE_GROUP, /// notification to quit receiving
    IGMP_MESSAGE_MEMBERSHIP_REPORT = IGMP_V2_MEMBERSHIP_REPORT /// send ready to receive notification to router
};

size_t SendIGMP(IGMP_MESSAGE_TYPE type, uint8_t maxRespTime, const std::string &groupAddressStr,
                const std::string &sourceAddressStr);

} // namespace Utils

#define ofxNetworkCheckError() ofxNetworkCheckErrno(__FILE__, __LINE__)

inline int ofxNetworkCheckErrno(const char *file, int line)
{
#ifdef _WIN32
    int err = WSAGetLastError();
#else
    int err = errno;
#endif
    switch (err) {
        case 0:
            break;
        case OFXNETWORK_ERROR(BADF):
            LOG(ERROR) << file << ": " << line << " EBADF: invalid socket";
            break;
        case OFXNETWORK_ERROR(CONNRESET):
            LOG(ERROR) << file << ": " << line << " ECONNRESET: connection closed by peer";
            break;
        case OFXNETWORK_ERROR(CONNABORTED):
            LOG(ERROR) << file << ": " << line << " ECONNABORTED: connection aborted by peer";
            break;
        case OFXNETWORK_ERROR(NOTCONN):
            LOG(ERROR) << file << ": " << line << " ENOTCONN: trying to receive before establishing a connection";
            break;
        case OFXNETWORK_ERROR(NOTSOCK):
            LOG(ERROR) << file << ": " << line << " ENOTSOCK: socket argument is not a socket";
            break;
        case OFXNETWORK_ERROR(OPNOTSUPP):
            LOG(ERROR) << file << ": " << line << " EOPNOTSUPP: specified flags not valid for this socket";
            break;
        case OFXNETWORK_ERROR(TIMEDOUT):
            LOG(ERROR) << file << ": " << line << " ETIMEDOUT: timeout";
            break;
#if !defined(_WIN32)
        case OFXNETWORK_ERROR(IO):
            LOG(ERROR) << file << ": " << line << " EIO: io error";
            break;
#endif
        case OFXNETWORK_ERROR(NOBUFS):
            LOG(ERROR) << file << ": " << line << " ENOBUFS: insufficient buffers to complete the operation";
            break;
        case OFXNETWORK_ERROR(NOMEM):
            LOG(ERROR) << file << ": " << line << " ENOMEM: insufficient memory to complete the request";
            break;
        case OFXNETWORK_ERROR(ADDRNOTAVAIL):
            LOG(ERROR) << file << ": " << line
                       << " EADDRNOTAVAIL: the specified address is not available on the remote machine";
            break;
        case OFXNETWORK_ERROR(AFNOSUPPORT):
            LOG(ERROR) << file << ": " << line
                       << " EAFNOSUPPORT: the namespace of the addr is not supported by this socket";
            break;
        case OFXNETWORK_ERROR(ISCONN):
            LOG(ERROR) << file << ": " << line << " EISCONN: the socket is already connected";
            break;
        case OFXNETWORK_ERROR(CONNREFUSED):
            LOG(ERROR) << file << ": " << line
                       << " ECONNREFUSED: the server has actively refused to establish the connection";
            break;
        case OFXNETWORK_ERROR(NETUNREACH):
            LOG(DEBUG) << file << ": " << line
                       << " ENETUNREACH: the network of the given addr isn't reachable from this host";
            break;
        case OFXNETWORK_ERROR(ADDRINUSE):
            LOG(ERROR) << file << ": " << line << " EADDRINUSE: the socket address of the given addr is already in use";
            break;
        case ENOPROTOOPT:
            LOG(ERROR) << file << ": " << line << " ENOPROTOOPT: the optname doesn't make sense for the given level";
            break;
        case EPROTONOSUPPORT:
            LOG(ERROR) << file << ": " << line
                       << " EPROTONOSUPPORT: the protocol or "
                          "style is not supported by the "
                          "namespace specified";
            break;
        case OFXNETWORK_ERROR(MFILE):
            LOG(ERROR) << file << ": " << line << " EMFILE: the process already has too many file descriptors open";
            break;
#if !defined(_WIN32)
        case OFXNETWORK_ERROR(NFILE):
            LOG(ERROR) << file << ": " << line << " ENFILE: the system already has too many file descriptors open";
            break;
#endif
        case OFXNETWORK_ERROR(ACCES):
            LOG(ERROR) << file << ": " << line
                       << " EACCES: the process does not have the privilege to "
                          "create a socket of the specified style or protocol";
            break;
        case OFXNETWORK_ERROR(MSGSIZE):
            LOG(ERROR) << file << ": " << line
                       << " EMSGSIZE: the socket type requires that the message be sent atomically, but "
                          "the message is too large for this to be possible";
            break;
#if !defined(_WIN32)
        case OFXNETWORK_ERROR(PIPE):
            LOG(ERROR) << file << ": " << line << " EPIPE: this socket was connected but the connection is now broken";
            break;
#endif
        case OFXNETWORK_ERROR(INVAL):
            LOG(ERROR) << file << ": " << line << " EINVAL: invalid argument";
            break;
#if !defined(_WIN32)
#if !defined(EWOULDBLOCK) || EAGAIN != EWOULDBLOCK
        case EAGAIN:
            // Not an error worth reporting, this is normal if the socket is non-blocking
            // LOG(ERROR) << file << ": " << line << " EAGAIN: try again";
            break;
#endif
#endif
        case OFXNETWORK_ERROR(WOULDBLOCK):
        case OFXNETWORK_ERROR(INPROGRESS):
        case OFXNETWORK_ERROR(ALREADY):
            // Not an error worth reporting, this is normal if the socket is non-blocking
            break;
        case OFXNETWORK_ERROR(INTR):
            // LOG(ERROR) << file << ": " << line << " EINTR: receive interrupted by a
            // signal, before any data available";
            break;
        default:
            LOG(ERROR) << file << ": " << line << " unknown error: " << err
                       << " see errno.h for description of the error";
            break;
    }

    return err;
}

} // namespace LedMapper

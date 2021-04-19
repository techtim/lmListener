/*
 * ofxNetworkUtils.h
 *
 *  Created on: 25/09/2010
 *      Author: arturo
 */

#pragma once

#include "easylogging++.h"
//#include <cerrno>

#ifdef TARGET_WIN32
#define WSAENOMEM WSA_NOT_ENOUGH_MEMORY
#define OFXNETWORK_ERROR(name) WSAE##name
#else
#define OFXNETWORK_ERROR(name) E##name
#endif

namespace LedMapper {

#define ofxNetworkCheckError() ofxNetworkCheckErrno(__FILE__, __LINE__)

inline int ofxNetworkCheckErrno(const char *file, int line)
{
#ifdef TARGET_WIN32
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
            LOG(ERROR) << file << ": " << line
                                     << " ECONNRESET: connection closed by peer";
            break;
        case OFXNETWORK_ERROR(CONNABORTED):
            LOG(ERROR) << file << ": " << line
                                     << " ECONNABORTED: connection aborted by peer";
            break;
        case OFXNETWORK_ERROR(NOTCONN):
            LOG(ERROR)
                << file << ": " << line
                << " ENOTCONN: trying to receive before establishing a connection";
            break;
        case OFXNETWORK_ERROR(NOTSOCK):
            LOG(ERROR) << file << ": " << line
                                     << " ENOTSOCK: socket argument is not a socket";
            break;
        case OFXNETWORK_ERROR(OPNOTSUPP):
            LOG(ERROR) << file << ": " << line
                                     << " EOPNOTSUPP: specified flags not valid for this socket";
            break;
        case OFXNETWORK_ERROR(TIMEDOUT):
            LOG(ERROR) << file << ": " << line << " ETIMEDOUT: timeout";
            break;
#if !defined(TARGET_WIN32)
        case OFXNETWORK_ERROR(IO):
            LOG(ERROR) << file << ": " << line << " EIO: io error";
            break;
#endif
        case OFXNETWORK_ERROR(NOBUFS):
            LOG(ERROR) << file << ": " << line
                                     << " ENOBUFS: insufficient buffers to complete the operation";
            break;
        case OFXNETWORK_ERROR(NOMEM):
            LOG(ERROR) << file << ": " << line
                                     << " ENOMEM: insufficient memory to complete the request";
            break;
        case OFXNETWORK_ERROR(ADDRNOTAVAIL):
            LOG(ERROR)
                << file << ": " << line
                << " EADDRNOTAVAIL: the specified address is not available on the remote machine";
            break;
        case OFXNETWORK_ERROR(AFNOSUPPORT):
            LOG(ERROR)
                << file << ": " << line
                << " EAFNOSUPPORT: the namespace of the addr is not supported by this socket";
            break;
        case OFXNETWORK_ERROR(ISCONN):
            LOG(ERROR) << file << ": " << line
                                     << " EISCONN: the socket is already connected";
            break;
        case OFXNETWORK_ERROR(CONNREFUSED):
            LOG(ERROR)
                << file << ": " << line
                << " ECONNREFUSED: the server has actively refused to establish the connection";
            break;
        case OFXNETWORK_ERROR(NETUNREACH):
            LOG(ERROR)
                << file << ": " << line
                << " ENETUNREACH: the network of the given addr isn't reachable from this host";
            break;
        case OFXNETWORK_ERROR(ADDRINUSE):
            LOG(ERROR)
                << file << ": " << line
                << " EADDRINUSE: the socket address of the given addr is already in use";
            break;
        case ENOPROTOOPT:
            LOG(ERROR)
                << file << ": " << line
                << " ENOPROTOOPT: the optname doesn't make sense for the given level";
            break;
        case EPROTONOSUPPORT:
            LOG(ERROR) << file << ": " << line << " EPROTONOSUPPORT: the protocol or "
                                                                "style is not supported by the "
                                                                "namespace specified";
            break;
        case OFXNETWORK_ERROR(MFILE):
            LOG(ERROR)
                << file << ": " << line
                << " EMFILE: the process already has too many file descriptors open";
            break;
#if !defined(TARGET_WIN32)
        case OFXNETWORK_ERROR(NFILE):
            LOG(ERROR)
                << file << ": " << line
                << " ENFILE: the system already has too many file descriptors open";
            break;
#endif
        case OFXNETWORK_ERROR(ACCES):
            LOG(ERROR) << file << ": " << line
                                     << " EACCES: the process does not have the privilege to "
                                        "create a socket of the specified style or protocol";
            break;
        case OFXNETWORK_ERROR(MSGSIZE):
            LOG(ERROR)
                << file << ": " << line
                << " EMSGSIZE: the socket type requires that the message be sent atomically, but "
                   "the message is too large for this to be possible";
            break;
#if !defined(TARGET_WIN32)
        case OFXNETWORK_ERROR(PIPE):
            LOG(ERROR)
                << file << ": " << line
                << " EPIPE: this socket was connected but the connection is now broken";
            break;
#endif
        case OFXNETWORK_ERROR(INVAL):
            LOG(ERROR) << file << ": " << line << " EINVAL: invalid argument";
            break;
#if !defined(TARGET_WIN32)
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
#include "ofxNetworkUtils.h"
#include <asio.hpp>
#include <cassert>

#if defined(_WIN32) || defined(_WIN64)
#include <cstdlib>
#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/if_packet.h>
#include <linux/igmp.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace LedMapper {
namespace Utils {

std::string ConvertIPv4ToString(uint32_t ipv4, bool networkByteOrder)
{
    if (ipv4 == 0) {
        assert(false && "can't use zero uint32_t for ip");
        return "0";
    }

    return networkByteOrder
               ? std::to_string((ipv4 & 0xff000000) >> 24) + "." + std::to_string((ipv4 & 0x00ff0000) >> 16) + "."
                     + std::to_string((ipv4 & 0x0000ff00) >> 8) + "." + std::to_string((ipv4 & 0x000000ff))
               : std::to_string((ipv4 & 0x000000ff)) + "." + std::to_string((ipv4 & 0x0000ff00) >> 8) + "."
                     + std::to_string((ipv4 & 0x00ff0000) >> 16) + "." + std::to_string((ipv4 & 0xff000000) >> 24);
}

uint32_t ConvertStringToIPv4(const std::string &strIPaddress, bool networkByteOrder)
{
    if (strIPaddress.empty()) {
        assert(false && "can't use empty string for ip");
        return 0;
    }

    if (networkByteOrder)
        return inet_addr(strIPaddress.c_str());

    std::stringstream s(strIPaddress);
    int a, b, c, d; // to store the 4 ints
    char ch; // to temporarily store the '.'
    s >> a >> ch >> b >> ch >> c >> ch >> d;
    return ((uint8_t)a << 24) + ((uint8_t)b << 16) + ((uint8_t)c << 8) + (uint8_t)d;
}

std::string GetIpAddressLocal()
{
    std::string ip = "127.0.0.1";
    auto confs = GetLocalNetworkConfigs();
    for (auto &conf : confs)
        if (conf.ip != ip)
            return conf.ip;

    return ip;
}

std::array<uint8_t, 6> GetMacAddress()
{
    std::array<uint8_t, 6> mac{ 0 };

#ifdef _WIN32
    /// TODO
#else

    struct ifaddrs *interfaces = NULL;
    struct ifaddrs *temp_addr = NULL;
    int success = 0;
    // retrieve the current interfaces - returns 0 on success
    success = getifaddrs(&interfaces);
    if (success == 0) {
        // Loop through linked list of interfaces
        temp_addr = interfaces;
        while (temp_addr != NULL) {
            if (temp_addr->ifa_addr == NULL) {
                temp_addr = temp_addr->ifa_next;
                continue;
            }

            if (temp_addr->ifa_addr->sa_family == AF_PACKET) {
                struct sockaddr_ll *s = (struct sockaddr_ll *)temp_addr->ifa_addr;
                int i;
                for (i = 0; i < 6; i++)
                    mac.at(i) = s->sll_addr[i];
            }
            temp_addr = temp_addr->ifa_next;
        }
    }
    // Free memory
    freeifaddrs(interfaces);

#endif

    return mac;
}

std::vector<NetworkConfig> GetLocalNetworkConfigs()
{
    std::vector<NetworkConfig> confs;

#ifdef _WIN32

    PMIB_IPADDRTABLE pIPAddrTable = (MIB_IPADDRTABLE *)std::malloc(sizeof(MIB_IPADDRTABLE));
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;
    IN_ADDR IPAddr;
    LPVOID lpMsgBuf;

    if (pIPAddrTable) {
        // Make an initial call to GetIpAddrTable to get the
        // necessary size into the dwSize variable
        if (GetIpAddrTable(pIPAddrTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) {
            std::free(pIPAddrTable);
            pIPAddrTable = (MIB_IPADDRTABLE *)std::malloc(dwSize);
        }
        if (pIPAddrTable == NULL) {
            LOG(ERROR) << "Memory allocation failed for GetIpAddrTable";
            return confs;
        }
    }
    // Make a second call to GetIpAddrTable to get the
    // actual data we want
    if ((dwRetVal = GetIpAddrTable(pIPAddrTable, &dwSize, 0)) != NO_ERROR) {
        LOG(ERROR) << "GetIpAddrTable failed with error: " << dwRetVal;
        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                          NULL, dwRetVal, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                          (LPTSTR)&lpMsgBuf, 0, NULL)) {
            LOG(ERROR) << lpMsgBuf;
            LocalFree(lpMsgBuf);
        }
        std::free(pIPAddrTable);
        return confs;
    }

    for (int i = 0; i < (int)pIPAddrTable->dwNumEntries; i++) {
        LOG(DEBUG) << "Interface Index:\t " << pIPAddrTable->table[i].dwIndex;
        IPAddr.S_un.S_addr = (u_long)pIPAddrTable->table[i].dwAddr;
        NetworkConfig conf;
        conf.ip = inet_ntoa(IPAddr);
        LOG(DEBUG) << "IP Address:     \t" << conf.ip;
        IPAddr.S_un.S_addr = (u_long)pIPAddrTable->table[i].dwMask;
        conf.mask = inet_ntoa(IPAddr);
        LOG(DEBUG) << "Subnet Mask:    \t" << conf.mask;
        IPAddr.S_un.S_addr = (u_long)pIPAddrTable->table[i].dwBCastAddr;
        LOG(DEBUG) << "BroadCast:      \t" << inet_ntoa(IPAddr) << " - " << pIPAddrTable->table[i].dwBCastAddr;
        LOG(DEBUG) << "Reassembly size:\t" << pIPAddrTable->table[i].dwReasmSize << "\n";
        /// return first ip not equal to localhost
        confs.push_back(conf);
    }

    if (pIPAddrTable) {
        std::free(pIPAddrTable);
        pIPAddrTable = NULL;
    }

#else

    struct ifaddrs *interfaces = NULL;
    struct ifaddrs *temp_addr = NULL;
    int success = 0;
    // retrieve the current interfaces - returns 0 on success
    success = getifaddrs(&interfaces);
    if (success == 0) {
        // Loop through linked list of interfaces
        temp_addr = interfaces;
        while (temp_addr != NULL) {
            if (temp_addr->ifa_addr == NULL) {
                temp_addr = temp_addr->ifa_next;
                continue;
            }

            if (temp_addr->ifa_addr->sa_family == AF_INET) {
                NetworkConfig conf;
                conf.ip = inet_ntoa(((struct sockaddr_in *)temp_addr->ifa_addr)->sin_addr);
                conf.mask = inet_ntoa(((struct sockaddr_in *)(temp_addr->ifa_netmask))->sin_addr);
                LOG(DEBUG) << "Device name:\t" << temp_addr->ifa_name << " ip=" << conf.ip << " mask=" << conf.mask;
                confs.push_back(conf);
            }

            temp_addr = temp_addr->ifa_next;
        }
    }
    // Free memory
    freeifaddrs(interfaces);

#endif

    return confs;
}

std::string GetBroadcastIp(uint32_t ip) noexcept
{
    uint32_t broadIp = 0xffffffff; /// default to 255.255.255.255
    auto ipStr = ConvertIPv4ToString(ip, false);
    auto confs = GetLocalNetworkConfigs();
    for (auto &conf : confs) {
        if (conf.ip != ipStr)
            continue;
        uint32_t uintMask = ConvertStringToIPv4(conf.mask, false);
        broadIp = htonl(ip) | ~uintMask;
        LOG(DEBUG) << "GetBroadcastIp " << ipStr << " mask=" << std::hex << uintMask << " bcast=" << std::hex << broadIp
                   << "|" << ConvertIPv4ToString(broadIp) << std::dec;
    }

    return ConvertIPv4ToString(broadIp);
}

} // namespace Utils
} // namespace LedMapper
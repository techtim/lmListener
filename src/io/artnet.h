#pragma once

const std::string ARTNET_SHORT_NAME = "Ledmap";
const std::string ARTNET_ID = "Art-Net\0";
inline constexpr uint16_t ARTNET_OEM_CODE = 0x9999;
inline constexpr uint16_t ARTNET_PORT = 6454;
inline constexpr uint16_t ARTNET_MAX_DMX_SIZE = 512;
inline constexpr uint16_t SUPPORTED_ARTNET_PROTVER = 14;
inline constexpr uint16_t ARTNET_MAX_UNIVERSE = 32768;

enum ARTNET_OPCODE_TYPE : uint16_t {
    ArtNetOpNone = 0,
    ArtNetOpPoll = 0x2000,
    ArtNetOpPollReply = 0x2100,
    ArtNetOpDmx = 0x5000
};

#pragma pack(1) // Enable byte alignment
typedef struct {
    char Id[8];
    uint16_t OpCode;
    uint8_t ProtVerHi;
    uint8_t ProtVerLo;
    uint8_t TalkToMe;
    uint8_t Priority;
} ART_POLL_TYPE;
#pragma pack() // Disable byte alignment

#pragma pack(1) // Enable byte alignment
typedef struct {
    char Id[8];
    uint16_t OpCode;
    uint8_t IP_Address[4];
    uint16_t Port;
    uint8_t VersInfoH;
    uint8_t VersInfo;
    uint8_t NetSwitch;
    uint8_t SubSwitch;
    uint8_t OemHi;
    uint8_t Oem;
    uint8_t UbeaVersion;
    uint8_t Status1;
    uint8_t EstaManLo;
    uint8_t EstaManHi;
    char ShortName[18];
    char LongName[64];
    char NodeReport[64];
    uint8_t NumPortsHi;
    uint8_t NumPortsLo;
    uint8_t PortTypes[4];
    uint8_t GoodInput[4];
    uint8_t GoodOutput[4];
    uint8_t Swin[4];
    uint8_t Swout[4];
    uint8_t SwVideo;
    uint8_t SwMacro;
    uint8_t SwRemote;
    uint8_t Spare[4];
    uint8_t MAC[6];
    uint8_t BindIP[4];
    uint8_t BindIndex;
    uint8_t Status2;
    uint8_t Filler[26];
} ART_POLL_REPLY_TYPE;
#pragma pack() // Disable byte alignment

#pragma pack(1) // Enable byte alignment
typedef struct {
    char Id[8];
    uint16_t OpCode;
    uint8_t ProtVerHi;
    uint8_t ProtVerLo;
    uint8_t Sequence;
    uint8_t Physical;
    uint8_t SubUni;
    uint8_t Net;
    uint8_t LengthHi;
    uint8_t Length;
    std::array<uint8_t, ARTNET_MAX_DMX_SIZE> Data; // Variable length DMX data
} ART_DMX_TYPE;
#pragma pack() // Disable byte alignment

typedef union {
    ART_POLL_TYPE artPoll;
    ART_POLL_REPLY_TYPE artPollReply;
    ART_DMX_TYPE artDmx;
} ARTNET_MSG_TYPE;

static inline void InitArtDmxFrame(ART_DMX_TYPE &artnetFrame)
{
    memset(&artnetFrame, 0, sizeof(artnetFrame));
    std::copy(ARTNET_ID.begin(), ARTNET_ID.begin() + (sizeof(artnetFrame.Id) - 1), artnetFrame.Id);

    artnetFrame.OpCode = ARTNET_OPCODE_TYPE::ArtNetOpDmx;

    artnetFrame.ProtVerHi = SUPPORTED_ARTNET_PROTVER >> 8; // protocol version high byte
    artnetFrame.ProtVerLo = SUPPORTED_ARTNET_PROTVER & 0xff; // protocol version low byte

    artnetFrame.Sequence = 0; // sequence no - disable sequence(0)
    artnetFrame.Physical = 0; // The physical input port from which DMX512

    // universe datasize
    artnetFrame.LengthHi = ARTNET_MAX_DMX_SIZE >> 8;
    artnetFrame.Length = ARTNET_MAX_DMX_SIZE & 0xff;
}

static inline void InitArtPollReplyFrame(ART_POLL_REPLY_TYPE &pollReply, uint8_t bindIndex, uint16_t universe,
                                         uint32_t hostIp, const std::array<uint8_t, 6> &mac)
{
    memset(&pollReply, 0, sizeof(pollReply));

    std::copy(ARTNET_ID.begin(), ARTNET_ID.begin() + (sizeof(pollReply.Id) - 1), pollReply.Id);

    pollReply.OpCode = ArtNetOpPollReply;
    pollReply.Port = ARTNET_PORT;
    memcpy(pollReply.IP_Address, &hostIp, 4);

    pollReply.VersInfoH = 0;
    pollReply.VersInfo = 0;
    pollReply.NetSwitch = (uint8_t)(universe >> 8);
    pollReply.SubSwitch = (uint8_t)(universe >> 4) & 0x0F;
    pollReply.OemHi = ARTNET_OEM_CODE >> 8;
    pollReply.Oem = ARTNET_OEM_CODE & 0xFF;
    pollReply.UbeaVersion = 0;
    pollReply.Status1 = 0;
    pollReply.EstaManHi = 'L';
    pollReply.EstaManLo = 'M';
    std::copy(ARTNET_SHORT_NAME.begin(), ARTNET_SHORT_NAME.end(), std::begin(pollReply.ShortName));
    std::copy(ARTNET_SHORT_NAME.begin(), ARTNET_SHORT_NAME.end(), std::begin(pollReply.LongName));
    pollReply.NodeReport[0] = 0;

    pollReply.NumPortsHi = 0;
    pollReply.NumPortsLo = 1;

    pollReply.PortTypes[0] = 0x80;

    pollReply.GoodInput[0] = 0;

    pollReply.GoodOutput[0] |= 0x80; // Data is being transmitted
    pollReply.GoodOutput[0] |= 0x02; // Merge mode is latest takes precedence or merging disabled

    pollReply.Swin[0] = 0;

    pollReply.Swout[0] = universe & 0x0F;

    pollReply.SwVideo = 0;
    pollReply.SwMacro = 0;
    pollReply.SwRemote = 0;

    std::copy(mac.begin(), mac.end(), pollReply.MAC);
    memcpy(pollReply.BindIP, &hostIp, 4);

    pollReply.BindIndex = 1 + bindIndex;
    pollReply.Status2 = 0x0D; // Product supports web browser configuration
}
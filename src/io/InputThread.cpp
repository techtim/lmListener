//
// Created by tim on 20.04.2021.
//

#include "InputThread.h"
#include "ofxNetworkUtils.h"
#include <numeric>

namespace LedMapper {

InputThread::InputThread(ReaderWriterQueue<InputConf> &config, ReaderWriterQueue<Frame> &record)
    : m_configQueue(config)
    , m_recordQueue(record)
    , m_hostIp(Utils::GetIpAddressLocal())
    , m_hostIp32(inet_addr(m_hostIp.c_str()))
    , m_mac(Utils::GetMacAddress())
    , m_activeUniverses({ 0 })
    , m_pollUniverses({ 0 })
    , m_artnetPollSize(static_cast<int>(sizeof(ART_POLL_TYPE)))
    , m_artnetDmxSize(static_cast<int>(sizeof(ART_DMX_TYPE)))
{

    /// Setup Artnet unicast|broadcast receiver on local ip & socket
    UdpSettings settings;
    settings.receiveOn(ARTNET_PORT);
    auto broadcastIp = Utils::GetBroadcastIp(m_hostIp32);
    settings.sendTo(broadcastIp, ARTNET_PORT);
    settings.broadcast = true;
    [[maybe_unused]] bool ok = m_artnetUdp.Setup(settings);
    assert(ok);

    InitArtPollReplyFrame(m_pollReply, 0, 0, m_hostIp32, m_mac);
    /// Setup broadcast receiver for ArtNetOpDmx and sender for ArtNetOpPoll local network id ending with 255
    InitArtDmxFrame(m_artnetFrame);
}

void InputThread::startListen(size_t fps) { startThreading(fps); }

void InputThread::stopListen() { stopThreading(); }

static size_t framesAtAtime = 0;
static ARTNET_OPCODE_TYPE opCode = ArtNetOpNone;

void InputThread::threadedUpdate()
{
    if (m_configQueue.try_dequeue(m_config)) {
        parseConfig(m_config);
    }

    framesAtAtime = s_maxChannelsIn;

    bool needArtPollReply = false;
    while (--framesAtAtime > 0) {
        // LOG_EVERY_N(30, DEBUG) << "frames ArtDMX received on universe: " << m_framesIn.at(framesAtAtime).universe
        //                        << " data=" << static_cast<int>(m_framesIn.at(framesAtAtime).data[0]);

        opCode = getArtNetFrame(m_frameIn);

        if (opCode == ArtNetOpPoll)
            needArtPollReply = true;
        else if (opCode == ArtNetOpDmx)
            m_recordQueue.emplace(m_frameIn);
    }

    if (needArtPollReply) {
        sendArtPollReply();
    }
}

void InputThread::parseConfig(const InputConf &config)
{
    m_activeUniverses = GetActiveUniverses(config);
    /// Reply with active or default universes
    m_pollUniverses = vector<uint16_t>{ m_activeUniverses.cbegin(), m_activeUniverses.cend() };
    if (m_pollUniverses.empty()) {
        m_pollUniverses.resize(2);
        std::iota(m_pollUniverses.begin(), m_pollUniverses.end(), 0);
    }

    LOG(DEBUG) << "InputThread config active input universes : " << m_activeUniverses;
}

ARTNET_OPCODE_TYPE InputThread::getArtNetFrame(Frame &newFrame)
{
    int received = m_artnetUdp.Receive((char *)&m_artnetMsg, m_artnetDmxSize);
    if (received < m_artnetPollSize || strncmp(m_artnetMsg.artDmx.Id, ARTNET_ID.c_str(), 7) != 0)
        return ArtNetOpNone;
    if (m_artnetMsg.artDmx.OpCode == ArtNetOpPoll)
        return ArtNetOpPoll;
    if (m_artnetMsg.artDmx.OpCode != ArtNetOpDmx)
        return ArtNetOpNone;

    newFrame.universe = (m_artnetMsg.artDmx.Net << 8) + m_artnetMsg.artDmx.SubUni;
    size_t dmxDataSize = received - (sizeof(ART_DMX_TYPE) - s_dmxUniverseSize);
    if (dmxDataSize > 0 && dmxDataSize <= s_dmxUniverseSize) {
        newFrame.data = std::move(m_artnetMsg.artDmx.Data);
        return ArtNetOpDmx;
    }
    else {
        LOG(ERROR) << "Oversized ArtDMX frame dropped on universe: " << newFrame.universe;
    }

    return ArtNetOpNone;
}

void InputThread::sendArtPollReply()
{
    /// Send a single poll reply for each universe
    for (size_t bindIndex = 0; bindIndex < m_pollUniverses.size(); ++bindIndex) {
        m_pollReply.BindIndex = 1 + bindIndex;
        m_pollReply.NetSwitch = m_pollUniverses[bindIndex] >> 8;
        m_pollReply.SubSwitch = (m_pollUniverses[bindIndex] >> 4) & 0x0F;
        m_pollReply.Swout[0] = m_pollUniverses[bindIndex] & 0x0F;
        // Send with broadcast connection
        m_artnetUdp.Send((char *)&m_pollReply, sizeof(m_pollReply));
    }
}

void InputThread::getLmFrame()
{
    size_t received = 0;
    size_t i = 0;
    size_t total_leds_num = 0, max_leds_in_chan;
    size_t chan_cntr = 0, curChannel;
    size_t headerByteOffset = 0, chanPixelOffset = 0;
    uint16_t ledsInChannel[] = { 0, 0, 0, 0, 0, 0 };
    char *pixels;
    char message[MAX_SENDBUFFER_SIZE];
    bool isWS = true;

    if ((received = m_lmInput.PeekReceive()) > 4) {
        if (m_lmInput.Receive(message, received) <= 0)
            return;

        chan_cntr = 0;
        max_leds_in_chan = 0;
        /// parse header to get number of leds to read per each channel
        /// header end is sequence of two 0xff chars
        while (chan_cntr + 1 < received && (message[chan_cntr * 2] != 0xff && message[chan_cntr * 2 + 1] != 0xff)) {
            ledsInChannel[chan_cntr] = message[chan_cntr * 2 + 1] << 8 | message[chan_cntr * 2];
            // LOG(DEBUG) << "chan #" << chan_cntr << "has leds=" << ledsInChannel[chan_cntr];
            if (ledsInChannel[chan_cntr] > max_leds_in_chan)
                max_leds_in_chan = ledsInChannel[chan_cntr];
            ++chan_cntr;
        }

        headerByteOffset = chan_cntr * 2 + 2;
        /// pixels pointer stores point to pixel data in message starting after offset
        pixels = message + headerByteOffset;

        if (chan_cntr > s_maxChannelsOut)
            chan_cntr = s_maxChannelsOut;

        total_leds_num = (received - headerByteOffset) / 3;

        /// For each channel fill output buffers with pixels data
        chanPixelOffset = 0;
        for (curChannel = 0; curChannel < chan_cntr; ++curChannel) {
            for (i = chanPixelOffset; i < ledsInChannel[curChannel] + chanPixelOffset && i < total_leds_num; ++i) {

                // printf("%i : %i -> %d  %d  %d\n", curChannel, i - chanPixelOffset,
                //        pixels[i * 3 + 0], pixels[i * 3 + 1],
                //        pixels[i * 3 + 2]);

                // if (isWS && (i - chanPixelOffset) < LED_COUNT_WS) {
                //    wsOut.channel[curChannel].leds[i - chanPixelOffset]
                //        = (pixels[i * 3 + 0] << 16) | (pixels[i * 3 + 1] << 8) | pixels[i * 3 + 2];
                //}
                // else {
                //    spiOut.writeLed(curChannel, i - chanPixelOffset, pixels[i * 3 + 0], pixels[i * 3 + 1],
                //                    pixels[i * 3 + 2]);
                //}
            }
            chanPixelOffset += ledsInChannel[curChannel];
        }
    }
}

} // namespace LedMapper
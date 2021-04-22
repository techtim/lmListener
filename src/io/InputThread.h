//
// Created by tim on 20.04.2021.
//

#pragma once

#include "InputConf.h"
#include "UdpManager.h"
#include "io/artnet.h"
#include "readerwriterqueue.h"

#include "ThreadedClass.h"

namespace LedMapper {

using namespace moodycamel;

class InputThread : public ThreadedClass {

public:
    InputThread(ReaderWriterQueue<InputConf> &config, ReaderWriterQueue<Frame> &record);
    InputThread(const InputThread &) = delete;
    InputThread &operator=(const InputThread &) = delete;
    InputThread(InputThread &&) = delete;
    InputThread &operator=(InputThread &&) = delete;
    ~InputThread() {}

    void startListen(size_t fps = s_recordFps);
    void stopListen();

private:
    void threadedUpdate() override;
    void parseConfig(const InputConf &config);
    void sendArtPollReply();

    ARTNET_OPCODE_TYPE getArtNetFrame(Frame &newFrame);
    void getLmFrame();

    InputConf m_config;
    ReaderWriterQueue<InputConf> &m_configQueue;
    ReaderWriterQueue<Frame> &m_recordQueue;

    UdpManager m_artnetUdp, m_lmInput;
    string m_hostIp;
    uint32_t m_hostIp32;
    std::array<uint8_t, 6> m_mac;
    std::vector<uint16_t> m_activeUniverses;

    int m_artnetPollSize;
    int m_artnetDmxSize;
    ARTNET_MSG_TYPE m_artnetMsg;
    ART_DMX_TYPE m_artnetFrame;

    ART_POLL_REPLY_TYPE m_pollReply;
    vector<uint16_t> m_pollUniverses;
    Frame m_frameIn;
};

} // namespace LedMapper
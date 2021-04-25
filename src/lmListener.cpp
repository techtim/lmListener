/*
// App listens on UDP port for frames,
// parse them and route to spi/pwm outputs
// through 2 switches
//
*/
#include "Common.h"
#include "RpiUtils.h"
#include "UdpManager.h"
#include "easylogging++.h"
#include "io/InputThread.h"
#include "spi/SpiOut.h"
#include <numeric>

using std::optional;

INITIALIZE_EASYLOGGINGPP

std::atomic<bool> continue_looping{ true };
int clear_on_exit = 0;

static const std::string s_spiDevice = "/dev/spidev0.0";

enum { TYPE_WS281X, TYPE_SK9822 };

static std::map<std::string, int> s_ledTypeToEnum = { { "WS281X", TYPE_WS281X }, { "SK9822", TYPE_SK9822 } };

using UniversesInOut = array<uint16_t, s_maxUniversesPerOut>;
static const array<UniversesInOut, s_maxChannelsOut> s_universesToOutRouting{ UniversesInOut{ 0, 1, 2, 3, 4 },
                                                                              UniversesInOut{ 5, 6, 7, 8, 9 } };
using namespace LedMapper;

ws2811_led_t *ledsWs1, *ledsWs2;

void stop_program(int sig)
{
    /* Ignore the signal */
    signal(sig, SIG_IGN);
    /* stop the looping */
    continue_looping.store(false);
    /* Put the ctrl-c to default action in case something goes wrong */
    signal(sig, SIG_DFL);
}

using OuputOffsetPair = std::pair<uint16_t, uint16_t>;

optional<OuputOffsetPair> GetOutputAndOffsetForUniverse(const array<UniversesInOut, s_maxChannelsOut> &routing,
                                                        const uint16_t universe)
{
    uint16_t offset = 0;
    auto it
        = std::find_if(routing.cbegin(), routing.cend(), [&universe, &offset](const UniversesInOut &universesInOut) {
              auto itUni = std::find(universesInOut.cbegin(), universesInOut.cend(), universe);
              if (itUni == universesInOut.cend())
                  return false;
              offset = itUni - universesInOut.cbegin();
              return true;
          });

    if (it != routing.cend())
        return std::make_pair(it - routing.cbegin(), offset);
    return {};
}
///
/// Main : init SPI, GPIO and WS interfaces, create lister on localhost:s_lmFrameInPort
/// receives Ledmap UDP frames or ArtNet and route them to LEDs through right outputs on Shield
///
int main()
{
    el::Loggers::reconfigureAllLoggers(el::ConfigurationType::SubsecondPrecision, "3");
    el::Loggers::reconfigureAllLoggers(el::ConfigurationType::Format,
                                       "%datetime{%H:%m:%s.%g} [%level] %fbase:%line: %msg");
    el::Loggers::reconfigureAllLoggers(el::ConfigurationType::ToFile, "false");
    el::Loggers::reconfigureAllLoggers(el::ConfigurationType::ToStandardOutput, "true");
    el::Loggers::reconfigureAllLoggers(el::ConfigurationType::MaxLogFileSize, "4096");

    /// WS (one wire) output setup
    ws2811_t wsOut;
    ws2811_return_t wsReturnStat;
    if (!initWS(ledsWs1, ledsWs2, wsOut)) {
        exit(1);
    }

    SpiOut spiOut;

#if (!defined(AMD64))
    if (!spiOut.init(s_spiDevice))
        exit(1);
#endif

    /// add two channels to spi out,
    spiOut.addChannel(LED_COUNT_SPI);
    spiOut.addChannel(LED_COUNT_SPI);

    if (!initGPIO())
        exit(1);

    /// Init Gpio Multiplexer Switcher, LED Type selection listener thread and atomic isWS flag init
    GpioOutSwitcher gpioSwitcher;
    std::atomic<bool> isWS{ gpioSwitcher.m_isWs };

    ReaderWriterQueue<InputConf> m_inputConfigQueue;
    ReaderWriterQueue<Frame> m_recordQueue;
    InputThread inputThread(m_inputConfigQueue, m_recordQueue);
    inputThread.startListen();

    std::thread typeListener([&isWS]() {
        UdpSettings udpConf;
        udpConf.receiveOn(STRIP_TYPE_PORT);
        auto typeInput = UdpManager();
        if (!typeInput.Setup(udpConf)) {
            LOG(ERROR) << "Failed to bind to port=" << STRIP_TYPE_PORT;
            exit(1);
        }
        std::string currentType{ "" };
        char message[6];
        while (continue_looping.load()) {
            if (typeInput.Receive(message, 6) < 6)
                continue;
            std::string type(message, 6);
            if (currentType != type) {
                currentType = type;
                LOG(DEBUG) << "Got new type " << type;
                isWS.store(s_ledTypeToEnum[type] == TYPE_WS281X, std::memory_order_release);
            }
            std::this_thread::sleep_for(s_secToMs);
        };
    });

    LOG(INFO) << "Inited LedmapListener";

    /// break while loops on termination
    signal(SIGINT, &stop_program);

#ifdef TEST_ANIMATION
    size_t animationCntr = 0;
#endif

    Frame recordFrame;
    size_t framesAtTime = s_maxChannelsIn;
    std::array<uint16_t, s_maxChannelsOut> ledsInChannel;
    optional<OuputOffsetPair> outputAndOffset;

    while (continue_looping.load()) {
#ifdef TEST_ANIMATION
        ++animationCntr;
        testAnim(wsOut, animationCntr);
        continue;
#endif

        /// update output route based on atomic bool changed in typeListener thread
        gpioSwitcher.switchWsOut(isWS.load(std::memory_order_acquire));

        framesAtTime = s_maxChannelsIn;
        /// wait for frames
        while (--framesAtTime >= 0 && m_recordQueue.try_dequeue(recordFrame)) {
            outputAndOffset = GetOutputAndOffsetForUniverse(s_universesToOutRouting, recordFrame.universe);
            if (!outputAndOffset.has_value())
                continue;

            uint16_t outChannel = outputAndOffset.value().first;
            uint16_t pixelOffset = outputAndOffset.value().second * s_pixelsInUniverse;

            /// don't use 510, 511, 512 addresses == s_dmxUniverseSize - 1
            for (size_t i = 0; i < s_dmxUniverseSize - 3; i += 3) {
                wsOut.channel[outChannel].leds[pixelOffset++] = (recordFrame.data.at(i + 0) << 16)
                                                                | (recordFrame.data.at(i + 1) << 8)
                                                                | recordFrame.data.at(i + 2);
            }
        }

#if (!defined(AMD64))
        if (isWS) {
            wsReturnStat = ws2811_render(&wsOut);
            if (wsReturnStat != WS2811_SUCCESS) {
                LOG(ERROR) << "ws2811_render failed: " << ws2811_get_return_t_str(wsReturnStat);
                break;
            }
            // LOG(DEBUG) << "leds send:" << ledsInChannel[0];
        }
        else {
            for (size_t curChannel = 0; curChannel < s_maxChannelsOut; ++curChannel) {
                if (ledsInChannel[curChannel] == 0)
                    continue;
//                digitalWrite(PIN_SWITCH_SPI, curChannel == 0 ? HIGH : LOW);
                spiOut.send(curChannel, ledsInChannel[curChannel]);
            }
            std::this_thread::sleep_for(microseconds(LED_COUNT_SPI));
        }
#endif
    }

    LOG(INFO) << "Exit from loop";

    if (typeListener.joinable())
        typeListener.join();

    inputThread.stopListen();
#if (!defined(AMD64))
    ws2811_fini(&wsOut);
#endif

    return 0;
}

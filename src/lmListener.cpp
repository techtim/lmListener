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

INITIALIZE_EASYLOGGINGPP

std::atomic<bool> continue_looping{ true };
int clear_on_exit = 0;

static const std::string s_spiDevice = "/dev/spidev0.0";

enum { TYPE_WS281X, TYPE_SK9822 };

static std::map<std::string, int> s_ledTypeToEnum = { { "WS281X", TYPE_WS281X }, { "SK9822", TYPE_SK9822 } };

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
///
/// Main : init SPI, GPIO and WS interfaces, create lister on localhost:s_lmFrameInPort
/// receive UDP frames and route them to LEDs through right outputs on Shield
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
    /// further can select kind of channel for different ICs
    spiOut.addChannel(LED_COUNT_SPI);
    spiOut.addChannel(LED_COUNT_SPI);

    if (!initGPIO())
        exit(1);

    /// UDP listeners setup
    UdpSettings udpConf;
    udpConf.receiveOn(s_lmFrameInPort);
    udpConf.receiveBufferSize = MAX_SENDBUFFER_SIZE;
    auto frameInput = UdpManager();
    if (!frameInput.Setup(udpConf)) {
        LOG(ERROR) << "Failed to bind to port=" << s_lmFrameInPort;
        exit(1);
    }

    /// Init Gpio Multiplexer Switcher, LED Type selection listener thread and atomic isWS flag init
    GpioOutSwitcher gpioSwitcher;
    std::atomic<bool> isWS{ gpioSwitcher.m_isWs };

    ReaderWriterQueue<InputConf> m_inputConfigQueue;

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
        };
    });

    LOG(INFO) << "Inited ledMapper Listener";

    /// break while loops on termination
    signal(SIGINT, &stop_program);

    size_t received = 0;
    size_t i = 0;
    size_t total_leds_num = 0, max_leds_in_chan;
    size_t chan_cntr = 0, curChannel;
    size_t headerByteOffset = 0, chanPixelOffset = 0;
    uint16_t ledsInChannel[] = { 0, 0, 0, 0, 0, 0 };
    char *pixels;
    char message[MAX_SENDBUFFER_SIZE];

#ifdef TEST_ANIMATION
    size_t animationCntr = 0;
#endif

    while (continue_looping.load()) {
#ifdef TEST_ANIMATION
        ++animationCntr;
        testAnim(wsOut, animationCntr);
        continue;
#endif

        /// update output route based on atomic bool changed in typeListener thread
        gpioSwitcher.switchWsOut(isWS.load(std::memory_order_acquire));

        /// wait for frames with min size 4 bytes which are header
        if ((received = frameInput.PeekReceive()) > 4) {
            if (frameInput.Receive(message, received) <= 0)
                continue;

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

                    if (isWS && (i - chanPixelOffset) < LED_COUNT_WS) {
                        wsOut.channel[curChannel].leds[i - chanPixelOffset]
                            = (pixels[i * 3 + 0] << 16) | (pixels[i * 3 + 1] << 8) | pixels[i * 3 + 2];
                    }
                    else {
                        spiOut.writeLed(curChannel, i - chanPixelOffset, pixels[i * 3 + 0], pixels[i * 3 + 1],
                                        pixels[i * 3 + 2]);
                    }
                }
                chanPixelOffset += ledsInChannel[curChannel];
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
                for (curChannel = 0; curChannel < chan_cntr; ++curChannel) {
                    if (ledsInChannel[curChannel] == 0)
                        continue;
                    digitalWrite(PIN_SWITCH_SPI, curChannel == 0 ? HIGH : LOW);
                    spiOut.send(curChannel, ledsInChannel[curChannel]);
                }
                std::this_thread::sleep_for(microseconds(max_leds_in_chan));
            }
#endif
        }
    }

    LOG(INFO) << "Exit from loop";

    if (typeListener.joinable())
        typeListener.join();

#if (!defined(AMD64))
    ws2811_fini(&wsOut);
#endif

    return 0;
}

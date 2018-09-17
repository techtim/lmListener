/*
// App listens on UDP port for frames,
// parse them and route to spi/pwm outputs 
// through 2 switches
//
*/

#include <iostream>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <map>
#include <vector>

#include <wiringPi.h>
#include "rpi_ws281x/ws2811.h"
#include "spi/sk9822led.h"
#include "UdpManager.h"

#include "easylogging++.h"

#define ARRAY_SIZE(stuff) (sizeof(stuff) / sizeof(stuff[0]))

// defaults for cmdline options
#define TARGET_FREQ WS2811_TARGET_FREQ
#define GPIO_PIN 18
#define GPIO_PIN_1 12
#define GPIO_PIN_2 13
#define DMA 10
//#define STRIP_TYPE            WS2811_STRIP_RGB		//
// WS2812/SK6812RGB integrated chip+leds
#define STRIP_TYPE WS2811_STRIP_GBR // WS2812/SK6812RGB integrated chip+leds
//#define STRIP_TYPE            SK6812_STRIP_RGBW		// SK6812RGBW
//(NOT SK6812RGB)

#define MAX_CHANNELS 2
#define LED_COUNT_WS 1000
#define LED_COUNT_SPI 3000
#define MAX_MESSAGE_SIZE LED_COUNT_SPI * MAX_CHANNELS * 3 // 2 SPI channels RGB

#define SERVER_PORT 3001

int continue_looping = 1;
int clear_on_exit = 0;

static const int PIN_SWITCH_1 = 5;
static const int PIN_SWITCH_2 = 6;
static const int PIN_SWITCH_SPI = 24;

static std::map<int, bool> s_gpioSwitches = {
    {PIN_SWITCH_1, false}, // false(LOW) - send ws to chan 1
    {PIN_SWITCH_2, true}, // false(LOW) - send ws to chan 2
    {PIN_SWITCH_SPI, true}}; // send spi to chan 1 (false) or chan 2 (true)

bool initGPIO() {
    if (wiringPiSetupGpio() != 0) {
        LOG(ERROR) << "Failed to init GPIO";
        return false;
    }
    for (auto &gpio : s_gpioSwitches) {
        pinMode(gpio.first, OUTPUT);
        LOG(INFO) << "Pin #" << std::to_string(gpio.first) << " -> " << (gpio.second ? "HIGH" : "LOW");
        digitalWrite(gpio.first, (gpio.second ? HIGH : LOW));
    }

    LOG(INFO) << "GPIO Inited";
    return true;
}

bool initWS(ws2811_t &ledstring)
{
    ledstring.freq = TARGET_FREQ;
    ledstring.dmanum = DMA;
    /// channel params sequence must fit its arrangement in ws2811_channel_t
    ledstring.channel[0] = {
            GPIO_PIN_1, // gpionum
            0, // invert
            LED_COUNT_WS, // count
            STRIP_TYPE, // strip_type
            nullptr,
            255, // brightness
    };
    ledstring.channel[1] = {
            GPIO_PIN_2, // gpionum
            0, // invert
            LED_COUNT_WS, // count
            STRIP_TYPE, // strip_type
            nullptr,
            255, // brightness
    };

    ws2811_return_t ret;
    if ((ret = ws2811_init(&ledstring)) != WS2811_SUCCESS) {
        LOG(ERROR) << "ws2811_init failed: \n" << ws2811_get_return_t_str(ret);
        return false;
    }
    return true;
};

struct SpiOut
{
    SpiOut() 
    : fd(-1)
    , size(0)
    {}
    
    ~SpiOut() { 
        for (auto &buf : buffers) 
            sk9822_free(&buf); 
    }

    bool init(const std::string &device){
        /* Open the device file using Low-Level IO */
        fd = open(device.c_str(), O_WRONLY);
        if (fd < 0) {
            LOG(ERROR) << "Error open spi: " << errno  << "-" << strerror(errno);
            return false;
        }
        /* Initialize the SPI bus for Total Control Lighting */
        int return_value = spi_init(fd);
        if (return_value == -1) {
            LOG(ERROR) << "SPI initialization error: " << errno << "-" << strerror(errno);
            return false;
        }
        return true;
    }

    bool addChannel(size_t maxLedsNumber){
        sk9822_buffer buf;
        /* Initialize pixel buffer */
        if (sk9822_init(&buf, maxLedsNumber) < 0) {
            LOG(ERROR) << "SPI Pixel buffer initialization error: Not enough memory.";
            return false;
        }
        LOG(INFO) << "Added SPI channel with " << maxLedsNumber << " leds";
        buffers.emplace_back(std::move(buf));
        return true;
    }

    void writeLed(size_t chan, size_t index, uint8_t red, uint8_t green, uint8_t blue) {
        if (chan > buffers.size() || index > buffers[chan].leds) {
            LOG(ERROR) << "SPI writeLed out of range chan=" << chan << " index=" << index;
            return;
        }
        auto &buf = buffers[chan];
        buf.pixels[index].f = 255;
        buf.pixels[index].r = red;
        buf.pixels[index].g = green;
        buf.pixels[index].b = blue;
    }
    void send(size_t chan, size_t ledsNumber){
        if (fd < 0 || chan >= buffers.size()) {
            LOG(ERROR) << "SPI not initialized or wrong channel:" << chan;
        }
        digitalWrite(PIN_SWITCH_SPI, chan == 0 ? LOW : HIGH);
        send_buffer(fd, &buffers[chan], ledsNumber);
    }
    int fd;
    size_t size;
    std::vector<bool> isDirtyBuffers;
    std::vector<sk9822_buffer> buffers;
};

static const std::string s_spiDevice = "/dev/spidev0.0";

void stop_program(int sig)
{
    /* Ignore the signal */
    signal(sig, SIG_IGN);
    /* stop the looping */
    continue_looping = 0;
    /* Put the ctrl-c to default action in case something goes wrong */
    signal(sig, SIG_DFL);
}
/// 
/// Main : init SPI, GPIO and WS interfaces, create lister on localhost:SERVER_PORT
/// receive UDP frames and route them to LEDs through right outputs on Shield
///
int main()
{
    /// WS (one wire) output setup
    ws2811_t wsOut;
    ws2811_return_t wsReturnStat;
	if (!initWS(wsOut)) {
        return 1;
    }
    
    SpiOut spiOut;
    if (!spiOut.init(s_spiDevice))
        return 1;
    /// add two channels to spi out, 
    /// further can select kind of channel for different ICs
    spiOut.addChannel(LED_COUNT_SPI);
    spiOut.addChannel(LED_COUNT_SPI);

    if (!initGPIO())
        return 1;
    
    /// UDP listener setup
    LedMapper::UdpSettings udpConf;
    udpConf.receiveOn(SERVER_PORT);
    auto frameInput = LedMapper::UdpManager();
    if (!frameInput.Setup(udpConf)) {
        LOG(ERROR) << "Failed to bind to port=" << SERVER_PORT;
        return 1;
    }
    frameInput.SetReceiveBufferSize(MAX_MESSAGE_SIZE);

    LOG(INFO) << "Inited ledMapper Listener";
    
    /// break while loop on termination
    signal(SIGINT, stop_program);

    size_t cntr = 0;

    int received = 0;
    int i = 0;
    int total_leds_num = 0;
    int chan_cntr = 0, curChannel;
    int byte_offset = 0, pixel_offset = 0;
    uint16_t ledsInChannel[] = { 0, 0, 0, 0, 0, 0 };
    char *pixels;
    char message[MAX_MESSAGE_SIZE];

    while (continue_looping) {
        while ((received = frameInput.PeekReceive()) > 0) {
            if (received < 4)
                continue;
            if (frameInput.Receive(message, received) <= 0)
                continue;

            chan_cntr = 0;
            while (chan_cntr + 1 < received
                   && (message[chan_cntr * 2] != 0xff && message[chan_cntr * 2 + 1] != 0xff)) {
                /// get number of leds to read per each channel
                ledsInChannel[chan_cntr] = message[chan_cntr * 2 + 1] << 8 | message[chan_cntr * 2];
                // LOG(INFO) << "#" << chan_cntr << " ledsInChannel : " << ledsInChannel[chan_cntr];
                ++chan_cntr;
            }

            byte_offset = chan_cntr * 2 + 2;
            /// pixels pointer stores point to pixel data in message starting after offset
            pixels = message + byte_offset;

            if (chan_cntr > MAX_CHANNELS)
                    chan_cntr = MAX_CHANNELS;

            total_leds_num = (received - byte_offset) / 3;
            // printf("num of chan %i recv leds: %i\n", chan_cntr, total_leds_num);

            pixel_offset = 0;
            for (curChannel = 0; curChannel < chan_cntr; ++curChannel) {
                for (i = pixel_offset;
                     i < ledsInChannel[curChannel] + pixel_offset && i < total_leds_num; i++) {

                    // printf("%i : %i -> %d  %d  %d\n", curChannel, i - pixel_offset,
                    //        pixels[i * 3 + 0], pixels[i * 3 + 1],
                    //        pixels[i * 3 + 2]);

                    wsOut.channel[curChannel].leds[i - pixel_offset]
                        = (pixels[i * 3 + 0] << 16) | (pixels[i * 3 + 1] << 8) | pixels[i * 3 + 2];
                    
                    spiOut.writeLed(curChannel, i - pixel_offset, pixels[i * 3 + 0], 
                                    pixels[i * 3 + 1], pixels[i * 3 + 2]);
                }
                pixel_offset += ledsInChannel[curChannel];
            }

            wsReturnStat = ws2811_render(&wsOut);
            if (wsReturnStat != WS2811_SUCCESS) {
                LOG(ERROR) << "ws2811_render failed: " << ws2811_get_return_t_str(wsReturnStat);
                break;
            }
            usleep(1000000 / 15);

            /// send SPI channel in has leds
            for (curChannel = 0; curChannel < chan_cntr; ++curChannel) {
                if (ledsInChannel[curChannel])
                    spiOut.send(curChannel, ledsInChannel[curChannel]);
            }
            // 15 frames /second
            // usleep(1000000 / 15);
            // usleep(33000); /// around 30 fps mb
        }

        // cntr++;
        // if (cntr%100000 == 0) {
        //     for (auto &gpio : s_gpioSwitches) {
        //         gpio.second = !gpio.second;
        //         LOG(INFO) << "Pin #" << gpio.first << " -> " << (gpio.second ? "HIGH" : "LOW");
        //         digitalWrite(gpio.first, (gpio.second ? HIGH : LOW));
        //     }
        //     usleep(10000);
        // }
        
    }

    LOG(INFO) << "Exit from loop";
    
    ws2811_fini(&wsOut);
    /* Blank the SPI pixels */
    // if (spiFD != -1) {
    //     for (size_t i = 0; i < LED_COUNT_SPI; ++i) {
    //         spiOut.pixels[i] = {0xFF, 0x00, 0x00, 0x00};
    //     }
    //     send_buffer(spiFD, &spiOut, LED_COUNT_SPI);
    // }

    signal(SIGINT, stop_program);

    return 0;
}

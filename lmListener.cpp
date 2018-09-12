/*
// App listens on UDP port for frames,
// parse them and route to spi/pwm outputs 
// through 2 switchesx
//
// compile:
// g++ lmListener.cpp libws2811.a -L -lws2811 -std=c++14 -o  lmListener
*/

#include <iostream>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rpi_ws281x/ws2811.h"
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
#define LED_COUNT_SPI 2000
#define MAX_MESSAGE_SIZE LED_COUNT_SPI * MAX_CHANNELS * 3 // 2 SPI channels RGB

#define SERVER_PORT 3001

int continue_looping = 1;
int clear_on_exit = 0;

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

void ctrl_c_handler(int sig) {
    LOG(INFO) << "Dying :o";
    ws2811_fini(&ledstring); 
 }

void stop_program(int sig)
{
    /* Ignore the signal */
    signal(sig, SIG_IGN);
    /* stop the looping */
    continue_looping = 0;
    /* Put the ctrl-c to default action in case something goes wrong */
    signal(sig, SIG_DFL);
}

int main()
{
    /// WS (one wire) output setup
    ws2811_t wsOut;
    ws2811_return_t wsReturnStat;
	if (!initWS(wsOut)) {
        return 1;
    }
    

    if (!initSpi()) {

    }

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

    int received = 0;
    int i = 0;
    int total_leds_num = 0;
    int chan_cntr = 0, cur_chan;
    int byte_offset = 0, pixel_offset = 0;
    uint16_t chan_leds_num[] = { 0, 0, 0, 0, 0, 0 };
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
                chan_leds_num[chan_cntr] = message[chan_cntr * 2 + 1] << 8 | message[chan_cntr * 2];
                LOG(INFO) << "#" << chan_cntr << " chan_leds_num : " << chan_leds_num[chan_cntr];
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
            for (cur_chan = 0; cur_chan < chan_cntr; ++cur_chan) {
                for (i = pixel_offset;
                     i < chan_leds_num[cur_chan] + pixel_offset && i < total_leds_num; i++) {

                    printf("%i : %i -> %d  %d  %d\n", cur_chan, i - pixel_offset,
                           pixels[i * 3 + 0], pixels[i * 3 + 1],
                           pixels[i * 3 + 2]);

                    ledstring.channel[cur_chan].leds[i - pixel_offset]
                        = (pixels[i * 3 + 0] << 16) | (pixels[i * 3 + 1] << 8) | pixels[i * 3 + 2];
                }
                pixel_offset += chan_leds_num[cur_chan];
            }

            if ((wsReturnStat = ws2811_render(&ledstring)) != WS2811_SUCCESS) {
                LOG(ERROR) << "ws2811_render failed: " << ws2811_get_return_t_str(wsReturnStat);
                break;
            }

            // 15 frames /sec
            // usleep(1000000 / 15);
            usleep(33333); /// around 30 fps mb
        }
    }

    ws2811_fini(&ledstring);

    return 0;
}

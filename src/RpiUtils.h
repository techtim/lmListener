//
// Created by tim on 21.04.2021.
//

#pragma once

#include "easylogging++.h"
#include "ws2811.h"

#if (!defined(AMD64))
#include <wiringPi.h>
#include <wiringPiSPI.h>
#endif

namespace LedMapper {

static const int PIN_SWITCH_1 = 5;
static const int PIN_SWITCH_2 = 6;
static const int PIN_SWITCH_SPI = 24;

static std::map<int, bool> s_gpioSwitches = { { PIN_SWITCH_1, false }, // false(LOW) - send ws to chan 1
                                              { PIN_SWITCH_2, false }, // false(LOW) - send ws to chan 2
                                              { PIN_SWITCH_SPI, true } }; // send spi to chan 1 (true) or chan 2 (false)

struct GpioOutSwitcher {
    GpioOutSwitcher()
        : m_isWs(false)
    {
        switchWsOut(true);
    }

    void switchWsOut(bool isWS)
    {
        if (m_isWs == isWS)
            return;
        LOG(DEBUG) << "switch to WS = " << isWS;
        m_isWs = isWS;

#if (!defined(AMD64))
        digitalWrite(PIN_SWITCH_1, m_isWs ? LOW : HIGH);
        digitalWrite(PIN_SWITCH_2, m_isWs ? LOW : HIGH);
        std::this_thread::sleep_for(milliseconds(500));
#endif
    }
    bool m_isWs;
};

static inline bool initGPIO()
{

#if (!defined(AMD64))
    if (wiringPiSetupGpio() != 0) {
        LOG(ERROR) << "Failed to init wiringPi SPI";
        return false;
    }
    for (auto &gpio : s_gpioSwitches) {
        pinMode(gpio.first, OUTPUT);
        LOG(INFO) << "Pin #" << std::to_string(gpio.first) << " -> " << (gpio.second ? "HIGH" : "LOW");
        digitalWrite(gpio.first, (gpio.second ? HIGH : LOW));
    }
#endif

    LOG(INFO) << "GPIO Inited";
    return true;
}

static inline bool initWS(ws2811_led_t *ledsWs1, ws2811_led_t *ledsWs2, ws2811_t &ledstring)
{
#if (defined(AMD64))
    return true;
#endif
    ledsWs1 = (ws2811_led_t *)malloc(sizeof(ws2811_led_t) * LED_COUNT_WS);
    ledsWs2 = (ws2811_led_t *)malloc(sizeof(ws2811_led_t) * LED_COUNT_WS);

    ledstring.render_wait_time = 0;
    ledstring.freq = WS2811_TARGET_FREQ;
    ledstring.dmanum = DMA;
    /// channel params sequence must fit its arrangement in ws2811_channel_t
    ledstring.channel[0] = {
        GPIO_PIN_1, // gpionum
        0, // invert
        LED_COUNT_WS, // count
        STRIP_TYPE, // strip_type
        ledsWs1,
        255, // brightness
    };

    ledstring.channel[1] = {
        GPIO_PIN_2, // gpionum
        0, // invert
        LED_COUNT_WS, // count
        STRIP_TYPE, // strip_type
        ledsWs2,
        255, // brightness
    };

    ws2811_return_t ret;
    if ((ret = ws2811_init(&ledstring)) != WS2811_SUCCESS) {
        LOG(ERROR) << "ws2811_init failed: \n" << ws2811_get_return_t_str(ret);
        return false;
    }
    return true;
};

static inline void testAnim(ws2811_t &wsOut, size_t &cntr)
{
    char val = static_cast<char>(static_cast<float>(cntr * 2));

    if (cntr > 126)
        val = static_cast<char>(255);
    if (cntr > 130)
        cntr = 0;

    for (size_t i = 0; i < LED_COUNT_WS; ++i)
        wsOut.channel[0].leds[i] = (val << 16) | (val << 8) | val;

#if (!defined(AMD64))
    ws2811_return_t wsReturnStat = ws2811_render(&wsOut);
    if (wsReturnStat != WS2811_SUCCESS) {
        LOG(ERROR) << "ws2811_render failed: " << ws2811_get_return_t_str(wsReturnStat);
    }
#endif
}

static inline double rgb2hue(uint8_t r, uint8_t g, uint8_t b)
{
    double hue = 0.0;

    double min = std::min(r, std::min(g, b));
    double max = std::max(r, std::max(g, b));

    double delta = max - min;
    if (delta < 0.00001 || max < 1.0)
        return hue;

    if (r >= max) // > is bogus, just keeps compilor happy
        hue = (g - b) / delta; // between yellow & magenta
    else if (g >= max)
        hue = 2.0 + (b - r) / delta; // between cyan & yellow
    else
        hue = 4.0 + (r - g) / delta; // between magenta & cyan

    hue *= 60.0; // degrees

    if (hue < 0.0)
        hue += 360.0;

    return hue;
}

} // namespace LedMapper
#pragma once

#include <atomic>
#include <cassert>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

using std::optional;
using std::string;
using std::vector;

using seconds = std::chrono::seconds;
using microseconds = std::chrono::microseconds;
using nanoseconds = std::chrono::nanoseconds;
using milliseconds = std::chrono::milliseconds;

using namespace std::chrono_literals;

// WS281X lib options
#define GPIO_PIN_1 12 // 21//12
#define GPIO_PIN_2 13
#define DMA 10
#define STRIP_TYPE WS2811_STRIP_RGB // WS2812/SK6812RGB integrated chip+leds
//#define STRIP_TYPE SK6812_STRIP_RGBW // SK6812RGBW

inline constexpr size_t s_dmxUniverseSize = 512;
inline constexpr size_t s_recordFps = 120;
inline constexpr size_t s_maxChannelsIn = 12;
inline constexpr size_t s_maxChannelsOut = 2;
inline constexpr size_t s_maxUniversesInOut = s_maxChannelsIn / s_maxChannelsOut;
inline constexpr size_t s_pixelsInUniverse = 170;
inline constexpr size_t LED_COUNT_WS = 1020;
inline constexpr size_t LED_COUNT_SPI = 2000;
inline constexpr size_t MAX_SENDBUFFER_SIZE = 4096 * 3; // 2 SPI channels RGB

inline constexpr int s_lmFrameInPort = 3001;
inline constexpr int STRIP_TYPE_PORT = 3002;

inline constexpr nanoseconds s_secToNs = seconds{ 1 };
inline constexpr milliseconds s_secToMs = seconds{ 1 };
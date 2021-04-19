#pragma once

#include <atomic>
#include <chrono>
#include <iostream>
#include <map>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <unistd.h>
#include <vector>

using microseconds = std::chrono::microseconds;
using milliseconds = std::chrono::milliseconds;
using namespace std::chrono_literals;

// WS281X lib options
#define GPIO_PIN_1 12 // 21//12
#define GPIO_PIN_2 13
#define DMA 10
#define STRIP_TYPE WS2811_STRIP_RGB // WS2812/SK6812RGB integrated chip+leds
//#define STRIP_TYPE SK6812_STRIP_RGBW // SK6812RGBW

constexpr size_t MAX_CHANNELS = 2;
constexpr size_t LED_COUNT_WS = 1000;
constexpr size_t LED_COUNT_SPI = 2000;
constexpr size_t MAX_SENDBUFFER_SIZE = 4096 * 3; // 2 SPI channels RGB

constexpr int FRAME_IN_PORT = 3001;
constexpr int STRIP_TYPE_PORT = 3002;
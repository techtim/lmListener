#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "sk9822led.h"
#include "../easylogging++.h"

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
        send_buffer(fd, &buffers[chan], ledsNumber);
    }

    int fd;
    size_t size;
    std::vector<bool> isDirtyBuffers;
    std::vector<sk9822_buffer> buffers;
};
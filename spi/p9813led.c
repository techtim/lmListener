/*
 * p9813led.c
 *
 * Copyright 2016 Tim Tavlinsev
 * This program is distributed under the Artistic License 2.0, a copy of which
 * is included in the file LICENSE.txt
 */

/// to compile
/// sudo gcc ./p9813led.c ./udp_listen.c p9813led.h -o udpSK98 -lm

#include "p9813led.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <math.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

void write_frame(p9813_color *p, uint8_t red, uint8_t green, uint8_t blue);
void write_raw(p9813_color *p, p9813_color color);
uint8_t make_flag(uint8_t red, uint8_t greem, uint8_t blue);
ssize_t write_all(int filedes, const void *buf, size_t size);

static uint8_t gamma_table_red[256];
static uint8_t gamma_table_green[256];
static uint8_t gamma_table_blue[256];

/// p9813 specific start end frames
static p9813_color startFrame;
static p9813_color endFrame;

int p9813_init(p9813_buffer *buf, int leds)
{
    buf->leds = leds;
    // size = num leds + start & end frame + (leds/2) bits of 0 => (8+8*(leds >> 4)) of 0x00
    buf->size = (leds + 2) * sizeof(p9813_color);
    buf->buffer = (p9813_color *)malloc(buf->size);
    if (buf->buffer == NULL) {
        return -1;
    }
    printf ("buf: leds=%i, size=%i \n", buf->leds, buf->size);
    buf->pixels = buf->buffer + 2;

    startFrame.r = 0x00;
    startFrame.g = 0x00;
    startFrame.b = 0x00;
    startFrame.f = 0x00;
    endFrame.r = 0x00;
    endFrame.g = 0x00;
    endFrame.b = 0x00;
    endFrame.f = 0x00;

    return 0;
}

int spi_init(int filedes)
{
    int ret;
    const uint8_t mode = SPI_MODE_0;
    const uint8_t bits = 8;
    ///  1.953125MHz
    /// 3.90625MHz
    /// const uint32_t speed = 1.953125 * 1000 * 1000;
    const uint32_t speed = 3.90625 * 1000.f * 1000.f;
    /// and even faster

    ret = ioctl(filedes, SPI_IOC_WR_MODE, &mode);
    if (ret == -1) {
        return -1;
    }

    ret = ioctl(filedes, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        return -1;
    }

    ret = ioctl(filedes, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    if (ret == -1) {
        return -1;
    }

    return 0;
}

void write_color(p9813_color *p, uint8_t red, uint8_t green, uint8_t blue)
{
    write_frame(p, red, green, blue);
}

int send_buffer(int filedes, p9813_buffer *buf, const int leds_num)
{
    int ret;
    write_raw(buf->buffer, startFrame);
    write_raw(buf->buffer + 1, startFrame);
    // write endFrame + zero bit for each second led with 32 leds step (==sizeof(p9813_color))

    ret = (int)write_all(filedes, buf->buffer, (leds_num + 2) * sizeof(p9813_color));
    return ret;
}

void p9813_free(p9813_buffer *buf)
{
    free(buf->buffer);
    buf->buffer = NULL;
    buf->pixels = NULL;
}

void write_raw(p9813_color *p, p9813_color color)
{
    p->f = color.f;
    p->r = color.r;
    p->g = color.g;
    p->b = color.b;
    // printf ("raw:  %02x , %02x \n", p->rg & 255, p->gb & 255);
}
void write_frame(p9813_color *p, uint8_t red, uint8_t green, uint8_t blue)
{
    p->f = 0xC0 | ((~blue & 0xC0) >> 2) | ((~green & 0xC0) >> 4) | ((~red & 0xC0) >> 6);;
    p->r = red;
    p->g = green;
    p->b = blue;
    // printf ("frame: %02x , %02x \n",p->rg, p->gb);
}

ssize_t write_all(int filedes, const void *buf, size_t size)
{
    ssize_t buf_len = (ssize_t)size;
    size_t attempt = size;
    ssize_t result;

    while (size > 0) {
        result = write(filedes, buf, attempt);
        if (result < 0) {
            if (errno == EINTR)
                continue;
            else if (errno == EMSGSIZE) {
                attempt = attempt / 2;
                result = 0;
            }
            else {
                return result;
            }
        }
        buf += result;
        size -= result;
        if (attempt > size)
            attempt = size;
    }

    return buf_len;
}

void set_gamma(double gamma_red, double gamma_green, double gamma_blue)
{
    int i;

    for (i = 0; i < 256; i++) {
        gamma_table_red[i] = (uint8_t)(pow(i/255.0,gamma_red)*253.0+0.5);
        gamma_table_green[i] = (uint8_t)(pow(i/255.0,gamma_green)*253.0+0.5);
        gamma_table_blue[i] = (uint8_t)(pow(i/255.0,gamma_blue)*253.0+0.5);
        // printf("%d, %d, %d, \n", gamma_table_red[i], gamma_table_green[i], gamma_table_blue[i]);
    }
}

void write_gamma_color(p9813_color *p, uint8_t red, uint8_t green, uint8_t blue)
{
    uint8_t gamma_corrected_red = gamma_table_red[red];
    uint8_t gamma_corrected_green = gamma_table_green[green];
    uint8_t gamma_corrected_blue = gamma_table_blue[blue];

    // printf("%d, %d, %d, \n", gamma_corrected_red, gamma_corrected_green, gamma_corrected_blue);
    // flag = make_flag(gamma_corrected_red,gamma_corrected_green,gamma_corrected_blue);
    write_frame(p, gamma_corrected_red, gamma_corrected_green, gamma_corrected_blue);
}

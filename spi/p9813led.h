/*
 * p9813led.h
 *
 * Copyright 2019 Tim Tavlintsev
 * This program is distributed under the Artistic License 2.0, a copy of which
 * is included in the file LICENSE.txt
 */
#ifndef _p9813LED_H
#define _p9813LED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/types.h>
#include <stdint.h>
#include <stdlib.h>

///
/// Implementation of the interface for writing to p9813 led device.
/// 00000000 00000000 00000000 00000000 FFFFFFFF BBBBBBBB GGGGGGGG RRRRRRRR ...
/// |---------------------------------| |---------------| |---------------|
/// 32 zeros to start the frame Led1 Led2 ... 32 zeroes in frame + leds number / 2 zero bits

typedef struct _p9813_color {
    // 11111111 BBBBBBBB GGGGGGGG RRRRRRRR
    uint8_t f;
    uint8_t b;
    uint8_t g;
    uint8_t r;
} p9813_color;

/* The p9813_buffer structure is a buffer in which pixel data is stored in
 * in memory prior to being written out to the SPI device for transmission
 * to the pixels. A buffer can be allocated with the p9813_init function and
 * deallocated with the p9813_free function. The buffer contains a pointer to
 * an array of p9813_color structures representing the array of colors you will
 * send to the leds using the send_buffer function. The "pixels" element of
 * this structure points to the array of pixels.
 */
typedef struct _p9813_buffer {
    size_t leds; /* number of LEDS */
    size_t size; /* size of buffer */
    p9813_color *buffer; /* pointer to buffer memory */
    p9813_color *pixels; /* pointer to start of pixels */
} p9813_buffer;

/* The p9813_init function allocates memory for the pixels in an order that
 * allows for efficient transfer by the send_buffer command. The function
 * takes two arguments:
 *
 * p9813_buffer *buf - A pointer to a p9813_buffer structure.
 * int leds - The number of LEDs for which memory is to be allocated.
 *
 * The function returns 0 if successful or a negative number if it cannot
 * allocate sufficient memory.
 */
int p9813_init(p9813_buffer *buf, int leds);

/* The spi_init function initializes the SPI device for use by the p9813
 * LED strands. It takes one argument:
 *
 * int filedes - An open file descriptor to an spidev device.
 *
 * The function returns a negative number if the initialization fails.
 */
int spi_init(int filedes);

/* The write_color function writes the p9813_color structure corresponding to
 * a color to a location in memory. Typically this location is part of the
 * p9813_buffer's allocated memory. The function takes four arguments:
 *
 * p9813_color *p - A pointer to a p9813_color structure to write to.
 * uint8_t red - The red value between 0 (off) and 255 (full on).
 * uint8_t green - The green value between 0 (off) and 255 (full on).
 * uint8_t blue - The blue value between 0 (off) and 255 (full on).
 */
void write_color(p9813_color *p, const uint8_t red, const uint8_t green, const uint8_t blue);

/* The send_buffer function writes the contents of the p9813_buffer structure to
 * the spi device. It takes two arguments:
 *
 * int filedes - The open and initialized file descriptor for the spi device.
 * p9813_buffer *buf - A pointer to a p9813_buffer which will be transferred to spi.
 *
 * This function returns the number of bytes written if successful or a
 * negative number if it fails. This function will always block while writing
 * and ensure all data is transferred out to the SPI bus.
 */
int send_buffer(int filedes, p9813_buffer *buf, const int leds_num);

/* The p9813_free function will free memory allocated by the p9813_init function.
 * It wakes one argument:
 *
 * p9813_buffer *buf - A pointer to a p9813_buffer that has been allocated by
 *      p9813_init.
 */
void p9813_free(p9813_buffer *buf);

/* The set_gamma function creates lookup tables which are used to apply the
 * gamma correction in the write_gamma_color function. Separate gamma
 * correction factors are chosen for each color. This function must be called
 * prior to using the write_gamma_color function. It takes three arguments:
 *
 * double gamma_red - The gamma correction for the red LEDs.
 * double gamma_green - The gamma correction for the green LEDs.
 * double gamma_blue - The gamma correction for the blue LEDs.
 *
 * Values between 2.2 and 3.0 appear to work well.
 */
void set_gamma(double gamma_red, double gamma_green, double gamma_blue);

/* The write_gamma_color function writes a gamma corrected pixel color to a
 * p9813_pixel structure in memory. It behaves the same way as the write_color
 * function except that a gamma correction is applied to each color. It takes
 * four arguments:
 *
 * p9813_color *p - A pointer to the location in which to write the pixel data.
 * uint8_t red - The red value between 0 (off) and 255 (full on).
 * uint8_t green - The green value between 0 (off) and 255 (full on).
 * uint8_t blue - The blue value between 0 (off) and 255 (full on).
 */
void write_gamma_color(p9813_color *p, uint8_t red, uint8_t green, uint8_t blue);

#ifdef __cplusplus
}
#endif

#endif /*!_p9813LED_H*/

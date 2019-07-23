/*
 * listen-udp.c - Illustrate simple TCP connection
 * It opens a blocking socket and
 * listens for messages in a for loop.  It takes the name of the machine
 * that it will be listening on as argument.
 */

// sudo gcc  -lm ./lpd8806led.c ./lm_listener8806.c -o lm_listener8806

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "lpd8806led.h"
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *device = "/dev/spidev0.0";
static const int leds = 3000;
static const int message_size = 3000 * 3 + 20;
static int continue_looping;

#define SERVER_PORT 3001
#define MAX_CHANNELS 1

void stop_program(int sig);
void draw_test(int fd, lpd8806_buffer *buf);
void HSVtoRGB(double h, double s, double v, double *r, double *g, double *b);

main(int argc, char *argv[])
{
    char message[message_size];
    char *pixels;
    int sock;
    struct sockaddr_in name;
    struct hostent *hp, *gethostbyname();
    int bytes;

    printf("lpd8806 UDP listen activating.\n");

    /* Create socket from which to read */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Opening datagram socket");
        exit(1);
    }

    /* Bind our local address so that the client can send to us */
    bzero((char *)&name, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    name.sin_port = htons(SERVER_PORT);

    if (bind(sock, (struct sockaddr *)&name, sizeof(name))) {
        perror("binding datagram socket");
        exit(1);
    }

    printf("Socket has port number #%d\n", ntohs(name.sin_port));

    // -------- lpd8806 ----------
    lpd8806_buffer buf;
    int fd;
    int return_value;
    lpd8806_color *p;
    int i;
    double h, r, g, b;

    /* Open the device file using Low-Level IO */
    fd = open(device, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
        exit(1);
    }

    /* Initialize the SPI bus for Total Control Lighting */
    return_value = spi_init(fd);
    if (return_value == -1) {
        fprintf(stderr, "SPI initialization error %d: %s\n", errno, strerror(errno));
        exit(1);
    }

    /* Initialize pixel buffer */
    if (lpd8806_init(&buf, leds) < 0) {
        fprintf(stderr, "Pixel buffer initialization error: Not enough memory.\n");
        exit(1);
    }

    /* Set the gamma correction factors for each color */
    set_gamma(2.2, 2.2, 2.2);

    /* Blank the pixels */
    for (i = 0; i < leds; i++) {
        write_color(&buf.pixels[i], 0x00, 0x00, 0x00);
    }

    send_buffer(fd, &buf, leds);
    signal(SIGINT, stop_program);

    continue_looping = 1;
    // unsigned int no_frame_cnt = 0;
    int total_leds_num = 0;
    int chan_cntr = 0, cur_chan;
    int byte_offset = 0, pixel_offset = 0;
    uint16_t chan_leds_num[] = { 0, 0, 0, 0, 0, 0 }; /// possible max num of channels - 6

    /// UDP receiver logic with packet header parser
    while (continue_looping) {
        while ((bytes = read(sock, message, message_size)) > 0) {
            if (bytes < 4)
                continue;

            chan_cntr = 0;
            /// parse first bytes of message for number of channels and number of LEDS per channel
            /// 0xffff means end of the header
            while (chan_cntr + 1 < bytes
                   && (message[chan_cntr * 2] != 0xff && message[chan_cntr * 2 + 1] != 0xff)) {
                chan_leds_num[chan_cntr] = message[chan_cntr * 2 + 1] << 8 | message[chan_cntr * 2];
                // printf("%i chan_leds_num : %i\n", chan_cntr, chan_leds_num[chan_cntr]);
                ++chan_cntr;
            }

            byte_offset = chan_cntr * 2 + 2;
            /// pixels is pointer to colors/pixels data in message
            pixels = message + byte_offset;

            if (chan_cntr > MAX_CHANNELS)
                chan_cntr = MAX_CHANNELS;

            total_leds_num = (bytes - byte_offset) / 3;
            // printf("num of chan %i recv leds: %i\n", chan_cntr, total_leds_num);

            pixel_offset = 0;
            for (cur_chan = 0; cur_chan < chan_cntr; ++cur_chan) {
                for (i = pixel_offset; i < chan_leds_num[cur_chan] + pixel_offset && i < total_leds_num; i++) {

                    // printf("%i : %i -> %d  %d  %d\n", cur_chan, i - pixel_offset,
                    //        pixels[i * 3 + 0], pixels[i * 3 + 1],
                    //        pixels[i * 3 + 2]);

                    write_color(&buf.pixels[i - pixel_offset],
                        pixels[i * 3 + 0], pixels[i * 3 + 1], pixels[i * 3 + 2]);
                }
                pixel_offset += chan_leds_num[cur_chan];
                send_buffer(fd, &buf, chan_leds_num[cur_chan]);
            }

            usleep(5000);
        }
    }

    for (i = 0; i < leds; i++) {
        write_color(&buf.pixels[i], 0x00, 0x00, 0x00);
    }
    send_buffer(fd, &buf, leds);

    close(sock);

    return 0;
}

/// TEST STAND BY CONTENT
int test_value = 0;
int isGroving = 1;
int led_n = 0;

void draw_test(int fd, lpd8806_buffer *buf)
{
    for (led_n = 0; led_n < leds; ++led_n) {
        write_color(&(buf->pixels[led_n]), test_value, test_value, test_value);
    }
    send_buffer(fd, buf, leds);
    test_value += isGroving ? 5 : -5;
    // printf("%d\n",test_value);
    if (test_value > 150)
        isGroving = 0;
    else if (test_value <= 0)
        isGroving = 1;

    usleep(10000);
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
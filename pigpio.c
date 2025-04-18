#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <math.h>
#include <pigpio.h>  // Include pigpio library

#define PORT 6454
#define BUFFER_SIZE 1024
#define MAX_UNIVERSES 16
#define MAX_FRAME_UNIVERSES MAX_UNIVERSES
#define DMX_SLOTS 512
#define MAX_DMX_SLOTS 512
#define SPI_CHANNEL 0  // SPI Channel 0
#define SPI_SPEED 3000000  // 3 MHz SPI speed
#define LATCH_PIN 27  // GPIO pin 27 for latch signal

#define DEFAULT_GAMMA 2.8
#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_DEBUG 2

int log_level = LOG_LEVEL_INFO;
int spi_handle;  // Handle for the SPI channel
float gamma_table[256];
float GAMMA = DEFAULT_GAMMA;

typedef struct {
    uint8_t data[MAX_FRAME_UNIVERSES][DMX_SLOTS];
    int received_flags[MAX_FRAME_UNIVERSES];
    int expected_universes;
} FrameBuffer;

FrameBuffer frame_buffer;

// Logging function
void log_message(int level, const char *message) {
    if (level <= log_level) {
        switch (level) {
            case LOG_LEVEL_ERROR:
                fprintf(stderr, "ERROR: %s\n", message);
                break;
            case LOG_LEVEL_INFO:
                printf("INFO: %s\n", message);
                break;
            case LOG_LEVEL_DEBUG:
                printf("DEBUG: %s\n", message);
                break;
            default:
                break;
        }
    }
}

// Initialize the frame buffer
void init_frame_buffer(int num_universes) {
    memset(&frame_buffer, 0, sizeof(FrameBuffer));
    frame_buffer.expected_universes = num_universes;
}

// Initialize the gamma correction table
void init_gamma_table() {
    for (int i = 0; i < 256; i++) {
        gamma_table[i] = (uint8_t)(255 * pow((i / 255.0), (1.0 / GAMMA)));
    }
}

// Initialize SPI
int init_spi() {
    spi_handle = spiOpen(SPI_CHANNEL, SPI_SPEED, 0);
    if (spi_handle < 0) {
        log_message(LOG_LEVEL_ERROR, "SPI initialization failed!");
        return -1;
    }
    log_message(LOG_LEVEL_INFO, "SPI initialized successfully.");
    return 0;
}

// Write data to SPI
void write_to_spi(uint8_t *data, size_t length) {
    if (spiWrite(spi_handle, (char *)data, length) < 0) {
        log_message(LOG_LEVEL_ERROR, "SPI write failed.");
    } else {
        log_message(LOG_LEVEL_DEBUG, "SPI data written successfully.");
    }
}

// Trigger latch (show the DMX data on LED)
void latch_led_output() {
    gpioWrite(LATCH_PIN, 1);  // Set LATCH_PIN high
    gpioDelay(500);           // Small delay to register the latch
    gpioWrite(LATCH_PIN, 0);  // Set LATCH_PIN low
    log_message(LOG_LEVEL_DEBUG, "Latch signal sent to display the DMX data on LEDs.");
}

// Rainbow chase function to simulate SPI test
void rainbow_chase() {
    uint8_t rainbow[MAX_DMX_SLOTS * 3];
    for (int i = 0; i < MAX_DMX_SLOTS; i++) {
        int offset = i % 256;
        rainbow[i * 3] = (offset < 85) ? offset * 3 : (offset < 170) ? 255 - (offset - 85) * 3 : 0;
        rainbow[i * 3 + 1] = (offset < 85) ? 255 - offset * 3 : (offset < 170) ? 0 : (offset - 170) * 3;
        rainbow[i * 3 + 2] = (offset < 85) ? 0 : (offset < 170) ? (offset - 85) * 3 : 255 - (offset - 170) * 3;
    }
    write_to_spi(rainbow, MAX_DMX_SLOTS * 3);
    latch_led_output();
}

// UDP listener thread
void *udp_listener(void *arg) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    uint8_t buffer[BUFFER_SIZE];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    log_message(LOG_LEVEL_INFO, "Listening for Art-Net packets on port 6454...");

    while (1) {
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
        if (n < 0) {
            perror("Receive failed");
            continue;
        }

        log_message(LOG_LEVEL_DEBUG, "Received packet from client.");
        // Process the packet (functionality omitted for brevity)
    }

    close(sockfd);
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t udp_thread;

    // Initialize pigpio
    if (gpioInitialise() < 0) {
        log_message(LOG_LEVEL_ERROR, "pigpio initialization failed!");
        return EXIT_FAILURE;
    }

    // Set LATCH_PIN as output
    gpioSetMode(LATCH_PIN, PI_OUTPUT);

    // Initialize gamma table
    init_gamma_table();

    // Initialize SPI
    if (init_spi() < 0) {
        gpioTerminate();
        return EXIT_FAILURE;
    }

    // Run rainbow chase as startup test
    log_message(LOG_LEVEL_INFO, "Running Rainbow Chase effect...");
    rainbow_chase();
    sleep(2);

    // Create UDP listener thread
    if (pthread_create(&udp_thread, NULL, udp_listener, NULL) != 0) {
        log_message(LOG_LEVEL_ERROR, "Failed to create UDP listener thread.");
        gpioTerminate();
        return EXIT_FAILURE;
    }

    // Wait for the thread to finish
    pthread_join(udp_thread, NULL);

    // Clean up
    spiClose(spi_handle);
    gpioTerminate();

    return EXIT_SUCCESS;
}

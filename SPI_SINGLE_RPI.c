#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <math.h>

#define PORT 6454
#define BUFFER_SIZE 1024
#define MAX_UNIVERSES 16
#define MAX_DMX_SLOTS 512
#define SPI_CHANNEL_0 0  // SPI Channel 0 (GPIO 10, 11, 12, etc.)

// Default values
#define DEFAULT_SPI_SPEED 4200000  // 4.2 MHz SPI speed
#define DEFAULT_LATCH_PIN 27  // Default GPIO pin 27 for latch signal
#define DEFAULT_GAMMA 2.8  // Default gamma correction value

// Logging levels
#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_DEBUG 2

// Default log level
int log_level = LOG_LEVEL_INFO;

// Configurable values
int SPI_SPEED = DEFAULT_SPI_SPEED;  // SPI speed in Hz
int LATCH_PIN = DEFAULT_LATCH_PIN;  // GPIO pin for latch signal
float GAMMA = DEFAULT_GAMMA;  // Gamma correction value
int reverse_data = 0;  // Flag to reverse the data array
int universes[MAX_UNIVERSES];  // Universes to process
int num_universes = 0;  // Number of universes specified

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

// Gamma correction lookup table
float gamma_table[256]; 

// Function to initialize the gamma correction table
void init_gamma_table() {
    for (int i = 0; i < 256; i++) {
        gamma_table[i] = (uint8_t)(255 * pow((i / 255.0), (1.0 / GAMMA)));
    }
}

// Function to initialize SPI using WiringPi
int init_spi() {
    if (wiringPiSetupGpio() == -1) {
        log_message(LOG_LEVEL_ERROR, "WiringPi setup failed!");
        return -1;
    }

    if (wiringPiSPISetup(SPI_CHANNEL_0, SPI_SPEED) == -1) {
        log_message(LOG_LEVEL_ERROR, "SPI setup failed for channel 0!");
        return -1;
    }

    log_message(LOG_LEVEL_INFO, "SPI initialized with speed set by user.");
    return 0;
}

// Function to write DMX data to SPI
void write_to_spi(uint8_t *data, size_t length) {
    if (wiringPiSPIDataRW(SPI_CHANNEL_0, data, length) == -1) {
        perror("Failed to write to SPI");
    } else {
        log_message(LOG_LEVEL_DEBUG, "DMX data written to SPI.");
    }
}

// Function to trigger latch (show the DMX data on LED)
void latch_led_output() {
    digitalWrite(LATCH_PIN, HIGH);  // Set LATCH_PIN high
    delay(10);  // Small delay to register the latch
    digitalWrite(LATCH_PIN, LOW);   // Set LATCH_PIN low to finish the latching
    log_message(LOG_LEVEL_DEBUG, "Latch signal sent to display the DMX data on LEDs");
}

// Function to reverse the data array
void reverse_data_array(uint8_t *data, size_t length) {
    for (size_t i = 0; i < length / 2; i++) {
        uint8_t temp = data[i];
        data[i] = data[length - 1 - i];
        data[length - 1 - i] = temp;
    }
}

// Function to print usage instructions
void print_usage() {
    printf("Usage: ./program_name [options]\n");
    printf("Options:\n");
    printf("  --gamma <value>       Set gamma correction value (default: 2.8)\n");
    printf("  --universes <list>    Comma-separated list of universes to process (e.g., 0,1,2)\n");
    printf("  --reverse             Reverse the DMX data array before sending to SPI\n");
    printf("  --log-level <level>   Set the log level: 0=ERROR, 1=INFO, 2=DEBUG\n");
    printf("  --spi-speed <value>   Set SPI speed in Hz (default: 4200000)\n");
    printf("  --latch-pin <pin>     Set GPIO pin for latch signal (default: 27)\n");
    printf("  --help                Show this help message\n");
}

// Main function
int main(int argc, char *argv[]) {
    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        } else if (strcmp(argv[i], "--gamma") == 0 && i + 1 < argc) {
            GAMMA = atof(argv[++i]);
        } else if (strcmp(argv[i], "--universes") == 0 && i + 1 < argc) {
            char *universes_str = argv[++i];
            char *token = strtok(universes_str, ",");
            while (token != NULL) {
                int universe = atoi(token);
                if (universe >= 0 && universe < MAX_UNIVERSES) {
                    universes[num_universes++] = universe;
                }
                token = strtok(NULL, ",");
            }
        } else if (strcmp(argv[i], "--reverse") == 0) {
            reverse_data = 1;
        } else if (strcmp(argv[i], "--log-level") == 0 && i + 1 < argc) {
            log_level = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--spi-speed") == 0 && i + 1 < argc) {
            SPI_SPEED = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--latch-pin") == 0 && i + 1 < argc) {
            LATCH_PIN = atoi(argv[++i]);
        }
    }

    // Initialize gamma correction table
    init_gamma_table();

    // Initialize SPI
    if (init_spi() < 0) {
        return EXIT_FAILURE;
    }

    // Set LATCH_PIN as an output
    pinMode(LATCH_PIN, OUTPUT);

    log_message(LOG_LEVEL_INFO, "Program started. Listening for Art-Net packets...");

    // Add further functionality here (e.g., creating UDP threads, etc.)

    return 0;
}

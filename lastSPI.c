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
#define MAX_FRAME_UNIVERSES MAX_UNIVERSES
#define DMX_SLOTS 512
#define MAX_DMX_SLOTS 512
#define SPI_CHANNEL_0 0  // SPI Channel 0 (GPIO 10, 11, 12, etc.)
#define SPI_SPEED 3000000  // 3 MHz SPI speed
#define LATCH_PIN 27  // GPIO pin 26 for latch signal
pthread_mutex_t spi_mutex;  // Mutex for SPI and latch operations

// Default gamma correction value
#define DEFAULT_GAMMA 2.8

// Logging levels
#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_DEBUG 2

// Default log level
int log_level = LOG_LEVEL_INFO;

typedef struct {
    uint8_t data[MAX_FRAME_UNIVERSES][DMX_SLOTS];
    int received_flags[MAX_FRAME_UNIVERSES];  // Flags to track received universes
    int expected_universes;  // Number of universes needed for a complete frame
} FrameBuffer;

FrameBuffer frame_buffer;

typedef struct {
    uint16_t universe;
    uint16_t length;
    uint8_t data[MAX_DMX_SLOTS];
} DMXPacket;

DMXPacket packet_buffer[MAX_UNIVERSES];  // Buffer for storing DMX data
float gamma_table[256];  // Gamma correction lookup table
float GAMMA = DEFAULT_GAMMA;  // Default gamma correction value
int universes[MAX_UNIVERSES];  // Universes to process
int num_universes = 0;  // Number of universes specified
int reverse_data = 0;  // Flag to reverse the data array

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

// Reset the frame buffer
void reset_frame_buffer() {
    memset(frame_buffer.received_flags, 0, sizeof(frame_buffer.received_flags));
    log_message(LOG_LEVEL_DEBUG, "Frame buffer reset.");
}

// Check if the frame is complete
int is_frame_complete() {
    for (int i = 0; i < frame_buffer.expected_universes; i++) {
        if (!frame_buffer.received_flags[i]) {
            return 0;  // Missing at least one universe
        }
    }
    return 1;  // All universes received
}

// Store universe data in the frame buffer and print the size
void store_universe_data(uint16_t universe, uint8_t *data, size_t length) {
    if (universe > MAX_FRAME_UNIVERSES || length != DMX_SLOTS) {
        log_message(LOG_LEVEL_ERROR, "Invalid universe or data length.");
        return;
    }

    // Adjust for 1-based universe numbering
    universe -= 1;

    // Store the data in the frame buffer
    memcpy(frame_buffer.data[universe], data, length);
    frame_buffer.received_flags[universe] = 1;
    log_message(LOG_LEVEL_DEBUG, "Universe data stored in frame buffer.");
}



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

    log_message(LOG_LEVEL_INFO, "SPI initialized: Channel=0, Speed=2000000");
    return 0;
}

void write_to_spi(uint8_t *data, size_t length) {
    pthread_mutex_lock(&spi_mutex);  // Lock the mutex

    if (wiringPiSPIDataRW(SPI_CHANNEL_0, data, length) == -1) {
        perror("Failed to write to SPI");
    } else {
        log_message(LOG_LEVEL_DEBUG, "DMX data written to SPI.");
    }

    pthread_mutex_unlock(&spi_mutex);  // Unlock the mutex
}

// Function to trigger latch (show the DMX data on LED)
void latch_led_output() {
    pthread_mutex_lock(&spi_mutex);  // Lock the mutex

    digitalWrite(LATCH_PIN, HIGH);  // Set LATCH_PIN high
    delayMicroseconds(50);         // Small delay to register the latch
    digitalWrite(LATCH_PIN, LOW);   // Set LATCH_PIN low to finish the latching

    log_message(LOG_LEVEL_DEBUG, "Latch signal sent to display the DMX data on LEDs");

    pthread_mutex_unlock(&spi_mutex);  // Unlock the mutex
}


// Function to reverse the data array
void reverse_data_array(uint8_t *data, size_t length) {
    for (size_t i = 0; i < length / 2; i++) {
        uint8_t temp = data[i];
        data[i] = data[length - 1 - i];
        data[length - 1 - i] = temp;
    }
}

// Rainbow chase function to simulate SPI test
void rainbow_chase() {
    uint8_t rainbow[MAX_DMX_SLOTS * 3];  // 3 bytes per LED (RGB)

    // Generate rainbow chase effect (simple RGB wheel effect)
    for (int i = 0; i < MAX_DMX_SLOTS; i++) {
        int offset = i % 256;
        rainbow[i * 3] = (offset < 85) ? offset * 3 : (offset < 170) ? 255 - (offset - 85) * 3 : 0;
        rainbow[i * 3 + 1] = (offset < 85) ? 255 - offset * 3 : (offset < 170) ? 0 : (offset - 170) * 3;
        rainbow[i * 3 + 2] = (offset < 85) ? 0 : (offset < 170) ? (offset - 85) * 3 : 255 - (offset - 170) * 3;
    }

    // Reverse the rainbow effect if the reverse flag is set
    if (reverse_data) {
        reverse_data_array(rainbow, MAX_DMX_SLOTS * 3);
    }

    // Send the rainbow chase effect to SPI
    write_to_spi(rainbow, MAX_DMX_SLOTS * 3);

    // Trigger latch to display the rainbow chase on LEDs
    latch_led_output();
}

// Function to apply gamma correction and scale to 16-bit
uint16_t gamma_correct_and_scale(uint8_t value) {
    float norm = value / 255.0;
    float corrected = pow(norm, 1.75) * 65535;
    return (uint16_t)corrected;
}

// Function to write DMX data to SPI with gamma correction and scaling to 16-bit
void write_to_spi_with_gamma(uint8_t *data, size_t length) {
    uint8_t spi_data[1024];  // Temporary buffer to store 16-bit values

    for (size_t i = 0; i < length; i++) {
        uint16_t corrected_value = gamma_correct_and_scale(data[i]);

        // Split the 16-bit value into two bytes (high byte and low byte)
        spi_data[i * 2] = (corrected_value >> 8) & 0xFF;  // High byte
        spi_data[i * 2 + 1] = corrected_value & 0xFF;     // Low byte
    }

    // Reverse the data array if the reverse flag is set
    if (reverse_data) {
        reverse_data_array(spi_data, length * 2);
    }

    // Write the data to SPI (16-bit values)
    write_to_spi(spi_data, length * 2);  // Write twice the length because we're sending 16-bit values
}

// Write the frame buffer to SPI
void write_frame_to_spi() {
    log_message(LOG_LEVEL_INFO, "Writing complete frame to SPI...");
    for (int i = 0; i < frame_buffer.expected_universes; i++) {
        write_to_spi_with_gamma(frame_buffer.data[i], DMX_SLOTS);
    }
    latch_led_output();
}

void handle_artnet_packet(uint8_t *buffer, size_t length) {
    uint16_t op_code = buffer[8] | (buffer[9] << 8);

    if (op_code == 0x5200) {  // Art-Net Sync
        log_message(LOG_LEVEL_INFO, "Art-Net Sync message received.");
        if (is_frame_complete()) {
            write_frame_to_spi();
        } else {
            log_message(LOG_LEVEL_ERROR, "Incomplete frame, skipping output.");
        }
        reset_frame_buffer();  // Prepare for the next frame
        return;
    }

    if (op_code != 0x5000) {  // Only handle DMX Art-Net packets
        log_message(LOG_LEVEL_INFO, "Non-DMX Art-Net packet received.");
        return;
    }

    uint16_t universe = buffer[14] | (buffer[15] << 8);
    uint16_t data_length = buffer[16] | (buffer[17] << 8);

    if (length < 18 + data_length) {
        log_message(LOG_LEVEL_ERROR, "Malformed Art-Net packet received.");
        return;
    }

    // Ensure the universe is within bounds and is one of the expected universes
    int process = 0;
    for (int i = 0; i < num_universes; i++) {
        if (universes[i] == universe) {
            process = 1;
            break;
        }
    }
    if (!process) {
        return;  // Ignore this universe
    }

    // Apply gamma correction and store the data in the frame buffer (direct transformation)
    for (int i = 0; i < DMX_SLOTS; i++) {
        frame_buffer.data[universe - 1][i] = gamma_table[buffer[18 + i]];  // Direct assignment
    }
    frame_buffer.received_flags[universe - 1] = 1;
}


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

        log_message(LOG_LEVEL_DEBUG, "Received packet from client");
        handle_artnet_packet(buffer, n);
    }

    close(sockfd);
    return NULL;
}

// Print usage instructions
void print_usage() {
    printf("Usage: ./your_program [options]\n");
    printf("Options:\n");
    printf("  --gamma <value>     Set gamma correction value (default: 2.8)\n");
    printf("  --universes <list>  Comma-separated list of universes to process (e.g., 0,1,2)\n");
    printf("  --reverse           Reverse the DMX data array before sending to SPI\n");
    printf("  --log-level <level> Set the log level: 0=ERROR, 1=INFO, 2=DEBUG\n");
    printf("  --help              Show this help message\n");
}

int main(int argc, char *argv[]) {
    pthread_t udp_thread;

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
        }
    }
    pthread_mutex_init(&spi_mutex, NULL);

    if (num_universes == 0) {
        for (int i = 0; i < MAX_UNIVERSES; i++) {
            universes[num_universes++] = i;
        }
    }
        // Initialize the frame buffer with the number of universes to process
    init_frame_buffer(num_universes);

    // Initialize gamma correction table
    init_gamma_table();

    // Initialize SPI
    if (init_spi() < 0) {
        return EXIT_FAILURE;
    }

    // Initialize wiringPi
    if (wiringPiSetupGpio() == -1) {
        log_message(LOG_LEVEL_ERROR, "WiringPi setup failed!");
        return -1;
    }

    // Set LATCH_PIN as an output
    pinMode(LATCH_PIN, OUTPUT);

    // Run rainbow chase effect as a startup test
    log_message(LOG_LEVEL_INFO, "Running Rainbow Chase effect...");
    rainbow_chase();
    delay(2000);  // Wait for 2 seconds to show the rainbow chase effect

    // Create a UDP listener thread
    if (pthread_create(&udp_thread, NULL, udp_listener, NULL) != 0) {
        log_message(LOG_LEVEL_ERROR, "Failed to create UDP listener thread");
        return EXIT_FAILURE;
    }

    // Join the thread (blocking main)
    pthread_join(udp_thread, NULL);

        //cleanup
        pthread_mutex_destroy(&spi_mutex);

    return 0;
}

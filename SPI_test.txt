#include <stdio.h>
#include <stdint.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <math.h>
#include <unistd.h>

#define SPI_CHANNEL_0 0
#define SPI_CHANNEL_1 1
#define DMX_DATA_LENGTH 512
#define LATCH_PIN_1 26
#define LATCH_PIN_2 27

// Function to latch the frame
void latch_frame();

// Function to send data over SPI
void sendData(int spi_channel, uint8_t value) {
    uint16_t corrected = (uint16_t)(pow((value / 255.0), 1.75) * 65535);
    uint8_t spi_data[2] = {(corrected >> 8) & 0xFF, corrected & 0xFF};
    wiringPiSPIDataRW(spi_channel, spi_data, 2);
}

// Function to send a color sequence to SPI
void sendColorSequence(int spi_channel) {
    printf("Sending color sequence (Red, Green, Blue) to SPI channel %d...\n", spi_channel);
    uint8_t colors[3][3] = {
        {255, 0, 0},  // Red
        {0, 255, 0},  // Green
        {0, 0, 255}   // Blue
    };

    // Loop through Red, Green, Blue colors
    for (int i = 0; i < 3; i++) {
        uint8_t red = colors[i][0];
        uint8_t green = colors[i][1];
        uint8_t blue = colors[i][2];

        for (int j = 0; j < DMX_DATA_LENGTH; j++) {
            // Send RGB data (3 channels per pixel)
            sendData(spi_channel, red);
            sendData(spi_channel, green);
            sendData(spi_channel, blue);
        }

        // Latch the frame after sending each color
        latch_frame();
        sleep(1);  // Wait 1 second before sending the next color
    }
}

// Latch frame function
void latch_frame() {
    digitalWrite(LATCH_PIN_1, HIGH);
    digitalWrite(LATCH_PIN_2, HIGH);
    usleep(10);  // 10 microseconds
    digitalWrite(LATCH_PIN_1, LOW);
    digitalWrite(LATCH_PIN_2, LOW);
}

// Main function
int main() {
    // Setup WiringPi and SPI
    if (wiringPiSetupGpio() == -1) {
        printf("WiringPi setup failed!\n");
        return -1;
    }
    if (wiringPiSPISetup(SPI_CHANNEL_0, 1000000) == -1) {
        printf("SPI setup failed for channel 0!\n");
        return -1;
    }
    if (wiringPiSPISetup(SPI_CHANNEL_1, 1000000) == -1) {
        printf("SPI setup failed for channel 1!\n");
        return -1;
    }

    // Set the latch pins as output
    pinMode(LATCH_PIN_1, OUTPUT);
    pinMode(LATCH_PIN_2, OUTPUT);

    // Send the color sequence (Red, Green, Blue) to both SPI channels
    sendColorSequence(SPI_CHANNEL_0);
    sendColorSequence(SPI_CHANNEL_1);

    // Wait for a while to let the test run
    sleep(2);

    printf("Color sequence test completed. Exiting program.\n");
    return 0;
}

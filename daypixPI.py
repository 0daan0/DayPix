import spidev
import threading
import time
import wiringpi as GPIO
import sys
from python_artnet import python_artnet as Artnet
import logging
import socket

# Logging
logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger()

# SPI and GPIO Setup
GPIO.wiringPiSetupGpio()
spi0 = spidev.SpiDev()
spi1 = spidev.SpiDev()
spi0.open(0, 0)
spi1.open(1, 0)
spi0.max_speed_hz = 3200000
spi1.max_speed_hz = 3200000
spi0.mode = spi1.mode = 0b00

latchPin1, latchPin2 = 26, 27
GPIO.pinMode(latchPin1, GPIO.OUTPUT)
GPIO.pinMode(latchPin2, GPIO.OUTPUT)

# Flags and Locks
buffer_lock = threading.Lock()
b1rec = b2rec = False

# Fetch the local IP address dynamically with retry mechanism
ipAddress = "127.0.0.1"  # Default to localhost if fetching fails
retry_count = 0
max_retries = 240  # Retry for 240 seconds (2 minutes)
retry_interval = 1  # Retry interval in seconds

while retry_count < max_retries:
    try:
        hostname = socket.gethostname()
        ipAddress = str(socket.gethostbyname(hostname))
        logger.info(f"Local IP Address: {ipAddress}")
        break  # Exit the loop if IP is fetched successfully
    except Exception as e:
        logger.error(f"Failed to get local IP address: {e}. Retrying in {retry_interval} seconds...")
        time.sleep(retry_interval)
        retry_count += 1

if retry_count == max_retries:
    logger.warning("Max retries reached. Using fallback IP address: 127.0.0.1")


# Art-Net Config
shortName = "XB1"
longName = "DayPix-Baracle"
port = 6454
artnetUniverse1, artnetUniverse2 = 2, 3
artNet = Artnet.Artnet("0.0.0.0", port, ipAddress, SHORTNAME=shortName, LONGNAME=longName, DEBUG=False)

# Buffers for double buffering
buffer1_spi0 = [0] * 512  # Universe 1 buffer
buffer2_spi0 = [0] * 512  # Universe 1 secondary buffer
buffer1_spi1 = [0] * 512  # Universe 2 buffer
buffer2_spi1 = [0] * 512  # Universe 2 secondary buffer

# Function to send data over SPI with color correction
def sendData(spi, value):
    try:
        norm = value / 255.0
        corrected = int((norm ** 1.75) * 65535)
        spi.xfer2([(corrected >> 8) & 0xFF, corrected & 0xFF])
    except Exception as e:
        logger.error(f"SPI transfer failed: {e}")

# Function to write pixel data to SPI
def writePixelbuffer(buffer, spi):
    for value in buffer:
        sendData(spi, value)

# Latching function
def latch_frame():
    GPIO.digitalWrite(latchPin1, GPIO.HIGH)
    GPIO.digitalWrite(latchPin2, GPIO.HIGH)
    time.sleep(0.00001)
    GPIO.digitalWrite(latchPin1, GPIO.LOW)
    GPIO.digitalWrite(latchPin2, GPIO.LOW)

# Callback for Universe 1
def universe_callback1(data):
    global b1rec, buffer1_spi0, buffer2_spi0
    with buffer_lock:
        # Swap buffers for double buffering
        if not b1rec:
            buffer1_spi0 = data
            b1rec = True

# Callback for Universe 2
def universe_callback2(data):
    global b2rec, buffer1_spi1, buffer2_spi1
    with buffer_lock:
        # Swap buffers for double buffering
        if not b2rec:
            buffer1_spi1 = data
            b2rec = True

# Sync callback function
def sync_callback():
    global b1rec, b2rec
    with buffer_lock:
        if b1rec and b2rec:
            # Write to the corresponding SPI buffers
            writePixelbuffer(buffer1_spi0, spi0)
            writePixelbuffer(buffer1_spi1, spi1)
            # Latch only after both universes are ready
            latch_frame()
            b1rec = b2rec = False

# Main Loop
try:
    while True:
        artNetBuffer = artNet.readBuffer()
        if artNetBuffer:
            for packet in artNetBuffer:
                if packet.universe == artnetUniverse1:
                    universe_callback1(packet.data)
                elif packet.universe == artnetUniverse2:
                    universe_callback2(packet.data)
                elif packet.opCode == 0x5200:  # Sync packet
                    sync_callback()

        # Reduce the sleep time to improve responsiveness and frame rate
        time.sleep(0.0001)  # Adjust delay for smoother sync and higher frequency updates
except KeyboardInterrupt:
    pass
finally:
    artNet.close()
    spi0.close()
    spi1.close()
    GPIO.digitalWrite(latchPin1, GPIO.LOW)
    GPIO.digitalWrite(latchPin2, GPIO.LOW)
    logger.info("Resources released. Goodbye!")
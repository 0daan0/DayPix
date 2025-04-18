import spidev
import threading
import time
import wiringpi as GPIO
import socket
import queue
import logging
from struct import unpack, pack
import random

# Logging setup
logging.basicConfig(level=logging.ERROR)
logger = logging.getLogger()

# SPI and GPIO Setup
GPIO.wiringPiSetupGpio()
spi0 = spidev.SpiDev()
spi1 = spidev.SpiDev()
spi0.open(0, 0)
spi1.open(1, 0)
spi0.max_speed_hz = 4200000
spi1.max_speed_hz = 4200000
spi0.mode = spi1.mode = 0b00

latchPin1, latchPin2 = 26, 27
GPIO.pinMode(latchPin1, GPIO.OUTPUT)
GPIO.pinMode(latchPin2, GPIO.OUTPUT)

# Flags and Locks
buffer_lock = threading.Lock()
b1rec = b2rec = False

# FPS and Sync Packet Counter
sync_packet_counter = 0
fps_lock = threading.Lock()

# Fetch the local IP address dynamically
try:
    hostname = socket.gethostname()
    ipAddress = str(socket.gethostbyname(hostname))
    logger.info(f"Local IP Address: {ipAddress}")
except Exception as e:
    logger.error(f"Failed to get local IP address: {e}")
    ipAddress = "127.0.0.1"  # Default to localhost if fetching fails

# Art-Net Config
shortName = "XB103-STL"
longName = "DayPix-Baracle103STL"
port = 6454
artnetUniverses_spi0 = [2]  # Universes on SPI0
artnetUniverses_spi1 = [3]  # Universes on SPI1

# Buffers for multiple universes on both SPI ports
buffer_spi0 = {universe: [0] * 512 for universe in artnetUniverses_spi0}
buffer_spi1 = {universe: [0] * 512 for universe in artnetUniverses_spi1}

# Queues for Art-Net data
queue_spi0 = queue.Queue()
queue_spi1 = queue.Queue()

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

# Art-Net Packet Handling Classes and Methods
class ArtnetPacket:
    def __init__(self):
        self.data = None

class Artnet:
    ARTNET_HEADER = b'Art-Net\x00'

    def __init__(self, BINDIP="", PORT=6454, SYSIP="10.10.10.1", MAC=["AA", "BB", "CC", "DD", "EE", "FF"], SWVER="14", SHORTNAME="python_artnet", LONGNAME="python_artnet", OEMCODE=0xabcd, ESTACODE=0x7FF0, PORTTYPE=[0x80, 0x00, 0x00, 0x00], REFRESH=44, DEBUG=False, UNILENGTH=16):
        self.BINDIP = BINDIP
        self.SYSIP = SYSIP
        self.PORT = PORT
        self.MAC = MAC
        self.SWVER = SWVER
        self.SHORTNAME = SHORTNAME[:17]
        self.LONGNAME = LONGNAME[:63]
        self.OEMCODE = OEMCODE
        self.ESTACODE = ESTACODE
        self.PORTTYPE = PORTTYPE
        self.REFRESH = REFRESH
        self.DEBUG = DEBUG
        self.packet = None
        self.packet_buffer = [ArtnetPacket()] * UNILENGTH
        self.listen = True
        self.t = threading.Thread(target=self.__init_socket, daemon=True)
        self.t.start()

    def __init_socket(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind((self.BINDIP, self.PORT))
        self.sock.settimeout(5)

        while self.listen:
            try:
                data, addr = self.sock.recvfrom(1024)
            except TimeoutError:
                continue
            except Exception as e:
                logger.error(f"Error: {e}")
                time.sleep(0.1)
                self.packet = None
            else:
                self.packet = self.artnet_packet_to_array(data, addr)

    def artnet_packet_to_array(self, raw_data, sender_addr):
        if unpack('!8s', raw_data[:8])[0] != Artnet.ARTNET_HEADER:
            return None

        (opCode,) = unpack('<H', raw_data[8:10])

        if opCode == 0x5000:  # DMX packet
            length = unpack('!H', raw_data[16:18])
            if len(raw_data) == 18 + length[0]:
                packet = ArtnetPacket()
                (packet.ver, packet.sequence, packet.physical, packet.universe, packet.length) = unpack('!HBBHH', raw_data[10:18])
                packet.opCode = opCode
                packet.universe = unpack('<H', pack('!H', packet.universe))[0]
                raw_data = unpack('{0}s'.format(int(packet.length)), raw_data[18:18 + int(packet.length)])[0]
                packet.data = list(raw_data)
                if packet.universe in artnetUniverses_spi0:
                    writePixelbuffer(packet.data[0:468], spi0)
                elif packet.universe in artnetUniverses_spi1:
                    writePixelbuffer(packet.data[0:468], spi1)
                return packet
        elif opCode == 0x5200:  # Sync packet
            sync_callback()
        return None

    def readBuffer(self):
        return self.packet_buffer

    def close(self):
        self.listen = False
        self.t.join()

# Sync callback function to count FPS (sync packets)
def sync_callback():
    global sync_packet_counter
    with fps_lock:
        sync_packet_counter += 1
    latch_frame()

# Function to display FPS based on sync packets
def display_fps():
    global sync_packet_counter
    while True:
        time.sleep(1)  # Update every second
        with fps_lock:
            fps = sync_packet_counter
            sync_packet_counter = 0
        print(f"FPS: {fps}")

# Main execution
if __name__ == "__main__":
    try:
        artNet = Artnet(BINDIP='0.0.0.0', PORT=6454, SYSIP=ipAddress, SHORTNAME=shortName, LONGNAME=longName)

        # Start the FPS display thread
        fps_thread = threading.Thread(target=display_fps)
        fps_thread.daemon = True
        fps_thread.start()

        # Keep the main program running
        while True:
            time.sleep(1)

    except KeyboardInterrupt:
        logger.info("Program interrupted. Closing...")
'''Simple library that takes Art-Net (DMX) packets and converts them to data that Python can use.

Made in Python 3.8

Art-Net spec: https://www.artisticlicence.com/WebSiteMaster/User%20Guides/art-net.pdf

Version 1.2.0
'''
import threading

from time import sleep
import random

from socket import (socket, AF_INET, SOCK_DGRAM, SOL_SOCKET, SO_REUSEADDR)
from struct import pack, unpack

# This class is the actual packet itself (called from within the main class)
class ArtnetPacket:
    '''This class is the data structure for the packet.'''
    def __init__(self):
        self.opCode = None          # Should always be 80 (0x5000) (int16) (little endian)
        self.ver = None             # Latest is V1.4 (14) (int16, split in two)
        self.sequence = None        # Sequence number, used to check if the packets have arrived in the correct order (int8)
        self.physical = None        # The physical DMX512 port this data came from. (int8)
        self.universe = None        # Which universe this packet is meant for (int16 (actually 15 bytes)) (little endian)
        self.length = None          # How much DMX data we have (full packet is 18+length) (int16)
        self.data = None            # The actual DMX data itself

class ArtPollPacket:
    '''This class holds the incoming ArtPoll packet'''
    def __init__(self):
        self.opCode = None          # Should always be 32 (0x2000) (int16) (little endian)
        self.ver = None             # Latest is V1.4 (14) (int16, split in two)
        self.flags = None           # see art-net documentation (int8)
        self.diag_priority = None   # see artnet documentation (table 5) (int8)
                                    # And then we just ignore the rest ;)

class ArtPollReplyPacket:
    '''This class holds the ArtPollReply packet'''
    def __init__(self):
        self.id = b'Art-Net\x00'                # b'Art-Net\x00' (int8)
        self.opCode = pack('<H', 0x2100)        # 0x2100 (int16) (little endian)
        self.ip_address = [127,0,0,1]           # array of the IP address (4xint8)
        self.port = pack('<H', 0x1936)          # 6454 (0x1936) (int16) (little endian)
        self.vers_info = b"14"                  # We're operating on V1.4 (14) (int16, split in two)
        self.net_switch = 0b00000000            #
        self.sub_switch = 0b00000000            #
        self.oem_code = 0xabcd                  # Artnet OEM code (int16)
        self.ubea_version = 0                   # Not used, set to 0 (int8)
        self.status_1 = 0b00110000              # see art-net documentation (int8)
        self.esta_man = pack('<H', 0x7FF0)      # ESTA manufacturer code (int16, little endian)
        self.port_name = b'\x00'*18             # Set by the controller (17 characters and null, int8)
        self.long_name = b'\x00'*64             # Set by the controller (63 characters and null, int8)
        self.node_report = b'\x00'*64           # Used for debugging, is formatted as xxxx [yyyy] zzzzz... (64 characters padding with null, int8)
        self.num_ports = 1                      # Num of input/output ports. Maximum is 4, but a node can ignore this and just set 0 (int16)
        self.port_types = [0x80,0x00,0x00,0x00] #
        self.good_input = [0b00000000]*4        # Status of node, see documentation for bits to set (int8)
        self.good_input_a = [0b00000000]*4      # Same as above (int8)
        self.sw_in = [0x00]*4                   # (4xint8)
        self.sw_out = [0x00]*4                  # (4xint8)
        self.acn_priority = 1                   # Priority of sACN packet between 1 and 200, 200 being most important
        self.sw_macro = 0                       # Only used if supporting macro inputs, otherwise set all 0s (int8)
        self.sw_remote = 0                      # Only used if supporting remote trigger inputs, otherwise set all 0s (int8)
        self.null_1 = b'\x00'*3                 # 3 bytes of null go here
        self.style = 0x00                       # see art-net documentation (int8)
        self.mac = [b"\x00"]*6                  # MAC address of the interface (6xint8)
        self.bind_ip = [0,0,0,0]                # IP address of the root device (only if part of a larger product) (4xint8)
        self.bind_index = 0                     #
        self.status_2 = 0b00001101              # see art-net documentation (int8)
        self.good_output_b = 0b11000000         # see art-net documentation (int8)
        self.status_3 = 0b00000000              # see art-net documentation (int8)
        self.default_resp_uid = b'\x00'*6       #
        self.user_data = 0x0000                 # User configurable, set to 0 (int16)
        self.refresh_rate = 44                  # Max refresh rate for device, max DMX512 is 44hz (int16)
        self.null_2 = b'\x00'*11                # 11 bytes of null go here

    def __iter__(self):
        for each in self.__dict__.values():
            if isinstance(each,list):
               for j in each:
                   yield j
            else:
                yield each

class Artnet:
    ARTNET_HEADER = b'Art-Net\x00'      # the header for Art-Net packets

    def __init__(self, BINDIP = "", PORT = 6454, SYSIP = "10.10.10.1", MAC = ["AA","BB","CC","DD","EE","FF"], SWVER = "14", SHORTNAME = "python_artnet", LONGNAME = "python_artnet", OEMCODE = 0xabcd, ESTACODE = 0x7FF0, PORTTYPE = [0x80,0x00,0x00,0x00], REFRESH = 44, DEBUG = False, UNILENGTH = 16):
        '''Inits Art-Net
        Args:
            BINDIP: Which interface to listen on; blank or 0.0.0.0 is all (string)
            PORT: Which port to listen on, default 6454 (int)
            UNILENGTH: How many universes to listen on, default 16 (int)
            SYSIP: Our IP address, used for node discovery (string)
            MAC: Our MAC address, used for node discovery (list, 6 strings)
            SWVER: Art-Net version, default 14 (V4) (string)
            SHORTNAME: Node name (string, 17 characters)
            LONGNAME: Node description (string, 63 characters)
            OEMCODE: Art-Net OEM code, set if you have one (int)
            ESTACODE: ESTA code, set if you have one (int)
            PORTTYPE: What type of pythical ports you have, see Art-Net documentation (list, 4 ints)
            REFRESH: Node refresh rate (44hz is max for DMX) (int)
            DEBUG: Whether to print debug messages (bool)'''

        self.BINDIP = BINDIP            # IP to listen on (either 0.0.0.0 (all interfaces) or a broadcast address)
        self.SYSIP = SYSIP              # IP of the system itself
        self.PORT = PORT                # Port to listen on (default is 6454)
        self.MAC = MAC                  # MAC address of the ArtNet port
        self.SWVER = SWVER              # Software version
        self.SHORTNAME = SHORTNAME[:17] # Short name
        self.LONGNAME = LONGNAME[:63]   # Long name
        self.OEMCODE = OEMCODE          # Art-Net OEM code
        self.ESTACODE = ESTACODE        # ESTA Manufacturer code
        self.PORTTYPE = PORTTYPE        #
        self.REFRESH = REFRESH          # Refresh rate of the node

        self.DEBUG = DEBUG

        self.packet = None
        self.packet_buffer = [ArtnetPacket()]*UNILENGTH

        # Starts the listner in it's own thread
        self.listen = True
        self.t = threading.Thread(target=self.__init_socket, daemon=True)
        self.t.start()

    def __init_socket(self):
        '''Creates a UDP socket with the specified IP and Port'''
        self.sock = socket(AF_INET, SOCK_DGRAM)  # UDP
        self.sock.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
        self.sock.bind((self.BINDIP, self.PORT))
        self.sock.settimeout(5)

        # Will keep listening until it's told to stop
        while self.listen:
            try:
                data, addr = self.sock.recvfrom(1024)
            except TimeoutError:
                continue
            except Exception as e:
                # Unless something goes wrong :V
                sleep(0.1)
                print(e)
                self.packet = None
            else:
                # Otherwise get the raw packet and decode it using the function.
                # print("Recieved from:" + str(addr))
                self.packet = Artnet.artnet_packet_to_array(self, data, addr)

    def close(self):
        '''Tells the socket to stop running and joins the thread back'''
        self.listen = False
        self.t.join()
        return None

    def art_pol_reply(self, senderAddr):
        '''Sends a reply to an ArtPoll packet; this allows other devices on the network to know who we are.'''
        reply = ArtPollReplyPacket()

        reply.ip_address = [int(i) for i in self.SYSIP.split(".")]
        reply.port = pack('<H', self.PORT)
        reply.vers_info = str.encode(self.SWVER)
        reply.oem_code = self.OEMCODE
        reply.esta_man = pack('<H', self.ESTACODE)
        reply.port_types = self.PORTTYPE
        reply.refresh_rate = self.REFRESH
        for i in range(len(self.MAC)):
            reply.mac[i] = bytes.fromhex(self.MAC[i])
        reply.port_name = str.encode(self.SHORTNAME).ljust(18, b'\x00')
        reply.long_name = str.encode(self.LONGNAME).ljust(64, b'\x00')

        if self.DEBUG: print(*reply)
        packed_reply = pack('!8s2s4B2s2sBBHBB2s18s64s64sH4B4B4B4B4BBBB3sB1s1s1s1s1s1s4BBBBB6sHH11s', *reply)

        # Sleep a random period between 0 and 1 seconds before sending the reply
        sleep(random.random())
        self.sock.sendto(packed_reply, senderAddr)
        if self.DEBUG: print("sent!")

        return None

    def artnet_packet_to_array(self, raw_data, sender_addr):
        '''Extracts DMX data from the Art-Net packet and returns it as a nice array.'''
        if unpack('!8s', raw_data[:8])[0] != Artnet.ARTNET_HEADER:
            # The packet doesn't have a valid header, so it's probably not an Art-Net packet (or it's broken... :V)
            if self.DEBUG: print("Received a non Art-Net packet")
            return None

        # Extracts the opcode from the packed (little endian int16)
        (opCode) = unpack('<H', raw_data[8:10])

        # and checks to see if the packet is an DMX Art-Net packet (0x5000)
        if opCode[0] == 0x5000:
            length = unpack('!H', raw_data[16:18])
            # makes sure the packet is the correct length (if it fetches them too quickly it comes through all malformed)
            if len(raw_data) == 18+length[0]:
                # stores the packet...
                packet = ArtnetPacket()
                # ...then unpacks it into it's constituent parts
                (packet.ver, packet.sequence, packet.physical, packet.universe, packet.length) = unpack('!HBBHH', raw_data[10:18])

                # These guys are little endian, so we need to swap the order
                packet.opCode = opCode[0]
                packet.universe = unpack('<H', pack('!H', packet.universe))[0]

                # this takes the DMX data and converts it to an array
                raw_data = unpack(
                    '{0}s'.format(int(packet.length)),
                    raw_data[18:18+int(packet.length)])[0]

                packet.data = list(raw_data)
                # then returns it
                if packet.universe < len(self.packet_buffer):
                    self.packet_buffer[packet.universe] = packet
                    return packet
                else:
                    if self.DEBUG: print("Received universe too high, discarding...")
                    return None
            else:
                return None
        elif opCode[0] == 0x5200:
            # Handle Sync packet (Art-Net Sync)
            #self.handle_sync_packet(sender_addr)
            if self.DEBUG: print("Sync packet received!")
            sync_packet = ArtnetPacket()
            sync_packet.opCode = 0x5200  # Sync packet
            sync_packet.ver = 14         # Version of the packet (default 14 for sync packets)
            sync_packet.sequence = 0     # Sequence number (optional)
            sync_packet.physical = 0     # Physical address (optional)
            sync_packet.universe = 0     # Universe (optional)
            sync_packet.length = 0       # No data payload for this packet type

            # Store the packet in the buffer if needed (optional)
            if sync_packet.universe < len(self.packet_buffer):
               self.packet_buffer[sync_packet.universe] = sync_packet

             # Return the sync packet (can be checked with opCode == 0x5200)
            return sync_packet

        # or checks to see if the packet is an ArtPoll packet (0x2000)
        elif opCode[0] == 0x2000:
            if self.DEBUG: print("poll!")
            # if the packet is at least 14 bytes, then it might be an ArtPoll packet
            if len(raw_data) >= 14:
                # stores the packet...
                poll_packet = ArtPollPacket()
                # ...then unpacks it into it's constituent parts
                (poll_packet.ver, poll_packet.flags, poll_packet.diag_priority) = unpack('!HBB', raw_data[10:14])

                poll_packet.opCode = opCode[0]

                # Then we need to be nice and send an ArtPollReply to let the controller know that we exist
                self.reply = threading.Thread(target=Artnet.art_pol_reply, args=(self, sender_addr))
                self.reply.start()

                # When we've sent the packet, we can return
                return None
            else:
                # No idea what we received in that case :V
                return None

        else:
            # otherwise, nothing happened so return it
            return None

    def readPacket(self):
        '''Returns the last Art-Net packet that we received.'''
        return(self.packet)

    def readBuffer(self):
        '''Returns the Art-Net packet buffer'''
        return(self.packet_buffer)

def version():
    '''Returns the library version'''
    return "1.2.0"

if __name__ == "__main__":
    ### Art-Net Config ###
    artnetBindIp = ""
    systemIp = "192.168.1.165"
    artnetPort = 6454
    artnetUniverse = 0

    artNet = Artnet(artnetBindIp,artnetPort,systemIp,DEBUG=True,UNILENGTH=2)
    while True:
        try:
            artNetPacket = artNet.readBuffer()
            # Makes sure the packet is valid and that there's some data available
            if artNetPacket is not None:
                if artNetPacket[artnetUniverse].data is not None:
                    # Stores the packet data array
                    dmx = artNetPacket[artnetUniverse].data
                    print("1: ",*dmx[:12], " ", artNetPacket[artnetUniverse].sequence)

                if artNetPacket[1].data is not None:
                    # Stores the packet data array
                    dmx = artNetPacket[1].data
                    print("2: ",*dmx[:12], " ", artNetPacket[1].sequence)

            sleep(0.5)
        except KeyboardInterrupt:
            break

    artNet.close()
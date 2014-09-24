# -*- python -*-
# vim:set ts=4 sw=4 et

"""
Protocol Buffers serial transfer

Simple protocol for transfer PB messages defined in miniecu.proto.

Message format:
   <STX><SEQ><LEN[2]><PAYLOAD[LEN]><CRC[2]>
"""

__all__ = (
    'PBStx',
    'ReceiveError',
    'msgs',
)

import serial
import threading
import struct
from xmodem_crc16 import xmodem_crc16

try:
    import miniecu_pb2 as msgs
except ImportError as ex:
    raise ImportError(str(ex) + ": did you run protoc generator?")


class ReceiveError(Exception):
    pass


class PBStx(object):
    """Protocol Buffers serial transfer protocol"""

    STX = 0xae          # STX
    EHEADER = '<BBH'    # Encode header: STX, SEQ, LEN
    DHEADER = '<BH'     # Decode header: SEQ, LEN
    MAX_LEN = 256
    CRCFMT = '<H'       # CRC16 (xmodem)

    def __init__(self, port, baud=57600, sysid=240):
        self.terminate = threading.Event()
        self.ser = serial.Serial(port, baud)
        self.ser.setTimeout(2.0)
        self._tx_seq = 0
        self._rx_seq = 0

    def __del__(self):
        self.terminate.set()

    def send(self, pbobj):
        if not isinstance(pbobj, msgs.Message):
            raise ValueError("Unknown object: " + repr(pbobj))

        payload = pbobj.SerializeToString()
        if len(payload) > PBStx.MAX_LEN:
            raise ValueError("Serialized {} too long: {}".format(repr(pbobj), len(payload)))

        buf = struct.pack(PBStx.EHEADER, PBStx.STX, self._tx_seq, len(payload))
        buf += payload

        tx_crc = xmodem_crc16(buf[1:])
        buf += struct.pack(PBStx.CRCFMT, tx_crc)

        self._tx_seq += 1
        self.ser.write(buf)

    def receive(self):
        seq = 0
        len_ = 0
        payload = bytearray()
        crc = 0
        rx_crc = 0
        hdr_len = struct.calcsize(PBStx.DHEADER)
        crc_len = struct.calcsize(PBStx.CRCFMT)

        while not self.terminate.is_set():
            # 1. wait start marker
            c = self.ser.read(1)
            if len(c) == 0 or ord(c[0]) != PBStx.STX:
                continue

            # 2. read header
            buf = self._read_or_die(hdr_len)
            rx_crc = xmodem_crc16(buf)
            seq, len_ = struct.unpack(PBStx.DHEADER, buf)

            # 3. read payload
            payload = self._read_or_die(len_)
            rx_crc = xmodem_crc16(payload, rx_crc)

            # 4. read crc
            buf = self._read_or_die(crc_len)
            crc, = struct.unpack(PBStx.CRCFMT, buf)

            # 5. check crc
            if crc == rx_crc:
                return self._deserialize(seq, payload)
            else:
                raise ReceiveError("CRC mismatch: 0x{:04x} != 0x{:04x}".format(
                    crc, rx_crc))

    def _read_or_die(self, rq_len):
        buf = self.ser.read(rq_len)
        if len(buf) != rq_len:
            raise ReceiveError("short read: {} != {}".format(len(buf), rq_len))

        return buf

    def _deserialize(self, seq, payload):
        pb = msgs.Message()
        pb.ParseFromString(payload)
        return pb

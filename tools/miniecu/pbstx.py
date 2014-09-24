# Protocol Buffers serial transfer
#
# Simple protocol for transfer PB messages defined in miniecu.proto.
# Each message should have MessageId number defined in enum.
#
# Message format:
#    <HDR><SEQ><MSGID><LEN><PAYLOAD[LEN]><CRC>
#
# vim:set ts=4 sw=4 et

__all__ = (
    'PBStx',
    'ReceiveError',
    'msgs',
)

import serial
import threading
import struct
import crcmod.predefined as pcrc

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
    CRCTYPE = 'xmodem'

    def __init__(self, port, baud=57600, sysid=240):
        self.terminate = threading.Event()
        self.ser = serial.Serial(port, baud)
        self.ser.setTimeout(2.0)
        self._tx_seq = 0
        self._rx_seq = 0
        self._tx_crc = pcrc.PredefinedCrc(PBStx.CRCTYPE)
        self._rx_crc = pcrc.PredefinedCrc(PBStx.CRCTYPE)

    def __del__(self):
        self.terminate.set()

    def _reset_crc(self, obj):
        obj.crcValue = obj.initValue

    def send(self, pbobj):
        if not isinstance(pbobj, msgs.Message):
            raise ValueError("Unknown object: " + repr(pbobj))

        payload = pbobj.SerializeToString()
        if len(payload) > PBStx.MAX_LEN:
            raise ValueError("Serialized {} too long: {}".format(repr(pbobj), len(payload)))

        buf = struct.pack(PBStx.EHEADER, PBStx.STX, self._tx_seq, len(payload))
        buf += payload

        self._reset_crc(self._tx_crc)
        self._tx_crc.update(buf[1:])
        buf += struct.pack(PBStx.CRCFMT, self._tx_crc.crcValue)

        self._tx_seq += 1
        self.ser.write(buf)

    def receive(self):
        state = 'STX'
        seq = 0
        len_ = 0
        payload = bytearray()
        crc = 0
        hdr_len = struct.calcsize(PBStx.DHEADER)
        crc_len = struct.calcsize(PBStx.CRCFMT)

        while not self.terminate.is_set():
            if state == 'STX':
                c = self.ser.read(1)
                if len(c) == 0 or c[0] != PBStx.STX:
                    continue

                state = 'HDR'

            if state == 'HDR':
                buf = self._read_or_die(hdr_len)

                self._reset_crc(self._rx_crc)
                self._rx_crc.update(buf)

                seq, len_ = struct.unpack(PBStx.DHEADER, buf)
                state = 'PAYLOAD'

            if state == 'PAYLOAD':
                payload = self._read_or_die(len_)

                self._reset_crc(self._rx_crc)
                self._rx_crc.update(payload)

                state = 'CRC'

            if state == 'CRC':
                buf = self._read_or_die(crc_len)

                crc = struct.unpack(PBStx.CRCFMT, buf)
                if crc == self._rx_crc.crcValue:
                    return self._deserialize(seq, payload)
                else:
                    raise ReceiveError("CRC mismatch")

    def _read_or_die(self, rq_len):
        buf = self.ser.read(rq_len)
        if len(buf) != rq_len:
            raise ReceiveError("short read: {} != {}".format(len(buf), rq_len))

        return buf

    def _deserialize(self, seq, payload):
        pb = msgs.Message()
        pb.ParseFromString(payload)
        return pb

# Protocol Buffers serial transfer
#
# Simple protocol for transfer PB messages defined in miniecu.proto.
# Each message should have MessageId number defined in enum.
#
# Message format:
#    <HDR><SEQ><MSGID><LEN><PAYLOAD[LEN]><CRC>
#
# vim:set ts=4 sw=4 et

import serial
import threading

try:
    from miniecu_pb2 import Status, Command, TimeReference, \
            ParamRequest, ParamSet, ParamValue, StatusText, \
            STATUS as MSGID_STATUS, \
            TIME_REFERENCE as MSGID_TIME_REFERENCE, \
            COMMAND as MSGID_COMMAND, \
            PARAM_REQUEST as MSGID_PARAM_REQUEST, \
            PARAM_SET as MSGID_PARAM_SET, \
            PARAM_VALUE as MSGID_PARAM_VALUE, \
            STATUS_TEXT as MSGID_STATUS_TEXT
except ImportError as ex:
    raise ImportError(str(ex) + ": did you run protoc generator?")


CRC_TABLE = (
    0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d,
    0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65, 0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d,
    0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
    0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd,
    0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2, 0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea,
    0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
    0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32, 0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a,
    0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42, 0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a,
    0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
    0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec, 0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4,
    0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c, 0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44,
    0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
    0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71, 0x78, 0x7f, 0x6a, 0x6d, 0x64, 0x63,
    0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b, 0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13,
    0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb, 0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83,
    0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3
)

def _crc8(crc, buf):
    crc8 = crc
    for b in buf:
        crc8 = CRC_TABLE[crc8 ^ b]

    return crc8



class ReceiveError(Exception):
    pass


class PBStx(object):
    """Protocol Buffers serial transfer protocol"""

    START_HDR = 0xa5
    MAX_LEN = 255

    PKT_TYPES = {
        MSGID_STATUS: Status,
        MSGID_TIME_REFERENCE: TimeReference,
        MSGID_COMMAND: Command,
        MSGID_PARAM_REQUEST: ParamRequest,
        MSGID_PARAM_SET: ParamSet,
        MSGID_PARAM_VALUE: ParamValue,
        MSGID_STATUS_TEXT: StatusText,
    }

    def __init__(self, port, baud=57600, sysid=240):
        self.ser = serial.Serial(port, baud)
        self.ser.setTimeout(2.0)
        self.terminate = threading.Event()
        self._tx_seq = 0
        self._rx_seq = 0

    def __del__(self):
        self.terminate.set()

    def send(self, pbobj):
        msgids = [k for k, v in self.PKT_TYPES.iteritems() if isinstance(pbobj, v)]
        if len(msgids) != 1:
            raise ValueError("Unknown PKT_TYPE for " + repr(pbobj))

        payload = pbobj.SerializeToString()
        if len(payload) > self.MAX_LEN:
            raise ValueError("Serialized {} too long: {}".format(repr(pbobj), len(payload)))

        buf = bytearray((self.START_HDR, self._tx_seq, msgids[0], len(payload)))
        buf += payload
        buf.append(_crc8(0, buf[1:]))

        self._tx_seq += 1

        self.ser.write(buf)

    def receive(self):
        state = 'HDR'
        seq = 0
        sysid = 0
        msgid = 0
        len_ = 0
        payload = bytearray()
        crc = 0

        while not self.terminate.is_set():
            c = self.ser.read(1)
            if len(c) == 0:
                continue

            c = bytearray(c)

            if state == 'HDR':
                if c[0] != self.START_HDR:
                    continue

                state = 'SEQ'

            elif state == 'SEQ':
                seq = c[0]
                crc = _crc8(0, c);
                state = 'MSGID'

            elif state == 'MSGID':
                msgid = c[0]
                crc = _crc8(crc, c)
                state = 'LEN'

            elif state == 'LEN':
                len_ = c[0]
                crc = _crc8(crc, c)
                # len_ <= self.MAX_LEN == True
                state = 'PAYLOAD' if len_ > 0 else 'CRC'

            elif state == 'CRC':
                if crc == c[0]:
                    return self._deserialize(seq, msgid, len_, payload)
                else:
                    raise ReceiveError("CRC error")

            # pass thouth payload receive from LEN
            if state == 'PAYLOAD':
                buf = self.ser.read(len_)
                if len(buf) != len_:
                    raise ReceiveError("short read: {} != {}".format(len(buf), len_))

                payload = bytearray(buf)
                crc = _crc8(crc, payload)
                state = 'CRC'

    def _deserialize(self, seq, msgid, len_, payload):
        if self.PKT_TYPES.has_key(msgid):
            self._rx_seq = seq
            pb = self.PKT_TYPES[msgid]()
            pb.ParseFromString(str(payload))
            return pb

        else:
            raise ReceiveError("Unknown MSGID: {}".format(msgid))


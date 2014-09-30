#!/usr/bin/env python
# -*- coding: utf-8 -*-
# vim:set ts=4 sw=4 et

import argparse
from miniecu import msgs, PBStx, ReceiveError


MSG_MAP = (
    ('param_set', msgs.ParamSet),
    ('memory_dump_request', msgs.MemoryDumpRequest)
)


def wrap_msg(msg):
    for k, t in MSG_MAP:
        if isinstance(msg, t):
            return msgs.Message(**{k: msg})

    raise TypeError("Unknown message type: %s" % repr(msg))


def make_ParamSet(engine_id, param_id, value):
    ps = msgs.ParamSet(engine_id=engine_id, param_id=param_id)
    if isinstance(value, bool):
        ps.value.u_bool = value
    elif isinstance(value, int):
        ps.value.u_int32 = value
    elif isinstance(value, float):
        ps.value.u_float = value
    elif isinstance(value, basestring):
        ps.value.u_string = value
    else:
        raise TypeError("Unsupported param type: %s" % repr(value))

    return wrap_msg(ps)


def main():
    def autoint(s):
        return int(s, 0)

    parser = argparse.ArgumentParser()
    parser.add_argument("device", help="com port device file")
    parser.add_argument("baudrate", help="com port baudrate", type=int, nargs='?', default=57600)
    parser.add_argument("-i", "--id", help="engine id", type=int, default=1)
    parser.add_argument("-t", "--type", help="memory type [0:RAM, 1:SST25]", type=int, default=0)
    parser.add_argument("-a", "--address", help="address", type=autoint, default=0)
    parser.add_argument("-s", "--size", help="size", type=autoint, default=0)

    args = parser.parse_args()

    pbstx = PBStx(args.device, args.baudrate)

    status_period = make_ParamSet(args.id, 'STATUS_PERIOD', 30000)
    dump_enable = make_ParamSet(args.id, 'DEBUG_MEMDUMP', True)

    pbstx.send(status_period)
    pbstx.send(dump_enable)

    dump_request = wrap_msg(msgs.MemoryDumpRequest(
        engine_id=args.id,
        type=args.type,
        stream_id=256,
        address=args.address,
        size=args.size))

    print '-' * 40
    print dump_request
    print '-' * 40

    pbstx.send(dump_request)

    while True:
        try:
            p = pbstx.receive()
            print '-' * 40
            print p
        except ReceiveError as ex:
            print '-' * 40
            print ex


if __name__ == '__main__':
    main()

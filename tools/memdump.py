#!/usr/bin/env python
# -*- coding: utf-8 -*-
# vim:set ts=4 sw=4 et

import argparse
import miniecu


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

    pbstx = miniecu.PBStx(args.device, args.baudrate)

    status_period = miniecu.ParamSet(engine_id=args.id, param_id='STATUS_PERIOD')
    status_period.value.u_int32 = 30000

    dump_enable = miniecu.ParamSet(engine_id=args.id, param_id='DEBUG_MEMDUMP')
    dump_enable.value.u_bool = True

    dump_request = miniecu.MemoryDumpRequest(engine_id=args.id,
                                             type=args.type,
                                             stream_id=256,
                                             address=args.address,
                                             size=args.size)
    print '-' * 40
    print dump_request
    print '-' * 40

    pbstx.send(status_period)
    pbstx.send(dump_enable)
    pbstx.send(dump_request)

    while True:
        try:
            p = pbstx.receive()
            print '-' * 80
            print repr(p)
            print p
        except miniecu.ReceiveError as ex:
            print '-' * 80
            print ex


if __name__ == '__main__':
    main()

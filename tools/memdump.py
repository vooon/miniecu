#!/usr/bin/env python
# -*- coding: utf-8 -*-
# vim:set ts=4 sw=4 et

from __future__ import print_function

import sys
import argparse
import random
from miniecu import msgs, PBStx, ReceiveError
from miniecu.utils import make_ParamSet, wrap_msg


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
    parser.add_argument("-v", "--verbose", help="verbose io print", action='store_true')

    args = parser.parse_args()

    pbstx = PBStx(args.device, args.baudrate)

    pbstx.send(make_ParamSet(args.id, 'STATUS_PERIOD', 30000))
    pbstx.send(make_ParamSet(args.id, 'DEBUG_MEMDUMP', True))

    stream_id = random.randint(0, 0xffffffff)

    dump_request = wrap_msg(msgs.MemoryDumpRequest(
        engine_id=args.id,
        type=args.type,
        stream_id=stream_id,
        address=args.address,
        size=args.size))

    print('=' * 40, file=sys.stderr)
    print(dump_request, file=sys.stderr)
    print('=' * 40, file=sys.stderr)

    pbstx.send(dump_request)

    buf = bytearray()
    while len(buf) < args.size:
        try:
            m = pbstx.receive()
            if m.HasField('memory_dump_page'):
                page = m.memory_dump_page
                if page.stream_id != stream_id:
                    print("wrong stream_id", file=sys.stderr)
                    continue

                idx = page.address - args.address
                if len(buf) < idx:
                    print('page missing, lost %d bytes' % (idx - len(buf)), file=sys.stderr)
                    buf.zfill(idx)

                buf.extend(page.page)

                if args.verbose:
                    print(m, file=sys.stderr)

            elif m.HasField('status_text') or args.verbose:
                print(m, file=sys.stderr)
        except ReceiveError as ex:
            print(repr(ex), file=sys.stderr)

    sys.stdout.write(buf)

if __name__ == '__main__':
    main()

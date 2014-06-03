#!/usr/bin/env python
# -*- coding: utf-8 -*-
# vim:set ts=4 sw=4 et

import argparse
import miniecu


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("device", help="com port device file")
    parser.add_argument("baudrate", help="com port baudrate", type=int, nargs='?', default=57600)

    args = parser.parse_args()

    pbstx = miniecu.PBStx(args.device, args.baudrate)

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

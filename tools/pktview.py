#!/usr/bin/env python
# -*- coding: utf-8 -*-
# vim:set ts=4 sw=4 et

import argparse
import miniecu
from miniecu.utils import recv_print
from miniecu.sql_log import Logger, LoggingWrapper


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("device", help="com port device file")
    parser.add_argument("baudrate", help="com port baudrate", type=int, nargs='?', default=57600)
    parser.add_argument("-l", "--log-db", help="logging to sql db")
    parser.add_argument("-n", "--log-name", help="log name")

    args = parser.parse_args()

    pbstx = miniecu.PBStx(args.device, args.baudrate)

    if args.log_db is not None:
        logger = Logger(args.log_db)
        logger.start(name=args.log_name, source="%s @ %s" % (args.device, args.baudrate))
        pbstx = LoggingWrapper(pbstx, logger)

    recv_print(pbstx)


if __name__ == '__main__':
    main()

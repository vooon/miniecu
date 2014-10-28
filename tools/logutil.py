#!/usr/bin/env python

import argparse
from prettytable import PrettyTable
from miniecu.sql_log import Logger, Log, PBTag, LogData


def do_list(args):
    logger = Logger(args.log_db)

    s = logger.ScopedSession()
    pt = PrettyTable(('#', 'Date', 'Name', 'Source', 'Message count'))
    for l in s.query(Log).all():
        pt.add_row((l.id, l.start_date, l.name, l.source, len(l.log)))

    print pt


def do_message_stat(args):
    logger = Logger(args.log_db)

    s = logger.ScopedSession()
    pt = PrettyTable(('#', 'Message', 'Field', 'Dir', 'Count'))

    log = s.query(Log).filter_by(id=args.log_id).first()
    for tag in s.query(PBTag).all():
        q = s.query(LogData).filter_by(log=log, pb_tag=tag)
        cnt = q.filter_by(direction='SEND').count()
        if cnt:
            pt.add_row((tag.id, tag.message_type, tag.field, 'SEND', cnt))

        cnt = q.filter_by(direction='RECV').count()
        if cnt:
            pt.add_row((tag.id, tag.message_type, tag.field, 'RECV', cnt))

    print pt


def main(argv=None):
    parser = argparse.ArgumentParser(description="sql log tool")
    parser.add_argument('-v', '--verbose', action='store_true', help="verbose output")
    subarg = parser.add_subparsers()

    list_args = subarg.add_parser('list', help='list all logs in DB')
    list_args.set_defaults(func=do_list)
    list_args.add_argument('log_db')

    mstat_args = subarg.add_parser('mstat', help='message statistics for Log')
    mstat_args.set_defaults(func=do_message_stat)
    mstat_args.add_argument('log_db')
    mstat_args.add_argument('log_id')

    args = parser.parse_args(argv)
    args.func(args)


if __name__ == '__main__':
    main()

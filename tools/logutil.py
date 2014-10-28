#!/usr/bin/env python

import csv
import argparse
from prettytable import PrettyTable
from miniecu.sql_log import Logger, Log, PBTag, LogData
from miniecu import msgs


def pb_to_kv_pairs(pb, prefix=()):
    for desc, val in pb.ListFields():
        if isinstance(val, (int, float, bool, long, basestring)):
            yield '.'.join(prefix + (desc.name, )), val
        else:
            for pair in pb_to_kv_pairs(val, prefix + (desc.name, )):
                yield pair


def pb_desc_to_keys(pb, prefix=()):
    if hasattr(pb, 'DESCRIPTOR'):
        fields = pb.DESCRIPTOR.fields
    else:
        fields = pb.message_type.fields

    for desc in fields:
        if desc.type != desc.TYPE_MESSAGE:
            yield '.'.join(prefix + (desc.name, ))
        else:
            for key in pb_desc_to_keys(desc, prefix + (desc.name, )):
                yield key


def do_list(args):
    """List all logs avalable in database"""
    logger = Logger(args.log_db)

    s = logger.ScopedSession()
    pt = PrettyTable(('#', 'Date', 'Name', 'Source', 'Message count'))
    for l in s.query(Log).all():
        pt.add_row((l.id, l.start_date, l.name, l.source, len(l.log)))

    print pt


def do_message_stat(args):
    """Collect message statistics"""
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


def do_csv_export(args):
    """Export miniecu.Status to CSV"""
    logger = Logger(args.log_db)

    s = logger.ScopedSession()
    log = s.query(Log).filter_by(id=args.log_id).first()
    status_tag = s.query(PBTag).filter_by(message_type='miniecu.Status').first()

    wr = csv.writer(args.csv_file, dialect='excel-tab')
    args.csv_file.write('# Date: {}\tName: {}\n'.format(log.start_date, log.name))

    header = [k for k in pb_desc_to_keys(msgs.Status)]
    wr.writerow(header)

    for log_data in s.query(LogData).filter_by(log=log, pb_tag=status_tag).all():
        msg = msgs.Message()
        msg.ParseFromString(log_data.pb_message)

        row = ['NaN'] * len(header)
        for k, v in pb_to_kv_pairs(msg.status):
            i = header.index(k)
            row[i] = v

        wr.writerow(row)


def main(argv=None):
    parser = argparse.ArgumentParser(description="sql log tool")
    parser.add_argument('-v', '--verbose', action='store_true', help="verbose output")
    subarg = parser.add_subparsers()

    list_args = subarg.add_parser('list', help=do_list.__doc__)
    list_args.set_defaults(func=do_list)
    list_args.add_argument('log_db')

    mstat_args = subarg.add_parser('mstat', help=do_message_stat.__doc__)
    mstat_args.set_defaults(func=do_message_stat)
    mstat_args.add_argument('log_db')
    mstat_args.add_argument('log_id')

    csvexport_args = subarg.add_parser('csvexport', help=do_csv_export.__doc__)
    csvexport_args.set_defaults(func=do_csv_export)
    csvexport_args.add_argument('log_db')
    csvexport_args.add_argument('log_id')
    csvexport_args.add_argument('csv_file', type=argparse.FileType('w'))

    args = parser.parse_args(argv)
    args.func(args)


if __name__ == '__main__':
    main()

# -*- python -*-

from __future__ import print_function

from pbstx import ReceiveError, msgs
from sql_log import Logger, LoggingWrapper


MESSAGE_FIELD_TYPE = (
    ('command', msgs.Command),
    ('param_request', msgs.ParamRequest),
    ('param_set', msgs.ParamSet),
    ('time_reference', msgs.TimeReference),
    ('memory_dump_request', msgs.MemoryDumpRequest)
)

PARAM_TYPE_FIELD_TYPE = (
    ('u_bool', bool),
    ('u_int32', int),
    ('u_float', float),
    ('u_string', basestring)
)


def wrap_msg(msg):
    for k, t in MESSAGE_FIELD_TYPE:
        if isinstance(msg, t):
            return msgs.Message(**{k: msg})

    raise TypeError("Unknown message type: %s" % repr(msg))


def make_ParamSet(engine_id, param_id, value):
    ps = msgs.ParamSet(engine_id=engine_id, param_id=param_id)
    for k, t in PARAM_TYPE_FIELD_TYPE:
        if isinstance(value, t):
            setattr(ps.value, k, value)
            return wrap_msg(ps)

    raise TypeError("Unsupported param type: %s" % repr(value))


def make_Command(engine_id, operation):
    cmd = msgs.Command(engine_id=engine_id, operation=operation)
    return wrap_msg(cmd)


def value_ParamType(pt):
    for k, t in PARAM_TYPE_FIELD_TYPE:
        if pt.HasField(k):
            return getattr(pt, k)

    raise ValueError('Incorrect ParamType')


def wrap_logger(pbstx, log_db=None, log_name=None, source=None, date=None):
    if log_db is not None:
        logger = Logger(log_db)
        logger.start(name=log_name, source=source, start_date=date)
        pbstx = LoggingWrapper(pbstx, logger)

    return pbstx


def recv_print(pbstx):
    while True:
        try:
            m = pbstx.receive()
            print('-' * 40)
            print(m)
        except ReceiveError as ex:
            print('-' * 40)
            print(repr(ex))

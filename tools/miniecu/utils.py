# -*- python -*-

from __future__ import print_function

import sys
from pbstx import ReceiveError, msgs
from sql_log import Logger, LoggingWrapper


def wrap_msg(msg):
    if isinstance(msg, msgs.Command):
        return msgs.Message(command=msg)
    elif isinstance(msg, msgs.ParamRequest):
        return msgs.Message(param_request=msg)
    elif isinstance(msg, msgs.ParamSet):
        return msgs.Message(param_set=msg)
    elif isinstance(msg, msgs.TimeReference):
        return msgs.Message(time_reference=msg)
    elif isinstance(msg, msgs.MemoryDumpRequest):
        return msgs.Message(memory_dump_request=msg)
    else:
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


def make_Command(engine_id, operation):
    cmd = msgs.Command(engine_id=engine_id, operation=operation)
    return wrap_msg(cmd)


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

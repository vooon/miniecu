# -*- python -*-

import logging
import threading
from miniecu import PBStx, ReceiveError, msgs
from miniecu.utils import wrap_logger, wrap_msg, make_ParamSet, make_Command, \
    value_ParamType
from models.param import ParamManager
from models.status import StatusManager
from models.status_text import StatusTextManager


log = logging.getLogger('comm')


class CommThread(threading.Thread):
    def __init__(self, port, baud=57600, engine_id=1, log_db=None, log_name=None):
        super(CommThread, self).__init__(name="CommThread")
        self.daemon = True
        self.terminate = threading.Event()

        self.HANDLERS = (
            ('status', self.handle_status),
            ('param_value', self.handle_param_value),
            ('command', self.handle_command),
            ('status_text', self.handle_status_text),
        )

        self.engine_id = engine_id
        self.pbstx = wrap_logger(PBStx(port, baud), log_db, log_name, "%s:%s" % (log_db, log_name))
        ParamManager().register_comm(self)
        self.start()

    def __del__(self):
        self.stop()

    def stop(self):
        self.terminate.set()

    def run(self):
        while not self.terminate.is_set():
            try:
                m = self.pbstx.receive()
                self.dispatch_message(m)
            except ReceiveError as ex:
                log.error(repr(ex))

    def dispatch_message(self, msg):
        for k, h in self.HANDLERS:
            if msg.HasField(k):
                return h(getattr(msg, k))

        log.warn("Unsupported message: %s", msg)

    def handle_status(self, status):
        StatusManager().update_status(status)

    def handle_command(self, command):
        pass

    def handle_param_value(self, param_value):
        try:
            ParamManager().update_param(param_value.param_id,
                                        param_value.param_index,
                                        param_value.param_count,
                                        value_ParamType(param_value.value))
        except ValueError as ex:
            log.error(repr(ex))

    def handle_status_text(self, status_text):
        StatusTextManager().add_message(status_text)

    def param_set(self, param_id, value):
        self.pbstx.send(make_ParamSet(self.engine_id, param_id, value))

    def param_request(self, param_id=None, param_index=None):
        pr = msgs.ParamRequest(engine_id=self.engine_id)
        if param_id:    pr.param_id = param_id
        if param_index: pr.param_index = param_index

        self.pbstx.send(wrap_msg(pr))

    def command(self, operation):
        self.pbstx.send(make_Command(self.engine_id, operation))

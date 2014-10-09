# -*- python -*-

import logging
from time import time
from utils import singleton, Signal
from miniecu import msgs
from commmgr import CommManager

ecu_log = logging.getLogger('ECU')


@singleton
class StatusTextManager(object):
    def __init__(self):
        self.messages = []
        self.sig_changed = Signal()
        CommManager().register_model(self)

    @property
    def last_message(self):
        return self.messages[-1]

    def add_message(self, msg):
        self.messages.append((time(), msg))
        if msg.severity == msgs.StatusText.DEBUG:
            ecu_log.debug(msg.text)
        elif msg.severity == msgs.StatusText.INFO:
            ecu_log.info(msg.text)
        elif msg.severity == msgs.StatusText.WARN:
            ecu_log.warn(msg.text)
        elif msg.severity == msgs.StatusText.ERROR:
            ecu_log.error(msg.text)
        elif msg.severity == msgs.StatusText.FAILURE:
            ecu_log.fatal(msg.text)
        else:
            ecu_log.debug("SEV(%s): %s", msg.severity, msg.text)

        self.sig_changed.emit()

    def clear(self):
        self.messages = []
        # self.sig_changed.emit()


StatusTextManager()

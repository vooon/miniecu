# -*- python -*-

import logging
import threading
from utils import singleton, Signal
from commmgr import CommManager
from miniecu import msgs

log = logging.getLogger(__name__)


@singleton
class CommandManger(object):
    def __init__(self):
        self._event = threading.Event()
        self._operation = msgs.Command.UNKNOWN
        self._response = msgs.Command.NAK
        CommManager().register_model(self)

    def clear(self):
        pass

    def handle_message(self, cmd):
        self._operation = cmd.operation
        self._response = cmd.response
        self._event.set()

    def _str_pb_enum(self, op):
        return msgs.Command.Operation.Name(op)

    def command(self, op):
        log.debug("Send command: %s", self._str_pb_enum(op))
        self._event.clear()
        self._operation = msgs.Command.UNKNOWN
        self._response = msgs.Command.NAK
        CommManager().command(op)

        self._event.wait(10.0)
        if self._operation != op:
            log.error("Wrong response. request %s, response %s",
                      self._str_pb_enum(op),
                      self._str_pb_enum(self._operation))
            return False

        else:
            log.debug("Response to %s: %s", self._str_pb_enum(op),
                      'ACK' if self._response == msgs.Command.ACK else 'NAK')
            return self._response == msgs.Command.ACK

    def load_config(self):
        return self.command(msgs.Command.LOAD_CONFIG)

    def save_config(self):
        return self.command(msgs.Command.SAVE_CONFIG)

    def ignition_enable(self):
        return self.command(msgs.Command.IGNITION_ENABLE)

    def ignition_disable(self):
        return self.command(msgs.Command.IGNITION_DISABLE)

    def starter_enable(self):
        return self.command(msgs.Command.STARTER_ENABLE)

    def starter_disable(self):
        return self.command(msgs.Command.STARTER_DISABLE)

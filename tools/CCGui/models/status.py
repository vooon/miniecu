# -*- python -*-

from time import time
from utils import singleton, Signal
from miniecu import msgs

# Dummy model manager, only stores last miniecu.Status message
#
# TODO: make required changes for proper status/plot pages


@singleton
class StatusManager(object):
    def __init__(self):
        self.last_message = None
        self.sig_changed = Signal()

    def update_status(self, msg):
        self.last_message = msg
        self.sig_changed.emit()

    def clear(self):
        pass


StatusManager()

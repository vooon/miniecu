# -*- python -*-

import time
import logging
from utils import singleton, Signal
from commmgr import CommManager
from gi.repository import GObject


@singleton
class TimeRefManager(object):
    def __init__(self):
        self._sync_id = None
        CommManager().register_model(self)

    def clear(self):
        if self._sync_id:
            GObject.source_remove(self._sync_id)
            self._sync_id = None

    def start(self):
        self.sync()
        self._sync_id = GObject.timeout_add(5000, self.sync)

    def sync(self):
        CommManager().time_reference(long(time.time() * 1000))
        return True

    def handle_message(self, time_ref):
        self.last_response = time_ref

# -*- python -*-

from time import time
from utils import singleton
from miniecu import msgs

# Dummy model manager, only stores last miniecu.Status message
#
# TODO: make required changes for proper status/plot pages


@singleton
class StatusManager(object):
    def __init__(self):
        self.last_message = None

    def update_status(self, msg):
        self.last_message = msg

    def clear(self):
        pass


StatusManager()

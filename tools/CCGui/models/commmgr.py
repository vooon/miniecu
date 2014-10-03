# -*- python -*-

from utils import singleton


@singleton
class CommManager(object):
    """
    Communication manager, holds connection instance, used by other managers
    to send messages to ECU
    """
    def __init__(self):
        self._comm = None
        self._models = []

    def register_model(self, model):
        self._models.append(model)

    def register(self, comm):
        if self._comm is not None:
            self._comm.stop()
            del self._comm

        self._comm = comm

    def clear(self):
        """Clear all models and remove communication thread"""
        for m in self._models:
            m.clear()

        if self._comm is not None:
            self._comm.stop()
            del self._comm
            self._comm = None

    def __getattr__(self, attr):
        return getattr(self._comm, attr)


CommManager()

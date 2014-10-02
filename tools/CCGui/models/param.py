# -*- python -*-

import weakref
import threading
from utils import singleton


class Parameter(object):
    def __init__(self, param_id, param_index, value):
        self.param_id = param_id
        self.param_index = param_index
        self._value = value
        self._init_type = type(value)
        self._changed = False

    def __repr__(self):
        return "<{} {} #{} {}({}){}>".format(
            Parameter.__name__, self.param_id, self.param_index,
            self._init_type.__name__, self._value,
            '*' if self._changed else ''
        )

    @property
    def is_changed(self):
        return self._changed

    @property
    def value(self):
        return self._value

    @value.setter
    def value(self, value):
        # only set value it it pass validator (type constructor)
        try:
            self._value = self._init_type(value)
            self._changed = True
        except ValueError:
            pass

    def validate(self, value):
        # TODO: more paranoic validate process
        try:
            self._init_type(value)
        except ValueError:
            return False

        return True


@singleton
class ParamManager(object):
    def __init__(self):
        self.parameters = {}
        self.missing_ids = set()
        self.comm = None
        self._event = threading.Event()

    @property
    def changed(self):
        return [p for p in self.parameters.values() if p.is_changed]

    def register_comm(self, comm):
        self.comm = weakref.proxy(comm)

    def clear(self):
        self.parameters.clear()

    def update_param(self, param_id, param_index, param_count, value):
        if len(self.missing_ids) == 0:
            self.missing_ids.update(range(param_count))

        p = self.parameters.get(param_id)
        if p:
            p._value = value
            p._changed = False
        else:
            self.parameters[param_id] = Parameter(param_id, param_index, value)

        self.missing_ids.discard(param_index)
        if len(self.missing_ids) == 0:
            self._event.set()

    def retrieve_all(self):
        self.missing_ids = set()
        self._event.clear()

        self.comm.param_request()
        self._event.wait(10.0)

        # not nesessary: try to request missing params
        if len(self.missing_ids) > 0:
            self._event.clear()
            for idx in self.missing_ids:
                self.comm.param_request(param_index=idx)

            self._event.wait(10.0)

        return len(self.missing_ids) == 0

    def sync(self):
        to_sync = self.changed
        self.missing_ids = set((p.param_index for p in to_sync))
        self._event.clear()
        for p in to_sync:
            self.comm.param_set(p.param_id, p.value)

        self._event.wait(10.0)
        return len(self.missing_ids) == 0


# initialize manager at module loading
ParamManager()

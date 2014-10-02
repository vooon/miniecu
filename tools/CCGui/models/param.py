# -*- python -*-

from signalslot import Signal
from ..utils import singleton


class Parameter(object):
    def __init__(self, param_id, value):
        self.param_id = param_id
        self._value = value
        self._init_type = type(value)
        self._changed = False
        self.updated = Signal()

    def __repr__(self):
        return "<{} {} {}({})>".format(Parameter.__name__, self.param_id,
                                       str(self._init_type), self._value)

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

    @property
    def changed(self):
        return [1 for p in self.parameters.values() if p.is_changed]

    def clear(self):
        self.parameters.clear()

    def update_param(self, param_id, value):
        p = self.parameters.get(param_id)
        if p:
            p.value = value
        else:
            self.parameters[param_id] = Parameter(param_id, value)


# initialize manager at module loading
ParamManager()

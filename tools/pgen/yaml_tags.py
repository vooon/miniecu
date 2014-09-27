# -*- python -*-

import yaml as Y


class DefinitionError(Exception):
    pass


class Parameter(object):
    """Base class for yaml parameter objects"""

    MAX_NAME = 15
    MAX_STRING = 15

    _accept_enum = False
    _accept_values = False
    _need_minmax = False
    _norm_type = str

    def __init__(self, **kvargs):
        if len(kvargs) > 0:
            self.__setstate__(kvargs)

    def __repr__(self):
        fields = ("{}: {}".format(k, v) for k, v in self.__dict__.iteritems()
                  if not k.startswith('_') and not callable(v))

        return "{}({})".format(self.__class__.__name__, ", ".join(sorted(fields)))

    def raise_definition_error(self, tag):
        raise DefinitionError("{}: {}: not defined".format(self.__class__.__name__, tag))

    def get_minmax(self, minmax):
        if self._accept_values and self.values is not None:
            values = self.values
        elif self._accept_enum and self.enum is not None:
            values = self.enum.values()
        else:
            self.raise_definition_error(minmax.__name__)

        return minmax((self._norm_type(v) for v in values))

    def __setstate__(self, definition):
        # copy common fields
        self.desc = definition.get('desc')
        self.var = definition.get('var')
        self.onchange = definition.get('onchange')
        self.default = definition.get('default')
        # common flags
        self.read_only = definition.get('read_only', False)
        self.dont_save = definition.get('dont_save', False)

        if self.desc is None:
            self.raise_definition_error('desc')

        if self._accept_values:
            self.values = definition.get('values')

        if self._accept_enum:
            self.enum = definition.get('enum')

        if self._need_minmax:
            # if min/max not defined try to calculate it
            self.min = definition.get('min')
            if self.min is None:
                self.min = self.get_minmax(min)
            else:
                self.min = self._norm_type(self.min)

            self.max = definition.get('max')
            if self.max is None:
                self.max = self.get_minmax(max)
            else:
                self.max = self._norm_type(self.max)

        if self.default is None:
            # if default not defined try to find it
            if self._accept_values and self.values is not None:
                self.default = self._norm_type(self.values[0])
            elif self._accept_enum and self.enum is not None:
                self.default = self._norm_type(self.enum.values()[0])
            elif self._need_minmax:
                self.default = self.min
            elif self._norm_type is bool:
                self.default = False
            else:
                self.raise_definition_error('default')

        else:
            # accept enum identifiers
            if self._accept_enum and isinstance(self.default, basestring):
                self.default = self.enum[self.default]

            # normalize default value
            self.default = self._norm_type(self.default)


class PtBool(Y.YAMLObject, Parameter):
    yaml_tag = u'!ptbool'

    _accept_enum = False
    _accept_values = False
    _need_minmax = False
    _norm_type = bool


class PtInt32(Y.YAMLObject, Parameter):
    yaml_tag = u'!ptint32'

    _accept_enum = True
    _accept_values = True
    _need_minmax = True
    _norm_type = int


class PtFloat(Y.YAMLObject, Parameter):
    yaml_tag = u'!ptfloat'

    _accept_enum = False
    _accept_values = True
    _need_minmax = True
    _norm_type = float


class PtString(Y.YAMLObject, Parameter):
    yaml_tag = u'!ptstring'

    _accept_enum = False
    _accept_values = True
    _need_minmax = False
    _norm_type = str

# -*- python -*-

import logging
from utils import singleton
from pgen.pgen import ParameterTable
from os import path

log = logging.getLogger(__name__)

DEF_FILE = 'parameters.yaml'
DEF_PATH = [
    # .
    path.dirname(path.abspath(__file__)),
    # ..
    path.dirname(path.dirname(path.abspath(__file__))),
    # ../../../fw/
    path.join(path.dirname(path.dirname(
        path.dirname(path.dirname(path.abspath(__file__))))), 'fw'),
]


@singleton
class ParamDef(object):
    def __init__(self):
        file_ = self._find_definition_file()
        if file_ is None:
            return

        self._table = ParameterTable()
        self._table.load(file_)
        log.info("Definition loaded.")

    def _find_definition_file(self):
        for dir_ in DEF_PATH:
            f = path.join(dir_, DEF_FILE)
            if path.exists(f):
                log.info("Param definition: %s", f)
                return f

        log.warn("Param definition not found")
        return None

    def desc(self, param_id):
        if not hasattr(self, '_table'):
            return 'No description file'

        pd = self._table.parameters.get(param_id)
        if pd:
            return pd.desc
        else:
            return 'Description not found'

    def minmax(self, param_id):
        if not hasattr(self, '_table'):
            return (-1e6, 1e6)

        pd = self._table.parameters.get(param_id)
        if pd:
            return (pd.min, pd.max)
        else:
            return (-1e6, 1e6)

    def fixed_values(self, param_id):
        if not hasattr(self, '_table'):
            return None

        pd = self._table.parameters.get(param_id)
        if pd:
            if pd._accept_enum and pd.enum:
                return pd.enum.iteritems()
            elif pd._accept_values and pd.values:
                return ((str(v), v) for v in pd.values)

        return None

# Init at start
ParamDef()

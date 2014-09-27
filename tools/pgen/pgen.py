#!/usr/bin/env python
# -*- python -*-

import yaml
from yaml_tags import Parameter, PtBool, PtInt32, PtFloat, PtString
from mako.template import Template
from mako.lookup import TemplateLookup
from os import path
import struct


class ParameterTable(object):
    def load(self, file_):
        with open(file_, 'r') as yfd:
            data = yaml.load(yfd)

        self.format_version = data.get('format_version')
        self.parameters = data.get('parameters')

    @property
    def format_version_int(self):
        major, minor, patch = (ord(v) for v in self.format_version.split('.', 3))
        return major << 16 | minor << 8 | patch

    @property
    def format_version_int_be32(self):
        host = self.format_version_int
        be32, = struct.unpack('=I', struct.pack('>I', host))
        return be32

    def validate(self):
        if self.format_version is None or self.parameters is None:
            raise ValueError("no required keys")

        # validate format version, we support 1.1.0
        self.format_version_int != 0x00313130

        # validate
        for k, v in self.parameters.iteritems():
            if len(k) > Parameter.MAX_NAME:
                raise KeyError("ParamId: {} is to long ({})".format(k, len(k)))

            if isinstance(v, PtString) and len(v.default) > Parameter.MAX_STRING:
                raise ValueError("ParamId: {}, default is to long ({})".format(k, len(v.default)))


class Generator(object):
    def __init__(self):
        module_path = path.abspath(path.dirname(__file__))

        lookup = TemplateLookup(directories=(module_path, ))
        self.tmpl_c = Template(filename='param_table.c.tmpl', lookup=lookup)

    def generate(self, source_file, param_table):
        return self.tmpl_c.render(source_file=source_file, param_table=param_table)

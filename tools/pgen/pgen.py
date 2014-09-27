#!/usr/bin/env python
# -*- python -*-

import time
import struct
import argparse
import yaml
from yaml_tags import Parameter, PtBool, PtInt32, PtFloat, PtString
from mako.template import Template
from os import path
from sys import exit


class ParameterTable(object):
    def load(self, file_):
        if isinstance(file_, basestring):
            with open(file_, 'r') as yfd:
                data = yaml.load(yfd)

        else:
            with file_:
                data = yaml.load(file_)

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

    @property
    def parameters_with_enum(self):
        return dict(((k, v) for k, v in self.parameters.iteritems()
                     if v._accept_enum and v.enum is not None))

    @property
    def parameters_with_onchange(self):
        return dict(((k, v) for k, v in self.parameters.iteritems()
                     if v.onchange is not None))

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
                raise ValueError("ParamId: {}, default is too long ({})".format(k, len(v.default)))

            if isinstance(v, PtString) and v.values is not None:
                for sv in v.values:
                    if len(sv) > Parameter.MAX_STRING:
                        raise ValueError("ParamId: {}, value: {} is too long ({})".format(k, sv, len(sv)))


class Generator(object):
    def __init__(self):
        module_path = path.abspath(path.dirname(__file__))

        self.tmpl_c = Template(filename=path.join(module_path, 'param_table.c.tmpl'))
        self.tmpl_h = Template(filename=path.join(module_path, 'param_table.h.tmpl'))

    def generate(self, source_file, out_dir, param_table):
        render_agrs = dict(
            gen_time=time.strftime("%a, %d %b %Y %H:%M:%S %Z"),
            source_file=source_file,
            param_table=param_table)

        gen_h = self.tmpl_h.render(**render_agrs)
        gen_c = self.tmpl_c.render(**render_agrs)

        with open(path.join(out_dir, 'param_table.h'), 'w') as fd:
            fd.write(gen_h)

        with open(path.join(out_dir, 'param_table.c'), 'w') as fd:
            fd.write(gen_c)


def main(argv=None):
    def dirtype(dir_):
        if not path.isdir(dir_):
            raise argparse.ArgumentTypeError("not directory")
        else:
            return dir_

    parser = argparse.ArgumentParser(description='Parameter table generator')
    parser.add_argument('definition', type=argparse.FileType('r'), help='Param definition file')
    parser.add_argument('-o', '--out-dir', type=dirtype, help='Output directory')

    args = parser.parse_args(argv)

    if args.out_dir is None:
        args.out_dir = ''

    param_table = ParameterTable()
    param_table.load(args.definition)

    generator = Generator()
    generator.generate(path.abspath(args.definition.name), args.out_dir,
                       param_table)

    exit(0)


if __name__ == '__main__':
    main()

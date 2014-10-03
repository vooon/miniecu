# -*- python -*-

from os import path
from gi.repository import Gtk

MODULE_PATH = path.abspath(path.dirname(__file__))


def get_builder(file_):
    builder = Gtk.Builder()
    builder.add_from_file(path.join(MODULE_PATH, file_))

    return builder

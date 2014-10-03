# -*- python -*-

from os import path
from gi.repository import Gtk

MODULE_PATH = path.abspath(path.dirname(__file__))

builder = Gtk.Builder()
builder.add_from_file(path.join(MODULE_PATH, 'ccgui.glade'))
builder.add_from_file(path.join(MODULE_PATH, 'conn_dlg.glade'))

# -*- python -*-

from os import path
from gi.repository import Gtk

MODULE_PATH = path.abspath(path.dirname(__file__))


class ConnDialog(object):
    def __init__(self):
        self.builder = Gtk.Builder()
        self.builder.add_from_file(path.join(MODULE_PATH, 'conn_dlg.glade'))

        self.dialog = self.builder.get_object('conn_dialog')
        self.dialog.connect("delete-event", Gtk.main_quit)
        self.dialog.show_all()

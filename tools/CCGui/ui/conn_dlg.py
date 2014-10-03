# -*- python -*-

from gi.repository import Gtk
from builder import builder


class ConnDialog(object):
    def __init__(self):
        self.dialog = builder.get_object('conn_dialog')
        self.dialog.connect("delete-event", Gtk.main_quit)
        self.dialog.show_all()

# -*- python -*-

from gi.repository import Gtk
from builder import builder


class CCGuiApplication(object):
    def __init__(self):
        self.window = builder.get_object('ccgui_window')
        self.window.connect("delete-event", Gtk.main_quit)
        self.window.show_all()

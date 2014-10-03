# -*- python -*-

from os import path
from gi.repository import Gtk

MODULE_PATH = path.abspath(path.dirname(__file__))


class CCGuiApplication(object):
    def __init__(self):
        self.builder = Gtk.Builder()
        self.builder.add_from_file(path.join(MODULE_PATH, 'ccgui.glade'))

        self.window = self.builder.get_object('ccgui_window')
        self.window.connect("delete-event", Gtk.main_quit)
        self.window.show_all()

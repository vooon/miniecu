# -*- python -*-

from gi.repository import Gtk
from builder import get_builder
import logging
from ui.conn_dlg import ConnDialog


class CCGuiApplication(object):
    def __init__(self):
        builder = get_builder('ccgui.glade')
        self.window = builder.get_object('ccgui_window')
        builder.connect_signals(self)

        self.window.show_all()

    def on_ccgui_window_delete_event(self, *args):
        logging.info("onQuit")
        Gtk.main_quit(*args)

    def on_connect_activate(self, *args):
        logging.debug("onConnect")
        dialog = ConnDialog()
        if dialog.run() == Gtk.ResponseType.OK:
            pass

    def on_disconnect_activate(self, *args):
        logging.debug("onDisConnect")

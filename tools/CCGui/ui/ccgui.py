# -*- python -*-

import serial
import logging
from gi.repository import Gtk
from ui.builder import get_builder
from ui.conn_dlg import ConnDialog

from models import CommManager
from comm import CommThread


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
            args = dialog.get_result_destroy()
            self.create_comm(*args)

    def on_disconnect_activate(self, *args):
        logging.debug("onDisConnect")
        CommManager().clear()

    def create_comm(self, port, baudrate, engine_id, log_file, log_name):
        try:
            CommManager().clear()
            CommManager().register(CommThread(port, baudrate, engine_id, None, log_name))
            logging.info("DEV: %s: opened", port)
        except serial.SerialException as ex:
            logging.error("DEV: %s: %s", port, repr(ex))



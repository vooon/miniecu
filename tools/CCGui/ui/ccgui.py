# -*- python -*-

import serial
import logging
from gi.repository import Gtk
from ui.builder import get_builder
from ui.conn_dlg import ConnDialog
from ui.param_item import ParamBoxRow

from models import CommManager, ParamManager
from comm import CommThread


class CCGuiApplication(object):
    def __init__(self):
        builder = get_builder('ccgui.ui')
        self.window = builder.get_object('ccgui_window')
        builder.connect_signals(self)

        self.window.set_default_size(640, 480)
        self.window.show_all()

        # store ref to some widgets
        self.param_listbox = builder.get_object('param_listbox')
        self.param_rows = {}

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
        self.update_params()
        logging.info("DEV: closed")

    def create_comm(self, port, baudrate, engine_id, log_file, log_name):
        try:
            CommManager().clear()
            CommManager().register(CommThread(port, baudrate, engine_id, None, log_name))
            logging.info("DEV: %s: opened", port)
        except serial.SerialException as ex:
            logging.error("DEV: %s: %s", port, repr(ex))

    def on_param_request_clicked(self, *args):
        logging.debug("onParamRequest")

        ParamManager().retrieve_all()
        self.update_params()

    def on_param_send_clicked(self, *args):
        logging.debug("onParamSend")

        ParamManager().sync()
        self.update_params()

    def on_param_load_clicked(self, *args):
        logging.debug("onParamLoad")

    def on_param_save_clicked(self, *args):
        logging.debug("onParamSave")

    def update_params(self):
        for k, p in ParamManager().parameters.iteritems():
            row = self.param_rows.get(k)
            if row:
                row.update()
            else:
                row = ParamBoxRow(p)
                self.param_rows[k] = row
                self.param_listbox.add(row)

        # remove old rows
        for k, row in self.param_rows.items():
            if not ParamManager().parameters.has_key(k):
                self.param_listbox.remove(row)
                del self.param_rows[k]

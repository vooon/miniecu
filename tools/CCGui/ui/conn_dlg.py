# -*- python -*-

from gi.repository import Gtk
from builder import get_builder
import logging

dlg_log = logging.getLogger('dlg')


class ConnDialog(object):
    def __init__(self):
        builder = get_builder('conn_dlg.glade')
        self.dialog = builder.get_object('conn_dialog')
        builder.connect_signals(self)

    def run(self, *args):
        self.dialog.show_all()
        return self.dialog.run(*args)

    def on_conn_dialog_close(self, *args):
        dlg_log.debug("onCancel")
        self.dialog.destroy()

    def on_ok_button_clicked(self, *args):
        dlg_log.debug("onOk")
        self.dialog.response(Gtk.ResponseType.OK)

    def on_log_switch_activate(self, *args):
        dlg_log.debug("onLogSwitch")


# -*- python -*-

from gi.repository import Gtk
from builder import get_builder
import logging
import serial
import serial.tools.list_ports as list_ports
from os import path

log = logging.getLogger(__name__)


class ConnDialog(object):
    def __init__(self):
        builder = get_builder('conn_dlg.ui')
        self.dialog = builder.get_object('conn_dialog')
        builder.connect_signals(self)

        # fill port combobox
        device_store = Gtk.ListStore(str, str)
        for dev, name, info in sorted(list_ports.comports()):
            log.debug("COM: %s %s", dev, info)
            if info == 'n/a':
                continue

            device_store.append([dev, info])

        port_combo = builder.get_object('port_combo')
        port_combo.set_model(device_store)
        text_renderer = Gtk.CellRendererText()
        port_combo.pack_start(text_renderer, True)
        port_combo.add_attribute(text_renderer, 'text', 1)
        port_combo.set_entry_text_column(0)

        # fill baudrate combobox
        baud_store = Gtk.ListStore(int, str)
        for br in sorted(serial.baudrate_constants.keys()):
            baud_store.append([br, str(br)])

        baudrate_combo = builder.get_object('baudrate_combo')
        baudrate_combo.set_model(baud_store)
        text_renderer = Gtk.CellRendererText()
        baudrate_combo.pack_start(text_renderer, True)
        baudrate_combo.add_attribute(text_renderer, 'text', 1)
        baudrate_combo.set_active_id(str(57600))

        engine_id = builder.get_object('engine_id')
        engine_id.set_value(1)
        engine_id.set_increments(1, 10)
        engine_id.set_range(1, 0xffff)

        # save ref to widgets
        self.port_combo = port_combo
        self.baudrate_combo = baudrate_combo
        self.engine_id = engine_id
        self.log_file = builder.get_object('sqlite_file')
        self.log_name = builder.get_object('log_name')

    def run(self, *args):
        self.dialog.show_all()
        return self.dialog.run(*args)

    def get_result_destroy(self):
        port = ''
        it = self.port_combo.get_active_iter()
        if it is not None:
            model = self.port_combo.get_model()
            port = model[it][0]
        else:
            edit = self.port_combo.get_child()
            port = edit.get_text()

        baudrate = 56700
        it = self.baudrate_combo.get_active_iter()
        if it is not None:
            model = self.baudrate_combo.get_model()
            baudrate = model[it][0]

        log_file = self.log_file.get_filename()
        if log_file:
            log_file = 'sqlite:///' + path.abspath(log_file)

        engine_id = self.engine_id.get_value_as_int()
        log_name = self.log_name.get_text()

        self.dialog.destroy()
        log.debug("DEV: %s:%d, ECU: %d, LOG: %s (%s)", port, baudrate, engine_id, log_file, log_name)
        return (port, baudrate, engine_id, log_file, log_name)

    def on_conn_dialog_close(self, *args):
        log.debug("onCancel")
        self.dialog.destroy()

    def on_ok_button_clicked(self, *args):
        log.debug("onOk")
        self.dialog.response(Gtk.ResponseType.OK)

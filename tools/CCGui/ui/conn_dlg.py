# -*- python -*-

from gi.repository import Gtk
from builder import get_builder
import logging
import serial
import serial.tools.list_ports as list_ports

dlg_log = logging.getLogger('dlg')


class ConnDialog(object):
    def __init__(self):
        builder = get_builder('conn_dlg.glade')
        self.dialog = builder.get_object('conn_dialog')
        builder.connect_signals(self)

        # fill port combobox
        device_store = Gtk.ListStore(str, str)
        for dev, name, info in sorted(list_ports.comports()):
            dlg_log.debug("COM: %s %s", dev, info)
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

    def run(self, *args):
        self.dialog.show_all()
        return self.dialog.run(*args)

    def get_result_destroy(self):
        # TODO
        port = ''
        baudrate = 56700
        log_file = None
        log_name = ''

        self.dialog.destroy()
        return (port, baudrate, log_file, log_name)

    def on_conn_dialog_close(self, *args):
        dlg_log.debug("onCancel")
        self.dialog.destroy()

    def on_ok_button_clicked(self, *args):
        dlg_log.debug("onOk")
        self.dialog.response(Gtk.ResponseType.OK)


# -*- python -*-
# -*- coding: utf-8 -*-

import serial
import logging
from gi.repository import Gtk
from ui.builder import get_builder
from ui.conn_dlg import ConnDialog
from ui.param_item import ParamBoxRow
from ui.gauge_meter import GtkGauge
from ui.status_utils import pb_to_kv_pairs, status_str

from models import CommManager, ParamManager, StatusManager, StatusTextManager, \
    CommandManger, TimeRefManager
from comm import msgs, CommThread


class CCGuiApplication(object):
    def __init__(self):
        builder = get_builder('ccgui.ui')
        self.window = builder.get_object('ccgui_window')
        builder.connect_signals(self)

        self.rpm_gauge = GtkGauge()
        self.rpm_gauge.name = 'RPM\n<span foreground="orange"><i><small>x1000</small></i></span>'
        self.rpm_gauge.start_value = 0
        self.rpm_gauge.end_value = 10
        self.rpm_gauge.initial_step = 1
        self.rpm_gauge.sub_step = 0.2
        self.rpm_gauge.drawing_step = 1
        self.rpm_gauge.strip_color_order = 'GYR'
        self.rpm_gauge.green_strip_start = 0
        self.rpm_gauge.yellow_strip_start = 7
        self.rpm_gauge.red_strip_start = 8
        self.rpm_gauge.smooth_interval = 50

        self.temp_gauge = GtkGauge()
        self.temp_gauge.name = 'Temp\n<span foreground="orange"><i>°C</i></span>'
        self.temp_gauge.start_value = 0
        self.temp_gauge.end_value = 130
        self.temp_gauge.initial_step = 10
        self.temp_gauge.sub_step = 2.0
        self.temp_gauge.drawing_step = 20
        self.temp_gauge.strip_color_order = 'GYR'
        self.temp_gauge.green_strip_start = 0
        self.temp_gauge.yellow_strip_start = 100
        self.temp_gauge.red_strip_start = 110
        self.temp_gauge.smooth_interval = 50

        self.batt_gauge = GtkGauge()
        self.batt_gauge.name = 'Battery\n<span foreground="orange"><i>V</i></span>'
        self.batt_gauge.start_value = 0
        self.batt_gauge.end_value = 12
        self.batt_gauge.initial_step = 1
        self.batt_gauge.sub_step = 0.2
        self.batt_gauge.drawing_step = 2
        self.batt_gauge.strip_color_order = 'RYG'
        self.batt_gauge.green_strip_start = 5
        self.batt_gauge.yellow_strip_start = 4
        self.batt_gauge.red_strip_start = 0
        self.batt_gauge.smooth_interval = 50

        gbox = builder.get_object('gauge_box')
        gbox.pack_start(self.rpm_gauge, True, True, 0)
        gbox.pack_start(self.temp_gauge, True, True, 0)
        gbox.pack_start(self.batt_gauge, True, True, 0)

        # Status message tree view
        self.status_treeview = builder.get_object('status_treeview')
        self.status_store = Gtk.ListStore(str, str)
        self.status_treeview.set_model(self.status_store)
        renderer1 = Gtk.CellRendererText()
        renderer2 = Gtk.CellRendererText()
        column1 = Gtk.TreeViewColumn("Field", renderer1, text=0)
        column2 = Gtk.TreeViewColumn("Value", renderer2, text=1)
        self.status_treeview.append_column(column1)
        self.status_treeview.append_column(column2)

        self.window.set_default_size(640, 480)
        self.window.show_all()

        # store ref to some widgets
        self.param_listbox = builder.get_object('param_listbox')
        self.status_bar = builder.get_object('status_bar')
        self.ignition_switch = builder.get_object('ignition_switch')
        self.starter_switch = builder.get_object('starter_switch')

        # param widgets
        self.param_rows = {}

        # connect model signals
        ParamManager().sig_changed.connect(self.update_params)
        StatusManager().sig_changed.connect(self.update_status)
        StatusTextManager().sig_changed.connect(self.update_statustext)

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
        logging.info("DEV: closed")

    def create_comm(self, port, baudrate, engine_id, log_db, log_name):
        try:
            CommManager().clear()
            CommManager().register(CommThread(port, baudrate, engine_id, log_db, log_name))
            logging.info("DEV: %s: opened", port)
            TimeRefManager().start()
            ParamManager().retrieve_all()
        except serial.SerialException as ex:
            logging.error("DEV: %s: %s", port, repr(ex))

    def on_param_request_clicked(self, *args):
        logging.debug("onParamRequest")

        ParamManager().retrieve_all()

    def on_param_send_clicked(self, *args):
        logging.debug("onParamSend")

        ParamManager().sync()

    def on_param_load_clicked(self, *args):
        logging.debug("onParamLoad")

        if CommandManger().load_config():
            ParamManager().retrieve_all()

    def on_param_save_clicked(self, *args):
        logging.debug("onParamSave")

        ParamManager().sync()
        CommandManger().save_config()

    def on_ignition_switch_active_notify(self, switch, *args):
        logging.debug("onIgnitionActivate")

        if switch.get_active():
            CommandManger().ignition_enable()
        else:
            CommandManger().ignition_disable()

    def on_starter_switch_active_notify(self, switch, *args):
        logging.debug("onStarterActivete")

        if switch.get_active():
            CommandManger().starter_enable()
        else:
            CommandManger().starter_disable()

    def update_params(self, **kvargs):
        """Param update slot"""
        for k, p in sorted(ParamManager().parameters.iteritems()):
            row = self.param_rows.get(k)
            if row:
                row.update()
            else:
                row = ParamBoxRow(p)
                self.param_rows[k] = row
                self.param_listbox.add(row)

        # TODO: recalculate gauge min/max and strip position by
        # RPM_LIMIT, BATT_TYPE, BATT_CELLS, TEMP_OVERHEAT

        # remove old rows
        for k, row in self.param_rows.items():
            if not ParamManager().parameters.has_key(k):
                self.param_listbox.remove(row)
                del self.param_rows[k]

    def update_status(self, **kvargs):
        status = StatusManager().last_message
        # logging.debug(str(status))
        logging.debug("status timestamp_ms: %s", status.timestamp_ms)

        self.update_rpm_value(status.rpm)
        self.update_temp_value(status.temperature.engine1)
        self.update_batt_value(status.battery.voltage)

        def update_field(filed, val):
            s_val = status_str(filed, val)
            for it in self.status_store:
                if it[0] == filed:
                    it[1] = s_val
                    return

            self.status_store.append([filed, s_val])

        for filed, val in pb_to_kv_pairs(status):
            # logging.debug("f: %s = %s: %s", filed, str(val), status_str(filed, val))
            update_field(filed, val)
            # TODO: remove old fileds (but simple .clear() does SIGFAULT)

    def update_rpm_value(self, val):
        # RPM gauge is 1/1000 of val
        val /= 1000.0
        self.rpm_gauge.set_value(val)

    def update_temp_value(self, val):
        # TEMP gauge in C, val in mC
        val = val / 1000.0
        self.temp_gauge.set_value(val)

    def update_batt_value(self, val):
        # BATT gauge in V, val in mV
        val = val / 1000.0
        self.batt_gauge.set_value(val)

    def update_statustext(self, **kvargs):
        ctx_id = self.status_bar.get_context_id("ECU")
        t, msg = StatusTextManager().last_message
        self.status_bar.push(ctx_id, "%s: %s: %s" % (
            t, msgs.StatusText.Severity.Name(msg.severity), msg.text))

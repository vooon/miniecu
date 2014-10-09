# -*- python -*-

import logging
from gi.repository import Gtk
from models.param_def import ParamDef

log = logging.getLogger(__name__)


class ParamBoxRow(Gtk.ListBoxRow):
    def __init__(self, param):
        Gtk.ListBoxRow.__init__(self)
        self.set_border_width(10)

        log.debug("Construct param row for: %s", param)

        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=50)

        vbox.pack_start(hbox, True, True, 0)
        self.add(vbox)

        id_label = Gtk.Label()
        id_label.set_justify(Gtk.Justification.RIGHT)
        id_label.set_markup("<b>%s</b>" % param.param_id)
        id_label.props.halign = Gtk.Align.START

        desc_label = Gtk.Label()
        desc_label.set_justify(Gtk.Justification.RIGHT)
        desc_label.set_markup("<small>%s</small>" % ParamDef().desc(param.param_id))
        desc_label.props.halign = Gtk.Align.START

        enum = ParamDef().fixed_values(param.param_id)
        if enum is not None:
            if isinstance(param.value, basestring):
                store_type = str
            else:
                store_type = type(param.value)

            value_id = ''
            enum_store = Gtk.ListStore(str, store_type)
            for kv_pair in enum:
                enum_store.append(kv_pair)
                if kv_pair[1] == param.value:
                    value_id = kv_pair[0]

            value_widget = Gtk.ComboBox.new_with_model(enum_store)
            text_renderer = Gtk.CellRendererText()
            value_widget.pack_start(text_renderer, True)
            value_widget.add_attribute(text_renderer, 'text', 0)
            value_widget.set_id_column(0)
            value_widget.set_active_id(value_id)
            value_widget.connect('changed', self.on_enum_value)

        elif isinstance(param.value, bool):
            value_widget = Gtk.Switch()
            value_widget.set_active(param.value)
            value_widget.connect('notify::active', self.on_bool_value)

        elif isinstance(param.value, (int, float)):
            min_, max_ = ParamDef().minmax(param.param_id)
            value_widget = Gtk.SpinButton()
            value_widget.set_adjustment(Gtk.Adjustment(param.value, min_, max_, 1.0, 10.0, 0))
            value_widget.set_numeric(True)
            if isinstance(param.value, float):
                value_widget.set_digits(20)
            value_widget.connect('value-changed', self.on_numeric_value)

        else:
            # string
            value_widget = Gtk.Entry()
            value_widget.set_text(str(param.value))
            value_widget.connect('activate', self.on_text_value)

        hbox.pack_start(id_label, True, True, 0)
        hbox.pack_start(value_widget, False, True, 0)

        vbox.pack_start(desc_label, True, True, 0)

        self.show_all()

        # save refs
        self.param = param
        self.value_widget = value_widget

    def on_bool_value(self, switch, gparam):
        self._set_value(switch.get_active())

    def on_numeric_value(self, spin):
        if isinstance(self.param.value, float):
            self._set_value(spin.get_value())
        else:
            self._set_value(spin.get_value_as_int())

    def on_text_value(self, entry):
        self._set_value(entry.get_text())

    def on_enum_value(self, combo):
        it = combo.get_active_iter()
        if it is not None:
            model = combo.get_model()
            self._set_value(model[it][1])
        else:
            log.error("ComboBox has wrong iterator for param: %s", self.param.param_id)

    def _set_value(self, val):
        if self.param.validate(val):
            log.debug("Change value of: %s to %s", self.param.param_id, val)
            self.param.value = val
            # TODO: mark widget
        else:
            log.error("Param %s set value not valid", param.param_id)
            # TODO: mark widget

    def update(self):
        if isinstance(self.value_widget, Gtk.Switch):
            self.value_widget.set_active(self.param.value)
        elif isinstance(self.value_widget, Gtk.SpinButton):
            self.value_widget.set_value(self.param.value)
        elif isinstance(self.value_widget, Gtk.ComboBox):
            for kv_pair in self.value_widget.get_model():
                if kv_pair[1] == self.param.value:
                    self.value_widget.set_active_id(kv_pair[0])
                    break
        else:
            self.value_widget.set_text(str(self.param.value))

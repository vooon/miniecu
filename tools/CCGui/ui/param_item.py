# -*- python -*-

import logging
from gi.repository import Gtk

log = logging.getLogger(__name__)


class ParamBoxRow(Gtk.ListBoxRow):
    def __init__(self, param):
        Gtk.ListBoxRow.__init__(self)

        log.debug("Construct param row for: %s", param)

        grid = Gtk.Grid()
        self.add(grid)

        id_label = Gtk.Label(param.param_id)
        desc_label = Gtk.Label("some long description: TODO")

        # TODO: right widget for value
        value_widget = Gtk.Entry()
        value_widget.set_text(str(param.value))

        grid.attach(id_label, 0, 0, 1, 1)
        grid.attach(desc_label, 0, 1, 2, 1)
        grid.attach(value_widget, 1, 0, 1, 1)

        self.show_all()

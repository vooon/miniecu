#!/usr/bin/env python

import sys
from os import path

# add path to tools dir
MODULE_PATH = path.abspath(path.dirname(__file__))
TOOLS_PATH = path.dirname(MODULE_PATH)
if TOOLS_PATH not in sys.path:
    sys.path.append(TOOLS_PATH)


from comm import CommThread
from gi.repository import Gtk


class CCGuiApplication(object):
    def __init__(self):
        self.builder = Gtk.Builder()
        self.builder.add_from_file(path.join(MODULE_PATH, 'ui/ccgui.glade'))

        self.window = self.builder.get_object('ccgui_window')
        self.window.connect("delete-event", Gtk.main_quit)
        self.window.show_all()


def main():
    app = CCGuiApplication()
    Gtk.main()


if __name__ == '__main__':
    main()

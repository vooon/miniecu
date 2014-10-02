#!/usr/bin/env python

# add path to tools dir
import sys
if not '..' in sys.path:
    sys.path.append('..')


from os import path
from comm import CommThread
from gi.repository import Gtk


class CCGuiApplication(object):
    def __init__(self):
        self.builder = Gtk.Builder()
        self.builder.add_from_file('ui/ccgui.glade')

        self.window = self.builder.get_object('ccgui_window')
        self.window.connect("delete-event", Gtk.main_quit)
        self.window.show_all()


def main():
    app = CCGuiApplication()
    Gtk.main()


if __name__ == '__main__':
    main()

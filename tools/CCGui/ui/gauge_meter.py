# -*- python -*-

# Port to python of gtkgauge widget from Asctec Ground Statio (ROS GCS).
#
# Original author: Gautier Dumonteil <gautier.dumonteil@gmail.com>
# Copyright (C) 2010, CCNY Robotics Lab
# Licensed under GPLv3.

import logging
from gi.repository import GObject, Gtk
import cairo
import math

log = logging.getLogger(__name__)


class GtkGauge(Gtk.DrawingArea):
    """
    Gtk Gauge Widget

    This widget provide an easy to read gauge instrument. <br>
    The design is made to look like to a real gauge<br>
    flight instrument in order to be familiar to aircraft and<br>
    helicopter pilots. This widget is fully comfigurable and<br>
    useable for several gauge type (Battery Voltage, Current,<br>
    Velocity, ...).
    """

    # main properties
    name = GObject.property(type=str, default="...not set...")
    start_value = GObject.property(type=int, default=0)
    end_value = GObject.property(type=int, default=100)
    initial_step = GObject.property(type=int, default=10)
    sub_step = GObject.property(type=float, default=2.0)
    drawing_step = GObject.property(type=int, default=10)

    # drawing props
    # grayscale_color = GObject.property(type=bool, default=False)
    # radial_color = GObject.property(type=bool, default=True)
    strip_color_order = GObject.property(type=str, default='YOR')
    green_strip_start = GObject.property(type=int, default=0)
    yellow_strip_start = GObject.property(type=int, default=0)
    orange_strip_start = GObject.property(type=int, default=0)
    red_strip_start = GObject.property(type=int, default=0)

    # colors RGB
    bg_color_inv = (0.7, ) * 3
    bg_color_gauge = (0.05, ) * 3
    bg_color_bounderie = (0.1, ) * 3
    bg_radial_color_begin_gauge = (0.7, ) * 3
    bg_radial_color_begin_bounderie = (0.2, ) * 3

    def __init__(self):
        Gtk.DrawingArea.__init__(self)

        self._x = 0
        self._y = 0
        self._radius = 0
        self._value = 0

        self.static_surface = None
        self.dynamic_surface = None

        self.connect('draw', self.on_draw)
        log.debug("GtkGauge constructed")

    def draw_static_once(self):
        if self.static_surface is not None:
            return

        x = 0
        y = 0
        w = 100
        h = 100

        log.debug("draw_static_once: x: %s, y: %s, w: %s, h: %s", x, y, w, h)

        self.static_surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, w, h)
        cr_static = cairo.Context(self.static_surface)
        cr_static.rectangle(x, y, w, h)
        cr_static.clip()

        self.draw_static_base(cr_static)

    def draw_static_base(self, cr):
        log.debug("draw_static_base")

        # calculations from gtk_gauge_draw_base
        al = self.get_allocation()
        x = al.width / 2
        y = al.height / 2
        radius = min(x, y) - 5

        rec_x0 = x - radius
        rec_y0 = y - radius
        rec_width = radius * 2
        rec_height = radius * 2
        rec_aspect = 1.0
        rec_corner_radius = rec_height / 8.0
        rec_radius = rec_corner_radius / rec_aspect

        # gauge base
        cr.new_sub_path()
        cr.arc(rec_x0 + rec_width - rec_radius, rec_y0 + rec_radius,
               rec_radius, math.radians(-90), math.radians(0))
        cr.arc(rec_x0 + rec_width - rec_radius, rec_y0 + rec_height - rec_radius,
               rec_radius, math.radians(0), math.radians(90))
        cr.arc(rec_x0 + rec_radius, rec_y0 + rec_height - rec_radius,
               rec_radius, math.radians(90), math.radians(180))
        cr.arc(rec_x0 + rec_radius, rec_y0 + rec_radius,
               rec_radius, math.radians(180), math.radians(270))
        cr.close_path()

        # fake light reflection (drawen when
        # radial_color=True and grayscale_color=False)
        def make_reflection_pattern(x, y, radius):
            return cairo.RadialGradient(x - 0.392 * radius, y - 0.967 * radius, 0.167 * radius,
                                        x - 0.477 * radius, y - 0.967 * radius, 0.836 * radius)

        # reflection 1
        pat = make_reflection_pattern(x, y, radius)
        rgba0 = self.bg_radial_color_begin_bounderie + (1.0, )
        rgba1 = self.bg_color_bounderie + (1.0, )
        pat.add_color_stop_rgba(0, *rgba0)
        pat.add_color_stop_rgba(1, *rgba1)

        cr.set_source(pat)
        cr.fill_preserve()
        cr.stroke()

        # meter circle
        cr.arc(x, y, radius, 0, 2 * math.pi)
        cr.set_source_rgb(0., 0., 0.)
        cr.fill_preserve()
        cr.stroke()

        cr.arc(x, y, radius - 0.04 * radius, 0, 2 * math.pi)
        cr.set_source_rgb(0.6, 0.5, 0.5)
        cr.stroke()

        # inner ring
        cr.set_line_width(0.01 * radius)
        radius -= 0.1 * radius
        cr.arc(x, y, radius, 0, 2 * math.pi)

        # reflection 2
        pat = make_reflection_pattern(x, y, radius)
        rgba0 = self.bg_radial_color_begin_gauge + (1.0, )
        rgba1 = self.bg_color_gauge + (1.0, )
        pat.add_color_stop_rgba(0, *rgba0)
        pat.add_color_stop_rgba(1, *rgba1)

        cr.set_source(pat)
        cr.fill_preserve()
        cr.stroke()

        # clolor strips
        self._alpha = 0.0

        # helper functions for strip_order_XYZ()
        def draw_strip_arc():
            step_count = (self.end_value - self.start_value) / self.sub_step
            cr.set_line_width(0.12 * radius)
            cr.arc(x, y, radius - 0.06 * radius,
                   -5 * math.pi + i * ((5 * math.pi / 4) / step_count),
                   -5 * math.pi + (i + 1) * ((5 * math.pi / 4) / step_count))

        def draw_strip_additional_red_arc():
            step_count = (self.end_value - self.start_value) / self.sub_step
            cr.set_line_width(0.1 * radius)
            cr.set_source_rgba(1, 0, 0, 0.2)
            cr.arc(x, y, radius - 0.23 * radius,
                   -5 * math.pi + i * ((5 * math.pi / 4) / step_count),
                   -5 * math.pi + (i + 1) * ((5 * math.pi / 4) / step_count))

        # draw one sub_step
        def strip_order_YOR(i):
            pass

        def strip_order_GYR(i):
            alpha_step = abs(1 / ((self.yellow_strip_start - self.green_strip_start) / self.sub_step))
            step = i * self.sub_step

            if self.yellow_strip_start > step >= self.green_strip_start:
                # green
                self._alpha += alpha_step
                cr.set_source_rgba(0, 0.65, 0, self._alpha)
            elif self.red_strip_start > step >= self.yellow_strip_start:
                # yellow
                cr.set_source_rgb(1, 1, 0)
            elif step >= self.red_strip_start:
                # red
                draw_strip_additional_red_arc()
                cr.stroke()
                cr.set_source_rgb(1, 0, 0)

            draw_strip_arc()
            cr.stroke()

        def strip_order_ROY(i):
            pass

        def strip_order_RYG(i):
            pass

        STRIP_ORDERS = {
            'YOR': strip_order_YOR,
            'GYR': strip_order_GYR,
            'ROY': strip_order_ROY,
            'RYG': strip_order_RYG,
        }

        draw_strip = STRIP_ORDERS.get(self.strip_color_order, strip_order_YOR)
        for i in range(0, int((self.end_value - self.start_value) / self.sub_step)):
            cr.save()
            draw_strip(i)
            cr.restore()

    def on_draw(self, widget, cr):
        self.draw_static_base(cr)

    def set_value(self, value):
        log.debug("set_value: %s", value)


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)

    win = Gtk.Window()
    gauge = GtkGauge()
    win.add(gauge)

    win.set_title('GtkGauge test')
    win.resize(300, 300)
    win.connect('delete-event', Gtk.main_quit)

    gauge.strip_color_order = 'GYR'
    gauge.green_strip_start = 0
    gauge.yellow_strip_start = 50
    gauge.red_strip_start = 75

    win.show_all()
    Gtk.main()

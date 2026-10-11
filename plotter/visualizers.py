import math
import tkinter as tk
from tkinter import ttk
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
import numpy as np

from collections import deque

AVAILABLE_VIEWS = []

# Half-range of the alarm axis, in microseconds. Pinned rather than autoscaled so
# the trip level keeps a fixed visual meaning between frames: the differences
# cluster tightly around a few tens of us, and left to autoscale a 1.5 us
# excursion fills the whole plot and reads as enormous. The axis only ever grows,
# never shrinks, so a strong target stays visible instead of silently rescaling
# back down.
PLOT_Y_LIMIT_US = 5.0

def register_view(cls):
    AVAILABLE_VIEWS.append(cls)
    return cls

class BasePlotTab(ttk.Frame):
    name = "Base Plot"

    def __init__(self, parent):
        super().__init__(parent)
        self.figure, self.ax = plt.subplots()
        self.ax.set_title(self.name)
        self.ax.grid(True, alpha=0.3)
        
        self.canvas = FigureCanvasTkAgg(self.figure, master=self)
        self.canvas.draw()
        
        self.toolbar = NavigationToolbar2Tk(self.canvas, self)
        self.toolbar.update()
        
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

    def redraw(self):
        self.ax.relim()
        self.ax.autoscale_view()
        self.canvas.draw()

    def update_view(self, measurement):
        pass

@register_view
class EMATab(BasePlotTab):
    name = "Filtr EMA"

    def __init__(self, parent):
        super().__init__(parent)
        self.ax.set_xlabel("Próbka")
        self.ax.set_ylabel("Wartość")
        self.raw_line, = self.ax.plot([], [], '-o', color='#808080ff', label='Surowa wartość', linewidth=2)
        self.slow_line, = self.ax.plot([], [],'-o', color='#d67728', label='Wartość wolnego filtru', linewidth=2)
        self.fast_line, = self.ax.plot([], [], '-o',color='#d6FF28', label='Wartość szybkiego filtru', linewidth=2)
        self.ax.legend(loc='upper right')

        max_len = 100
        self.raw = deque(maxlen=max_len) 
        self.slow = deque(maxlen=max_len)
        self.fast = deque(maxlen=max_len)

    def update_view(self, measurement):
        self.raw.append(measurement.time)
        self.fast.append(measurement.fast)
        self.slow.append(measurement.slow)

        x_axis = range(len(self.raw))
        self.raw_line.set_data(x_axis, self.raw)
        self.fast_line.set_data(x_axis, self.fast)
        self.slow_line.set_data(x_axis, self.slow)
        self.redraw()

@register_view
class EMASplitTab(BasePlotTab):
    name = "Filtr EMA (Separowane)"

    def __init__(self, parent):
        super().__init__(parent)

        self.figure.clf()

        self.ax1 = self.figure.add_subplot(211) 
        self.ax2 = self.figure.add_subplot(212, sharex=self.ax1)

        self.ax1.set_title("Surowe dane")
        self.ax1.set_xlabel("Próbka")
        self.ax1.set_ylabel("Wartość")
        self.ax1.grid(True, alpha=0.3)
        self.raw_line, = self.ax1.plot([], [], '-o',color='#808080', label='Surowa', linewidth=1)

        self.ax2.set_title("Przefiltrowane dane")
        self.ax2.set_xlabel("Próbka")
        self.ax2.set_ylabel("Roznica [us]")
        self.ax2.grid(True, alpha=0.3)
        self.ema_line, = self.ax2.plot([], [], '-o',label='EMA', linewidth=2)

        self.raw = deque(maxlen=100) 
        self.filtered = deque(maxlen=100) 
        # Threshold is adaptive, so each sample has to be judged against the bar
        # that was in force when it arrived, not against whatever the latest one is.
        self.thresholds = deque(maxlen=100)
        self.threshold_line, = self.ax2.plot([], [], '--', color='darkred', linewidth=1, label='Próg')
        self.fill_collection = None
        self.vlines_collection = None
        self.y_limit = PLOT_Y_LIMIT_US
        self.ax2.set_ylim(-self.y_limit, self.y_limit)
        self.ax2.legend(loc='upper right')
        self.figure.tight_layout()

    def update_view(self, measurement):
        self.raw.append(measurement.time)
        self.filtered.append(measurement.difference)
        self.thresholds.append(measurement.threshold)

        arr_raw = np.array(self.raw)
        arr_filtered = np.array(self.filtered)
        arr_thresholds = np.array(self.thresholds)
        x_axis = np.arange(len(arr_raw))
        
        self.raw_line.set_data(x_axis, arr_raw)
        self.ema_line.set_data(x_axis, arr_filtered)

        # Both edges, because the detector compares the magnitude. A single line above
        # zero would leave the lower trip invisible on the axis that matters most.
        level = arr_thresholds[-1]
        self.threshold_line.set_data(
            [x_axis[0], x_axis[-1], np.nan, x_axis[0], x_axis[-1]],
            [level, level, np.nan, -level, -level],
        )
        
        if self.fill_collection:
            self.fill_collection.remove()
            self.fill_collection = None
        if self.vlines_collection:
            self.vlines_collection.remove()
            self.vlines_collection = None

        # >= matches the detector's own comparison, so the shading and the buzzer
        # agree on the boundary instead of differing by one exact hit.
        alarm_mask = np.abs(arr_filtered) >= arr_thresholds

        self.fill_collection = self.ax2.fill_between(
            x_axis,
            0, 1,
            where=alarm_mask,
            transform=self.ax2.get_xaxis_transform(),
            facecolor='red',
            alpha=0.2,
            interpolate=False
        )

        self.vlines_collection = self.ax2.vlines(
            x_axis[alarm_mask],
            ymin=0, ymax=1,
            transform=self.ax2.get_xaxis_transform(),
            colors='red',
            alpha=0.2,
            linewidth=1
        )
        self.redraw()

    def redraw(self):
        self.ax1.relim()
        self.ax1.autoscale_view()

        self.ax2.relim()
        self.ax2.autoscale_view()

        # Re-assert after autoscale: the alarm axis is pinned so the trip level
        # reads at a consistent height, and only widens when a real excursion
        # would otherwise fall off the top or bottom.
        if self.filtered:
            peak = float(np.max(np.abs(np.array(self.filtered))))
            if peak > self.y_limit:
                self.y_limit = peak * 1.1
        self.ax2.set_ylim(-self.y_limit, self.y_limit)
        self.ax2.set_ylabel(f"Roznica [us]")

        self.canvas.draw()


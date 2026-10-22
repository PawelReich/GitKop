import math
import tkinter as tk
from tkinter import ttk
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
import numpy as np
from collections import deque

AVAILABLE_VIEWS = []

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

    def update_view(self, values, special):
        pass

@register_view
class TimeDomainTab(BasePlotTab):
    name = "Buffer View"

    def __init__(self, parent):
        super().__init__(parent)
        self.ax.set_xlabel("Sample")
        self.ax.set_ylabel("ADC Value")
        
        self.line_ref, = self.ax.plot([], [], 'o-', color='#1f77b4', linewidth=1, markersize=3)

    def update_view(self, values, special):
        
        x_data = range(len(values))
        self.line_ref.set_data(x_data, values)
        
        self.redraw()

@register_view
class Slope(BasePlotTab):
    name = "Slope"

    def __init__(self, parent):
        super().__init__(parent)
        self.ax.set_xlabel("Sample")
        self.ax.set_ylabel("Slope (V/s)")
        
        self.line_ref, = self.ax.plot([], [], 'o-', color='#1f77b4', linewidth=1, markersize=3)
        self.buffer = deque(maxlen=100) 

    def update_view(self, values, special):
        self.buffer.append(special[3])
        x_data = range(len(self.buffer))
        self.line_ref.set_data(x_data, self.buffer)
        
        self.redraw()

@register_view
class EMATab(BasePlotTab):
    name = "EMA Filter"

    def __init__(self, parent):
        super().__init__(parent)
        self.ax.set_xlabel("Sample")
        self.ax.set_ylabel("Value")
        self.raw_line, = self.ax.plot([], [], '-o', color='#808080ff', label='Raw value', linewidth=2)
        self.slow_line, = self.ax.plot([], [],'-o', color='#d67728', label='Slow filter value', linewidth=2)
        self.fast_line, = self.ax.plot([], [], '-o',color='#d6FF28', label='Fast filter value', linewidth=2)
        self.ax.legend(loc='upper right')

        max_len = 100
        self.raw = deque(maxlen=max_len) 
        self.slow = deque(maxlen=max_len)
        self.fast = deque(maxlen=max_len)

    def update_view(self, values, special):
        self.raw.append(special[1])
        self.fast.append(special[4])
        self.slow.append(special[5])

        x_axis = range(len(self.raw))
        self.raw_line.set_data(x_axis, self.raw)
        self.fast_line.set_data(x_axis, self.fast)
        self.slow_line.set_data(x_axis, self.slow)
        self.redraw()

@register_view
class EMASplitTab(BasePlotTab):
    name = "EMA Filter (Separated)"

    def __init__(self, parent):
        super().__init__(parent)

        self.figure.clf()

        self.ax1 = self.figure.add_subplot(211) 
        self.ax2 = self.figure.add_subplot(212, sharex=self.ax1)

        self.ax1.set_title("Raw data")
        self.ax1.set_xlabel("Sample")
        self.ax1.set_ylabel("Value")
        self.ax1.grid(True, alpha=0.3)
        self.raw_line, = self.ax1.plot([], [], '-o',color='#808080', label='Raw', linewidth=1)

        self.ax2.set_title("Filtered data")
        self.ax2.set_xlabel("Sample")
        self.ax2.set_ylabel("Value")
        self.ax2.grid(True, alpha=0.3)
        self.ema_line, = self.ax2.plot([], [], '-o',label='EMA', linewidth=2)

        self.raw = deque(maxlen=100) 
        self.filtered = deque(maxlen=100) 
        self.fill_collection = None
        self.vlines_collection = None
        self.figure.tight_layout()

    def update_view(self, values, special):
        self.raw.append(special[1])
        self.filtered.append(special[2])

        arr_raw = np.array(self.raw)
        arr_filtered = np.array(self.filtered)
        x_axis = np.arange(len(arr_raw))
        
        self.raw_line.set_data(x_axis, arr_raw)
        self.ema_line.set_data(x_axis, arr_filtered)
        
        if self.fill_collection:
            self.fill_collection.remove()
            self.fill_collection = None
        if self.vlines_collection:
            self.vlines_collection.remove()
            self.vlines_collection = None

        alarm_mask = np.abs(self.filtered) > 1

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
        self.canvas.draw()


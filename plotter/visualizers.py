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
    name = "Widok bufora"

    def __init__(self, parent):
        super().__init__(parent)
        self.ax.set_xlabel("Próbka")
        self.ax.set_ylabel("Wartość ADC")
        
        self.line_ref, = self.ax.plot([], [], 'o-', color='#1f77b4', linewidth=1, markersize=3)

    def update_view(self, values, special):
        
        x_data = range(len(values))
        self.line_ref.set_data(x_data, values)
        
        self.redraw()

@register_view
class Slope(BasePlotTab):
    name = "Nachylenie"

    def __init__(self, parent):
        super().__init__(parent)
        self.ax.set_xlabel("Próbka")
        self.ax.set_ylabel("Nachylenie (V/s)")
        
        self.line_ref, = self.ax.plot([], [], 'o-', color='#1f77b4', linewidth=1, markersize=3)
        self.buffer = deque(maxlen=100) 

    def update_view(self, values, special):
        self.buffer.append(special[3])
        x_data = range(len(self.buffer))
        self.line_ref.set_data(x_data, self.buffer)
        
        self.redraw()

@register_view
class FFTTab(BasePlotTab):
    name = "FFT"

    def __init__(self, parent):
        super().__init__(parent)
        self.ax.set_xlabel("Frequency")
        self.ax.set_ylabel("Magnitude")
        self.line_ref, = self.ax.plot([], [], color='#ff7f0e', linewidth=1)
        self.fft_history_buffer = deque(maxlen=1000) 

    def update_view(self, values, special):
        smallest = special[1]

        self.fft_history_buffer.append(smallest)
        data_np = np.array(self.fft_history_buffer)
        
        data_np = data_np - np.mean(data_np) 
        
        windowing = np.hanning(len(data_np)) 
        data_np = data_np * windowing

        fft_vals = np.fft.rfft(data_np)
        fft_mag = np.abs(fft_vals)
        
        freqs = np.linspace(0, len(data_np)/2, len(fft_mag))
        
        self.line_ref.set_xdata(freqs)
        self.line_ref.set_ydata(fft_mag)
        self.redraw()

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
        self.ax2.set_ylabel("Wartość")
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


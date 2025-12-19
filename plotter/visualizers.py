import tkinter as tk
from tkinter import ttk
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
import numpy as np
from collections import deque

# --- 1. The Registry ---
# This list holds references to all view classes marked with @register_view
AVAILABLE_VIEWS = []

def register_view(cls):
    """Decorator to register a view class automatically."""
    AVAILABLE_VIEWS.append(cls)
    return cls

# --- 2. Base Class ---
class BasePlotTab(ttk.Frame):
    """
    Parent class to handle common Matplotlib embedding logic.
    Subclasses must implement update_view(self, context).
    """
    name = "Base Plot" # Override this in subclasses

    def __init__(self, parent):
        super().__init__(parent)
        self.figure, self.ax = plt.subplots()
        self.ax.set_title(self.name)
        self.ax.grid(True, alpha=0.3)
        
        # Embed Canvas
        self.canvas = FigureCanvasTkAgg(self.figure, master=self)
        self.canvas.draw()
        
        # Toolbar
        self.toolbar = NavigationToolbar2Tk(self.canvas, self)
        self.toolbar.update()
        
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

    def redraw(self):
        self.ax.relim()
        self.ax.autoscale_view()
        self.canvas.draw()

    def update_view(self, context):
        pass

# --- 3. The Views ---

@register_view
class TimeDomainTab(BasePlotTab):
    name = "Time Domain"

    def __init__(self, parent):
        super().__init__(parent)
        self.ax.set_xlabel("Sample Index")
        self.ax.set_ylabel("Raw ADC Value")
        
        self.line_ref, = self.ax.plot([], [], 'o-', color='#1f77b4', linewidth=1, markersize=3)
        self.area_ref  = self.ax.axvline(x=0, color='red', linestyle='--', label='Area')
        self.smallest_ref  = self.ax.axvline(x=0, color='green', linestyle=':', label='Smallest')
        self.ax.legend(loc='upper right')

    def update_view(self, context):
        values = context['values']
        special = context['special']
        
        x_data = range(len(values))
        self.line_ref.set_xdata(x_data)
        self.line_ref.set_ydata(values)
        
        self.area_ref.set_xdata([special[1]])
        self.smallest_ref.set_xdata([special[0]])
        self.redraw()

@register_view
class FFTTab(BasePlotTab):
    name = "FFT (History of Smallest)"

    def __init__(self, parent):
        super().__init__(parent)
        self.ax.set_xlabel("Frequency")
        self.ax.set_ylabel("Magnitude")
        self.line_ref, = self.ax.plot([], [], color='#ff7f0e', linewidth=1)
        self.fft_history_buffer = deque(maxlen=1000) 

    def update_view(self, context):
        
        values = context['values']
        smallest = values[int(context['special'][0])]

        self.fft_history_buffer.append(smallest)
        data_np = np.array(self.fft_history_buffer)
        
        # Remove DC Offset (center signal at 0)
        data_np = data_np - np.mean(data_np) 
        
        # Apply Hanning window to smooth spectral leakage
        windowing = np.hanning(len(data_np)) 
        data_np = data_np * windowing

        # Perform FFT
        fft_vals = np.fft.rfft(data_np)
        fft_mag = np.abs(fft_vals)
        
        # Create X-axis (Bins)
        freqs = np.linspace(0, len(data_np)/2, len(fft_mag))
        
        self.line_ref.set_xdata(freqs)
        self.line_ref.set_ydata(fft_mag)
        self.redraw()

@register_view
class EMATab(BasePlotTab):
    name = "EMA Filter (Alpha=0.1)"

    def __init__(self, parent):
        super().__init__(parent)
        self.ax.set_xlabel("History Index")
        self.ax.set_ylabel("Amplitude")
        self.raw_line, = self.ax.plot([], [], color='lightgray', label='Raw', linewidth=0)
        self.ema_line, = self.ax.plot([], [], color='#d62728', label='EMA', linewidth=2)
        self.ax.legend(loc='upper right')
        self.buffer = deque(maxlen=1000) 

    def update_view(self, context):
        val = context['values'][int(context['special'][0])]
        self.buffer.append(val)

        data = np.array(self.buffer)
        
        # Calculate Exponential Moving Average
        alpha = 0.2
        ema = [data[0]]
        for i in range(1, len(data)):
            val = (data[i] * alpha) + (ema[-1] * (1 - alpha))
            ema.append(val)

        x_axis = range(len(data))
        self.raw_line.set_xdata(x_axis)
        self.raw_line.set_ydata(data)
        self.ema_line.set_xdata(x_axis)
        self.ema_line.set_ydata(ema)
        self.redraw()


@register_view
class ProcessedTab(BasePlotTab):
    name = "Processed"

    def __init__(self, parent):
        super().__init__(parent)
        self.ax.set_xlabel("History Index")
        self.ax.set_ylabel("Amplitude")
        self.ema_line, = self.ax.plot([], [], color='#d62728', label='Value', linewidth=2)
        self.ax.legend(loc='upper right')
        self.buffer = deque(maxlen=1000) 

    def update_view(self, context):
        processed = context['values'][int(context['special'][0])]

        self.buffer.append(processed)

        data = np.array(self.buffer)

        x_axis = range(len(data))
        self.ema_line.set_xdata(x_axis)
        self.ema_line.set_ydata(data)
        self.redraw()

#!/usr/bin/python3
import queue
import time
import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox

from serial_manager import SerialManager
from visualizers import AVAILABLE_VIEWS
from detector import MetalDetector, Measurement, BUZZER_COMMAND

VIEW_INTERVAL = 0.08
GUI_TICK_MS = 20
MAX_DRAIN_PER_TICK = 500

READOUT_FIELDS = (
    ("Czas", "time", "{:.2f} us"),
    ("Szybka", "fast", "{:.2f} us"),
    ("Wolna", "slow", "{:.2f} us"),
    ("Roznica", "difference", "{:+.2f} us"),
    ("Prog", "threshold", "{:.2f} us"),
    ("Szum p99", "noise_quantile", "{:.2f} us"),
    ("Metal", "metal_detected", "{}"),
)

def _is_timestamp(line):
    """True for a line off the board's stream: one bare integer, in microseconds.

    The board frames nothing around its samples, so being a number is the whole
    test for telling a measurement apart from a banner or log line.
    """
    try:
        int(line)
        return True
    except ValueError:
        return False


class GitKopDebugger:
    def __init__(self, root):
        self.root = root
        self.root.title("GitKop Debugging Tool")
        self._closing = False
        self._after_id = None

        self.detector = MetalDetector()
        self.last_draw = 0.0
        self.serial_mgr = SerialManager(on_line=self.on_serial_line)

        self.active_views : list[ViewClass] = {}
        
        self._setup_top_controls()
        self._setup_readout()
        self._setup_main_area()

        self._after_id = self.root.after(50, self.process_queue)

    def _setup_top_controls(self):
        control_frame = ttk.LabelFrame(self.root, text="Konfiguracja", padding=5)
        control_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)

        ttk.Label(control_frame, text="Port:").pack(side=tk.LEFT, padx=5)
        self.port_combobox = ttk.Combobox(control_frame, width=20)
        self.port_combobox.pack(side=tk.LEFT, padx=5)
        
        ttk.Label(control_frame, text="Baud:").pack(side=tk.LEFT, padx=5)
        self.baud_combobox = ttk.Combobox(control_frame, values=["115200", "2000000", "9600", "921600"], width=10)
        self.baud_combobox.current(0)
        self.baud_combobox.pack(side=tk.LEFT, padx=5)

        self.btn_connect = ttk.Button(control_frame, text="Polacz", command=self.toggle_connection)
        self.btn_connect.pack(side=tk.LEFT, padx=10)

        self.refresh_ports()

    def _setup_readout(self):
        frame = ttk.LabelFrame(self.root, text="Pomiar", padding=5)
        frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)

        self.readout_vars = []
        for label, key, fmt in READOUT_FIELDS:
            ttk.Label(frame, text=f"{label}:").pack(side=tk.LEFT, padx=(14, 2))
            var = tk.StringVar(value="-")
            ttk.Label(frame, textvariable=var).pack(side=tk.LEFT)
            self.readout_vars.append((var, key, fmt))

    def _update_readout(self, measurement):
        values = {
            "time": measurement.time,
            "fast": measurement.fast,
            "slow": measurement.slow,
            "difference": measurement.difference,
            "threshold": measurement.threshold,
            "noise_quantile": self.detector.noise_quantile,
            "metal_detected": "TAK" if measurement.metal_detected else "nie",
        }
        for var, key, fmt in self.readout_vars:
            var.set(fmt.format(values[key]))

    def _setup_main_area(self):
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        for ViewClass in AVAILABLE_VIEWS:
            view_instance = ViewClass(self.notebook)
            self.notebook.add(view_instance, text=ViewClass.name)
            self.active_views[str(view_instance)] = view_instance

    def refresh_ports(self):
        ports = self.serial_mgr.list_ports()
        self.port_combobox['values'] = ports
        if ports: self.port_combobox.current(0)

    def toggle_connection(self):
        if not self.serial_mgr.is_connected:
            port = self.port_combobox.get()
            baud = self.baud_combobox.get()
            if not port: return
            
            success, msg = self.serial_mgr.connect(port, baud)
            if success:
                self.detector = MetalDetector()
                self.serial_mgr.send(BUZZER_COMMAND.format(hz=0))
                self.btn_connect.config(text="Rozlacz")
                self.log_message("INFO", msg)
            else:
                messagebox.showerror("Connection Error", msg)
        else:
            self.serial_mgr.send(BUZZER_COMMAND.format(hz=0))
            msg = self.serial_mgr.disconnect()
            self.btn_connect.config(text="Connect")
            self.log_message("INFO", msg)

    def process_queue(self):
        latest = None
        noise = []
        count = 0
        while not self.serial_mgr.data_queue.empty() and count < MAX_DRAIN_PER_TICK:
            item = self.serial_mgr.data_queue.get()
            count += 1
            if isinstance(item, Measurement):
                latest = item
            else:
                noise.append(item)

        for line in noise:
            if not _is_timestamp(line):
                self.log_message("INFO", line)

        if latest is not None:
            now = time.monotonic()
            if now - self.last_draw >= VIEW_INTERVAL:
                self.last_draw = now

                self._update_readout(latest)

                current_tab_id = self.notebook.select()

                if current_tab_id in self.active_views:
                    self.active_views[current_tab_id].update_view(latest)

        if not self._closing:
            self._after_id = self.root.after(GUI_TICK_MS, self.process_queue)

    def on_serial_line(self, line):
        if not _is_timestamp(line):
            return

        # The AWD exit time, in microseconds.
        now_mono = time.monotonic()
        measurement = self.detector.process(int(line))

        command = self.detector.command_for(measurement, now_mono)
        if command is not None:
            repeats = 2 if command.strip() == "0" else 1

            sent = True
            for _ in range(repeats):
                if not self.serial_mgr.send(command):
                    sent = False

            self.log_message("BUZZER", f"TX: {command.strip()}")

        try:
            self.serial_mgr.data_queue.put_nowait(measurement)
        except queue.Full:
            pass

    def log_message(self, tag, message):
        print(f"[{tag}] {message}")

if __name__ == "__main__":
    root = tk.Tk()
    app = GitKopDebugger(root)
    root.mainloop()

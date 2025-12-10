#!/usr/bin/python3
import tkinter as tk
from tkinter import ttk, scrolledtext, filedialog, messagebox
import serial
import serial.tools.list_ports
import threading
import queue
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
import csv
import time

class SerialPlotterApp:
    def __init__(self, root : tk.Tk):
        self.root = root
        self.root.title("GitKop Debugging Tool")
        self.root.geometry("1000x800")

        # --- Variables ---
        self.serial_port = None
        self.is_connected = False
        self.stop_event = threading.Event()
        self.data_queue = queue.Queue()
        self.current_plot_data = [] 
        
        # --- UI Setup ---
        self._setup_ui()

        # --- Initialize Plot Line ---
        self.line_ref, = self.ax.plot([], [], 'o', color='#1f77b4', linewidth=1)
        self.area_ref  = self.ax.axvline(x=0, color='red', linestyle='--', label='Area of Interest')
        self.smallest_ref  = self.ax.axvline(x=0, color='green', linestyle=':', label='Smallest Sample')
        
        self.root.after(50, self.process_queue)

        self.connect_serial("/dev/ttyACM0", 2000000)

    def _setup_ui(self):
        # 1. Top Control Panel
        control_frame = ttk.LabelFrame(self.root, text="Connection Settings", padding=5)
        control_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)

        ttk.Label(control_frame, text="Port:").pack(side=tk.LEFT, padx=5)
        self.port_combobox = ttk.Combobox(control_frame, width=20)
        self.port_combobox.pack(side=tk.LEFT, padx=5)
        self.refresh_ports()

        ttk.Button(control_frame, text="Refresh", command=self.refresh_ports).pack(side=tk.LEFT, padx=2)

        ttk.Label(control_frame, text="Baud:").pack(side=tk.LEFT, padx=5)
        self.baud_combobox = ttk.Combobox(control_frame, values=["2000000", "9600", "115200", "921600"], width=10)
        self.baud_combobox.current(0) 
        self.baud_combobox.pack(side=tk.LEFT, padx=5)

        self.btn_connect = ttk.Button(control_frame, text="Connect", command=self.toggle_connection)
        self.btn_connect.pack(side=tk.LEFT, padx=10)

        self.btn_export = ttk.Button(control_frame, text="Export CSV", command=self.export_data, state=tk.DISABLED)
        self.btn_export.pack(side=tk.RIGHT, padx=10)

        # 2. Main Area
        paned_window = ttk.PanedWindow(self.root, orient=tk.VERTICAL)
        paned_window.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # Plot Frame
        plot_frame = ttk.Frame(paned_window)
        paned_window.add(plot_frame, weight=3)

        self.fig, self.ax = plt.subplots()
        self.ax.set_title("Linear buffer view")
        self.ax.set_xlabel("Sample Index")
        self.ax.set_ylabel("Raw ADC Value")
        self.ax.grid(True, alpha=0.3)
        
        self.canvas = FigureCanvasTkAgg(self.fig, master=plot_frame)
        self.canvas.draw()
        
        toolbar = NavigationToolbar2Tk(self.canvas, plot_frame)
        toolbar.update()
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

        # Log Frame
        log_frame = ttk.LabelFrame(paned_window, text="Logs")
        paned_window.add(log_frame, weight=1)

        self.log_text = scrolledtext.ScrolledText(log_frame, state='disabled', height=10, font=("Consolas", 9))
        self.log_text.pack(fill=tk.BOTH, expand=True)
        self.log_text.tag_config('DATA', foreground='blue')
        self.log_text.tag_config('ERROR', foreground='red')

    def refresh_ports(self):
        ports = serial.tools.list_ports.comports()
        self.port_combobox['values'] = [port.device for port in ports]
        if self.port_combobox['values']:
            self.port_combobox.current(0)

    def toggle_connection(self):
        port = self.port_combobox.get()
        baud = self.baud_combobox.get()
        if not self.is_connected:
            self.connect_serial(port, baud)
        else:
            self.disconnect_serial()

    def connect_serial(self, port, baud):
        if not port: return

        try:
            # timeout=1 is critical so readline() doesn't block forever
            self.serial_port = serial.Serial(port, baudrate=int(baud), timeout=1)
            self.is_connected = True
            self.stop_event.clear()
            self.btn_connect.config(text="Disconnect")
            
            self.read_thread = threading.Thread(target=self.read_loop, daemon=True)
            self.read_thread.start()
            self.log_message("System", f"Connected to {port}")
        except Exception as e:
            messagebox.showerror("Error", str(e))

    def disconnect_serial(self):
        self.stop_event.set()
        self.is_connected = False
        if self.serial_port:
            try:
                self.serial_port.close()
            except: pass
        self.btn_connect.config(text="Connect")
        self.log_message("System", "Disconnected.")

    def read_loop(self):
        while not self.stop_event.is_set() and self.serial_port and self.serial_port.is_open:
            try:
                if self.serial_port.in_waiting:
                    # errors='replace' prevents crashing on corrupt bytes
                    line = self.serial_port.readline().decode('utf-8', errors='replace').strip()
                    if line:
                        self.data_queue.put(line)
                else:
                    time.sleep(0.01) # Important: Rest CPU if no data
            except Exception:
                break

    def process_queue(self):
        MAX_LINES_PER_UPDATE = 50 
        count = 0
        
        while not self.data_queue.empty() and count < MAX_LINES_PER_UPDATE:
            line = self.data_queue.get()
            self.parse_line(line)
            count += 1
            
        # Schedule next check
        self.root.after(20, self.process_queue)

    def parse_line(self, line):
        print(line)
        if line.startswith("DATA[") and line.endswith("$"):
            try:
                content = line[5:line.index("]")]
                values = [int(x) for x in content.split()]

                special_content = line[line.index("]")+2:-1].split()
                special_values = [int(x) for x in special_content]

                area_of_interest_idx = special_values[0]
                smallest_idx = special_values[1]
                self.update_plot(values, area_of_interest_idx, smallest_idx)
                self.log_message("DATA", f"Received Packet: {len(values)} samples")
                self.btn_export.config(state=tk.NORMAL)
            except ValueError as e:
                import traceback
                self.log_message("ERROR", f"{traceback.format_exc()}")
                pass
        else:
            # If line is extremely long, truncate it to avoid lag
            if len(line) > 500: line = line[:500] + "..."
            self.log_message("INFO", line)

    def log_message(self, tag, message):
        self.log_text.config(state='normal')
        self.log_text.insert(tk.END, message + "\n", tag)
        # Only scroll if we are near the bottom to allow user reading
        self.log_text.see(tk.END)
        self.log_text.config(state='disabled')

    def update_plot(self, data, area_of_interest_idx, smallest_idx):
        self.current_plot_data = data
        
        x_data = range(len(data))
        self.line_ref.set_xdata(x_data)
        self.line_ref.set_ydata(data)

        self.area_ref.set_xdata([area_of_interest_idx])
        self.smallest_ref.set_xdata([smallest_idx]);
        
        # Rescale axes to fit new data
        self.ax.relim()
        self.ax.autoscale_view()
        
        # Render
        self.canvas.draw()

    def export_data(self):
        if not self.current_plot_data: return
        path = filedialog.asksaveasfilename(defaultextension=".csv")
        if path:
            try:
                with open(path, 'w', newline='') as f:
                    writer = csv.writer(f)
                    writer.writerow(["Index", "Value"])
                    for i, val in enumerate(self.current_plot_data):
                        writer.writerow([i, val])
                messagebox.showinfo("Success", "Saved.")
            except Exception as e:
                messagebox.showerror("Error", str(e))

if __name__ == "__main__":
    root = tk.Tk()
    app = SerialPlotterApp(root)
    root.mainloop()

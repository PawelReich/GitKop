#!/usr/bin/python3
import tkinter as tk
from tkinter import ttk, scrolledtext, filedialog, messagebox
import csv
import traceback

from serial_manager import SerialManager
from visualizers import AVAILABLE_VIEWS

class GitKopDebugger:
    def __init__(self, root):
        self.root = root
        self.root.title("GitKop Debugging Tool")

        # --- Logic Modules ---
        self.serial_mgr = SerialManager()
        
        # Store latest packet for CSV export
        self.current_data = [] 

        # Dictionary to store active view instances: { "widget_id": ViewObject }
        self.active_views = {}

        # --- UI Setup ---
        self._setup_top_controls()
        self._setup_main_area()
        self._setup_log_area()

        # Start the processing loop
        self.root.after(50, self.process_queue)

    def _setup_top_controls(self):
        control_frame = ttk.LabelFrame(self.root, text="Connection Settings", padding=5)
        control_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)

        ttk.Label(control_frame, text="Port:").pack(side=tk.LEFT, padx=5)
        self.port_combobox = ttk.Combobox(control_frame, width=20)
        self.port_combobox.pack(side=tk.LEFT, padx=5)
        
        ttk.Button(control_frame, text="Refresh", command=self.refresh_ports).pack(side=tk.LEFT, padx=2)

        ttk.Label(control_frame, text="Baud:").pack(side=tk.LEFT, padx=5)
        self.baud_combobox = ttk.Combobox(control_frame, values=["2000000", "9600", "115200", "921600"], width=10)
        self.baud_combobox.current(0)
        self.baud_combobox.pack(side=tk.LEFT, padx=5)

        self.btn_connect = ttk.Button(control_frame, text="Connect", command=self.toggle_connection)
        self.btn_connect.pack(side=tk.LEFT, padx=10)

        self.btn_export = ttk.Button(control_frame, text="Export Packet CSV", command=self.export_csv, state=tk.DISABLED)
        self.btn_export.pack(side=tk.RIGHT, padx=10)
        
        self.refresh_ports()

    def _setup_main_area(self):
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        for ViewClass in AVAILABLE_VIEWS:
            view_instance = ViewClass(self.notebook)
            
            # Add to notebook tab
            self.notebook.add(view_instance, text=ViewClass.name)
            
            # Register in our dictionary so we can identify it later
            self.active_views[str(view_instance)] = view_instance

    def _setup_log_area(self):
        log_frame = ttk.LabelFrame(self.root, text="Logs", height=150)
        log_frame.pack(side=tk.BOTTOM, fill=tk.X, padx=5, pady=5)
        
        self.log_text = scrolledtext.ScrolledText(log_frame, state='disabled', height=8, font=("Consolas", 9))
        self.log_text.pack(fill=tk.BOTH, expand=True)
        self.log_text.tag_config('DATA', foreground='blue')
        self.log_text.tag_config('ERROR', foreground='red')
        self.log_text.tag_config('INFO', foreground='black')

    # --- Actions ---
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
                self.btn_connect.config(text="Disconnect")
                self.log_message("INFO", msg)
            else:
                messagebox.showerror("Connection Error", msg)
        else:
            msg = self.serial_mgr.disconnect()
            self.btn_connect.config(text="Connect")
            self.log_message("INFO", msg)

    def process_queue(self):
        # Process up to 50 lines to keep GUI responsive
        count = 0
        while not self.serial_mgr.data_queue.empty() and count < 50:
            line = self.serial_mgr.data_queue.get()
            self.parse_line(line)
            count += 1
        self.root.after(20, self.process_queue)

    def parse_line(self, line):
        # Expected Format: DATA[1 2 3 ...] [10 5] $
        if line.startswith("DATA[") and line.endswith("$"):
            try:
                # 1. Parse raw string
                main_end = line.index("]")
                values = [int(x) for x in line[5:main_end].split()]
                
                special_part = line[main_end+1:-1].replace('[', '').replace(']', '').split()
                special_vals = [float(x) for x in special_part]

                self.current_data = values
                self.btn_export.config(state=tk.NORMAL)

                context = {
                    'values': values,
                    'special': special_vals,
                }

                current_tab_id = self.notebook.select()
                
                if current_tab_id in self.active_views:
                    self.active_views[current_tab_id].update_view(context)

                self.log_message("DATA", f"Packet: {len(values)} samples")

            except Exception:
                self.log_message("ERROR", f"Parse error: {traceback.format_exc()}")
        else:
            if len(line) > 200: line = line[:200] + "..."
            self.log_message("INFO", line)

    def log_message(self, tag, message):
        self.log_text.config(state='normal')
        self.log_text.insert(tk.END, message + "\n", tag)
        self.log_text.see(tk.END)
        self.log_text.config(state='disabled')

    def export_csv(self):
        if not self.current_data: return
        path = filedialog.asksaveasfilename(defaultextension=".csv")
        if path:
            try:
                with open(path, 'w', newline='') as f:
                    writer = csv.writer(f)
                    writer.writerow(["Index", "Value"])
                    for i, val in enumerate(self.current_data):
                        writer.writerow([i, val])
                messagebox.showinfo("Success", "Saved CSV successfully.")
            except Exception as e:
                messagebox.showerror("Error", str(e))

if __name__ == "__main__":
    root = tk.Tk()
    app = GitKopDebugger(root)
    root.mainloop()

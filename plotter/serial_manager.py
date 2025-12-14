import serial
import serial.tools.list_ports
import threading
import queue
import time

class SerialManager:
    """
    Handles serial communication in a separate thread to avoid freezing the GUI.
    """
    def __init__(self):
        self.serial_port = None
        self.is_connected = False
        self.stop_event = threading.Event()
        self.data_queue = queue.Queue()
        self.read_thread = None

    def list_ports(self):
        devices = [port.device for port in serial.tools.list_ports.comports()]
        devices.reverse()
        return devices

    def connect(self, port, baud):
        if self.is_connected:
            self.disconnect()
            
        try:
            self.serial_port = serial.Serial(port, baudrate=int(baud), timeout=1)
            self.is_connected = True
            self.stop_event.clear()
            
            self.read_thread = threading.Thread(target=self._read_loop, daemon=True)
            self.read_thread.start()
            return True, f"Connected to {port}"
        except Exception as e:
            return False, str(e)

    def disconnect(self):
        self.stop_event.set()
        self.is_connected = False
        if self.serial_port:
            try:
                self.serial_port.close()
            except: 
                pass
        return "Disconnected"

    def _read_loop(self):
        """Internal loop running in background thread."""
        while not self.stop_event.is_set() and self.serial_port and self.serial_port.is_open:
            try:
                if self.serial_port.in_waiting:
                    # errors='replace' prevents crashing on corrupt bytes
                    line = self.serial_port.readline().decode('utf-8', errors='replace').strip()
                    if line:
                        self.data_queue.put(line)
                else:
                    time.sleep(0.005) # Yield CPU
            except Exception:
                break

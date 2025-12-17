import pyedflib
import numpy as np
import matplotlib.pyplot as plt
import os
from scipy.signal import savgol_filter  # Import the smoothing filter

FILE_LIST = [
    ['../meas/good.edf', 'Cewka tłumiona krytycznie'],
    ['../meas/undamped.edf', 'Cewka nietłumiona']
]

TRIGGER_CH_INDEX = 0
DATA_CH_INDEX = 1

TIME_LIMIT_SEC = 0.00002
REMOVE_OFFSET = True

# --- SMOOTHING CONFIGURATION ---
APPLY_SMOOTHING = True
SMOOTH_WINDOW = 51   # Must be odd (e.g., 21, 51, 101). Higher = smoother but less detail.
SMOOTH_POLY = 3      # Polynomial order (3 is usually good for scope traces)

# --- VISUAL CONFIGURATION ---
LINE_WIDTH = 1.0     # Default is often 1.5 or 2.0. 1.0 or 0.8 looks "lighter".

def find_falling_edge(signal, threshold=None):
    if threshold is None:
        threshold = (np.max(signal) + np.min(signal)) / 2
    
    below_thresh = signal < threshold
    transitions = np.diff(below_thresh.astype(int)) 
    falling_edges = np.where(transitions == 1)[0]
    
    return falling_edges[0] if len(falling_edges) > 0 else 0

def process_edf_files():
    # Use a cleaner style
    plt.style.use('seaborn-v0_8-whitegrid') 
    
    fig_together, ax_together = plt.subplots(figsize=(10, 6))
    ax_together.set_xlabel("Czas (us)")
    ax_together.set_ylabel("Amplituda (mV)")

    for i, file_data in enumerate(FILE_LIST):
        file_path = file_data[0]
        label_text = file_data[1]

        if not os.path.exists(file_path):
            print(f"File not found: {file_path}")
            continue

        f = pyedflib.EdfReader(file_path)
        trigger_signal = f.readSignal(TRIGGER_CH_INDEX)
        data_signal = f.readSignal(DATA_CH_INDEX)
        fs = f.getSampleFrequency(DATA_CH_INDEX)
        f.close()

        trigger_idx = find_falling_edge(trigger_signal)
        aligned_data = data_signal[trigger_idx:]

        if TIME_LIMIT_SEC:
            max_samples = int(TIME_LIMIT_SEC * fs)
            if len(aligned_data) > max_samples:
                aligned_data = aligned_data[:max_samples]

        if REMOVE_OFFSET and len(aligned_data) > 0:
            aligned_data = aligned_data - aligned_data[0]

        # --- APPLY SMOOTHING ---
        if APPLY_SMOOTHING and len(aligned_data) > SMOOTH_WINDOW:
            # savgol_filter(data, window_length, polyorder)
            aligned_data = savgol_filter(aligned_data, SMOOTH_WINDOW, SMOOTH_POLY)

        time_axis = np.linspace(0, len(aligned_data)/fs, len(aligned_data)) * 1000 * 1000
        
        # Plot with thinner linewidth and anti-aliasing
        ax_together.plot(time_axis, aligned_data, label=label_text, 
                         linewidth=LINE_WIDTH, antialiased=True)

    ax_together.grid(True, linewidth=0.5)
    
    # Improve Legend appearance
    ax_together.legend(frameon=True)
    
    plt.tight_layout()
    # plt.show()

    plt.savefig("meas.png")

if __name__ == "__main__":
    process_edf_files()

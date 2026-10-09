from dataclasses import dataclass, field

FAST_ALPHA = 0.1
SLOW_ALPHA = 0.0005
DETECTION_THRESHOLD = 5.0

ADC_REFERENCE_VOLTAGE = 3.3
ADC_MAX_CODE = 4095
SAMPLE_RATE_HZ = 4160000.0

STREAM_THROTTLE_DIV = 4
BUZZER_COMMAND = "{hz}\n"
BUZZ_MIN_ON_TIME = 0.15
BUZZ_MIN_OFF_TIME = 0.5
BUZZ_MIN_HZ = 3000
BUZZ_MAX_HZ = 6000
BUZZ_HZ_PER_US = 200
BUZZ_HZ_DEADBAND = 150
BUZZ_MIN_CMD_INTERVAL = 0.05
DETECTION_HYSTERESIS = 0.7


@dataclass
class Measurement:
    time: float = 0.0
    fast: float = 0.0
    slow: float = 0.0
    difference: float = 0.0
    metal_detected: bool = False
    buzzer_hz: int = 0


class EMA:
    def __init__(self, alpha):
        self.alpha = alpha
        self.out = None

    def update(self, value):
        if self.out is None:
            self.out = float(value)
        else:
            self.out = (self.alpha * value) + ((1.0 - self.alpha) * self.out)
        return self.out


class MetalDetector:
    """Filters the raw stream coming off the board and decides whether the coil
    is over metal. Replaces the filtering that used to live in GitKop.c."""

    def __init__(self, threshold=DETECTION_THRESHOLD, stream_throttle_div=STREAM_THROTTLE_DIV):
        self.threshold = threshold
        self.sensitivity = sensitivity
        self.max_threshold = max_threshold
        # The board only emits every Nth pulse, so the smoothing factors have to
        # be widened by the same decimation to keep the original time constants.
        self.fast_filter = EMA(1.0 - (1.0 - FAST_ALPHA) ** stream_throttle_div)
        self.slow_filter = EMA(1.0 - (1.0 - SLOW_ALPHA) ** stream_throttle_div)
        self._last_buzzer_hz = None

    def calculate_slope(self, samples):
        if len(samples) < 2:
            return 0.0
        delta_s = samples[-1] - samples[0]
        delta_v = delta_s * (ADC_REFERENCE_VOLTAGE / ADC_MAX_CODE)
        delta_time = (len(samples) - 1) / SAMPLE_RATE_HZ
        return delta_v / delta_time

    def process(self, samples, time):
        fast = self.fast_filter.update(time)
        slow = self.slow_filter.update(time)
        difference = fast - slow

        metal_detected = difference >= self.threshold
        buzzer_hz = int(difference) if metal_detected else 0

        return Measurement(
            time=time,
            fast=fast,
            slow=slow,
            difference=difference,
            metal_detected=metal_detected,
            buzzer_hz=buzzer_hz,
        )

    def command_for(self, measurement):
        """Buzzer command to send to the board, or None when it is unchanged."""
        if measurement.buzzer_hz == self._last_buzzer_hz:
            return None
        self._last_buzzer_hz = measurement.buzzer_hz
        return BUZZER_COMMAND.format(hz=measurement.buzzer_hz)
from collections import deque
from dataclasses import dataclass

FAST_ALPHA = 0.15
SLOW_ALPHA = 0.001

DETECTION_THRESHOLD = 1.5
SENSITIVITY = 0
THRESHOLD_QUANTILE = 0.99
MAX_THRESHOLD = 12.0
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
    threshold: float = DETECTION_THRESHOLD
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
    def __init__(self,
                 threshold=DETECTION_THRESHOLD,
                 sensitivity=SENSITIVITY,
                 max_threshold=MAX_THRESHOLD,
                 stream_throttle_div=STREAM_THROTTLE_DIV):
        self.threshold = threshold
        self.sensitivity = sensitivity
        self.max_threshold = max_threshold
        # The board only emits every Nth pulse, so the smoothing factors have to
        # be widened by the same decimation to keep the original time constants.
        self.fast_filter = EMA(1.0 - (1.0 - FAST_ALPHA) ** stream_throttle_div)
        self.slow_filter = EMA(1.0 - (1.0 - SLOW_ALPHA) ** stream_throttle_div)
        self._last_buzzer_hz = None
        self._last_cmd_time = -1.0
        self._last_metal_detected = False
        self._prev_metal_detected = False
        self._buzzer_on = False
        self._buzzer_on_time = 0.0
        self._buzzer_off_time = -1.0  # Allow immediate first trigger
        self._residuals = deque(maxlen=256)

    @property
    def noise_quantile(self):
        if len(self._residuals) < 32:
            return 0.0

        ordered = sorted(abs(d) for d in self._residuals)
        index = min(int(len(ordered) * THRESHOLD_QUANTILE), len(ordered) - 1)
        return ordered[index]

    def _effective_threshold(self, difference):
        quantile = self.noise_quantile
        if quantile == 0.0:
            return self.threshold

        adaptive = self.sensitivity * quantile

        return min(self.max_threshold, max(self.threshold, adaptive))

    def process(self, time):
        fast = self.fast_filter.update(time)
        slow = self.slow_filter.update(time)
        difference = fast - slow

        signal = abs(difference)

        threshold = self._effective_threshold(difference)

        if self._last_metal_detected:
            effective_threshold = threshold * DETECTION_HYSTERESIS
        else:
            effective_threshold = threshold

        metal_detected = signal >= effective_threshold
        buzzer_hz = self._buzzer_hz(signal) if metal_detected else 0

        if not metal_detected:
            self._residuals.append(difference)

        self._prev_metal_detected = self._last_metal_detected
        self._last_metal_detected = metal_detected

        return Measurement(
            time=time,
            fast=fast,
            slow=slow,
            difference=difference,
            threshold=threshold,
            metal_detected=metal_detected,
            buzzer_hz=buzzer_hz,
        )

    @staticmethod
    def _buzzer_hz(signal):
        hz = int(BUZZ_MIN_HZ + signal * BUZZ_HZ_PER_US)
        return max(BUZZ_MIN_HZ, min(BUZZ_MAX_HZ, hz))

    def reset_noise_estimate(self):
        self._residuals.clear()

    def command_for(self, measurement, now_mono):
        wants_on = measurement.buzzer_hz > 0
        now = now_mono

        if self._buzzer_on:
            if wants_on:
                if abs(measurement.buzzer_hz - self._last_buzzer_hz) < BUZZ_HZ_DEADBAND:
                    return None
            else:
                if (now - self._buzzer_on_time) < BUZZ_MIN_ON_TIME:
                    return None
        else:
            if not wants_on:
                return None
            else:
                if (now - self._buzzer_off_time) < BUZZ_MIN_OFF_TIME:
                    return None

        if wants_on and (now - self._last_cmd_time) < BUZZ_MIN_CMD_INTERVAL:
            return None

        self._last_cmd_time = now
        self._last_buzzer_hz = measurement.buzzer_hz
        if wants_on and not self._buzzer_on:
            self._buzzer_on = True
            self._buzzer_on_time = now
        elif not wants_on and self._buzzer_on:
            self._buzzer_on = False
            self._buzzer_off_time = now

        return BUZZER_COMMAND.format(hz=measurement.buzzer_hz)

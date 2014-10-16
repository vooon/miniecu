
import math


class LowPassFilter2p:
    def __init__(self):
        self._cutoff_freq = 0.0
        self._a1 = self._a2 = 0.0
        self._b0 = self._b1 = self._b2 = 0.0
        self._delay_element_1 = 0.0
        self._delay_element_2 = 0.0

    def set_cutoff_freq(self, sample_freq, cutoff_freq):
        self._cutoff_freq = cutoff_freq
        if self._cutoff_freq <= 0.0:
            return

        fr = sample_freq / self._cutoff_freq
        ohm = math.tan(math.pi / fr)
        c = 1.0 + 2.0 * math.cos(math.pi / 4) * ohm + ohm * ohm
        self._b0 = ohm * ohm / c
        self._b1 = 2.0 * self._b0
        self._b2 = self._b0
        self._a1 = 2.0 * (ohm * ohm - 1.0) / c
        self._a2 = (1.0 - 2.0 * math.cos(math.pi / 4) * ohm + ohm * ohm) / c

    def apply(self, sample):
        if self._cutoff_freq <= 0.0:
            return sample

        delay_element_0 = sample \
            - self._delay_element_1 * self._a1 \
            - self._delay_element_2 * self._a2

        output = delay_element_0 * self._b0 \
            + self._delay_element_1 * self._b1 \
            + self._delay_element_2 * self._b2

        self._delay_element_2 = self._delay_element_1
        self._delay_element_1 = delay_element_0

        return output

    def reset(self, sample):
        self._delay_element_1 = self._delay_element_2 = sample
        return self.apply(sample)

import re
import numpy as np
from scipy.signal import resample_poly
from scipy.io import wavfile
from statistics import mean
from collections import Counter

def parse_log_file(filename="output.txt"):
    pwm_lookup = {}
    pwms_per_loop = []
    latest_loop = None
    pwm_vals = []

    # regex patterns
    pwm_pattern = re.compile(r"PWM\s+(-?\d+):0x([0-9a-fA-F]+)")
    port_pattern = re.compile(r"port:\s+(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)")
    loop_pattern = re.compile(r"loop:\s+(\d+)")

    current_loop = None
    loop_count = 0
    max_pwm_magnitude = 0

    with open(filename, "r") as f:
        for line in f:
            line = line.strip()
            if not line or "Started" in line or "Completed" in line:
                continue

            # Parse PWM table
            m_pwm = pwm_pattern.match(line)
            if m_pwm:
                idx = int(m_pwm.group(1))
                hexval = int(m_pwm.group(2), 16)
                pwm_lookup[hexval] = idx
                max_pwm_magnitude = abs(idx) if abs(idx) > max_pwm_magnitude else max_pwm_magnitude 
                continue

            # Track loop number
            m_loop = loop_pattern.match(line)
            if m_loop:
                current_loop = int(m_loop.group(1))
                latest_loop = current_loop
                if loop_count > 0:
                    pwms_per_loop.append(loop_count)
                loop_count = 0
                continue

            # Parse port values
            m_port = port_pattern.match(line)
            if m_port and current_loop is not None:
                left = int(m_port.group(1), 16)
                right = int(m_port.group(2), 16)
                pair = (pwm_lookup.get(left, None), pwm_lookup.get(right, None))
                if any(x is not None for x in pair):
                    pwm_vals.append(pair)
                loop_count += 1

    return pwm_lookup, pwm_vals, pwms_per_loop, max_pwm_magnitude, latest_loop


if __name__ == "__main__":
    pwm_table, pwm_vals, pwms_per_loop, max_pwm_magnitude, latest_loop = parse_log_file("output.txt")

    print("PWM Table (hex→index):")
    for k, v in pwm_table.items():
        print(f"  {hex(k)} → {v}")

    print("\nPWM outputs:")
    pwm_array = np.array(pwm_vals, dtype=float)
    pwm_array /= max_pwm_magnitude # scale to +-1
    print(pwm_array.shape)
    # print(pwm_array)

    # Resample from 1.5 MHz -> 48 kHz (L=4, M=125)
    y_48k = resample_poly(pwm_array, up=4, down=125, axis=0)
    print("48kHz output shape:", y_48k.shape)
    print(y_48k)
    wavfile.write("output_48k.wav", 48000, np.int16(y_48k * 32767))

    # Resample to 96 kHz
    y_96k = resample_poly(pwm_array, up=8, down=125, axis=0)
    print("96kHz output shape:", y_96k.shape)

    # Resample to 192 kHz
    y_192k = resample_poly(pwm_array, up=16, down=125, axis=0)
    print("192kHz output shape:", y_192k.shape)

    print(f'pwms_per_loop ave:{mean(pwms_per_loop)} min:{min(pwms_per_loop)} max:{max(pwms_per_loop)}')
    histogram = Counter(pwms_per_loop)
    print(f'pwms_per_loop histogram: {histogram}')

    print(f"Latest loop number: {latest_loop}")

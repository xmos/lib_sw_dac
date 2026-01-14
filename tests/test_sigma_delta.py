# Copyright 2025-2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import re
import pytest
from pathlib import Path
from filelock import FileLock
import subprocess
import shutil
import numpy as np
from scipy.signal import resample_poly
from scipy.io import wavfile
from helpers import THDN_and_freq, create_if_needed
from scipy.signal import butter, sosfilt, sosfiltfilt # Simulate the filter on xms0021
from scipy.signal import firwin, freqz, lfilter # Brickwall filter


max_pcm = 32767
pwm_rate = 1500000
filter_cutoff = 48000
pwm_idle_words = [0x0ff0, 0xfffffffffffff00f, 0xfffffffff00ff00f, 0x0ff00ff0]

def parse_output(stdout):
    pwm_lookup = {}

    # regex patterns
    pwm_pattern = re.compile(r"PWM\s+(-?\d+):0x([0-9a-fA-F]+)")

    current_loop = None
    loop_count = 0
    max_pwm_magnitude = 0
    timedout = 0

    for line in stdout:
        line = line.strip()
        if not line or "Started" in line or "Completed" in line:
            continue
        if "Timeout" in line:
            timedout = int(line.split(":")[1])

        # Parse PWM table
        m_pwm = pwm_pattern.match(line)
        if m_pwm:
            idx = int(m_pwm.group(1))
            hexval = int(m_pwm.group(2), 16)
            pwm_lookup[hexval] = idx
            max_pwm_magnitude = abs(idx) if abs(idx) > max_pwm_magnitude else max_pwm_magnitude 
    # handle case where we have even number of PWM entries - need offset due to no 0 word
    if len(pwm_lookup) % 2 == 0:
        for pwm_val in pwm_lookup.keys():
            pwm_lookup[pwm_val] = pwm_lookup[pwm_val] + 0.5

    # Add idle words
    for pwd_idle_word in pwm_idle_words:
        pwm_lookup[pwd_idle_word] = 0

    return pwm_lookup, max_pwm_magnitude, timedout

def parse_xscope(filepath, pwm_lookup):
    text = open(filepath, "r").read()

    # match "b<binary> <var>"
    matches = re.findall(r"b([01]+)\s+(\d+)", text)

    pairs = []
    sample_num = 0
    # xscope IDs
    left_pwm_idx = 0
    right_pwm_idx = 1
    
    for bits, var in matches:
        pwm = int(bits, 2) # binary
        var = int(var, 2)
        if not pwm in list(pwm_lookup.keys()):
            print(f"Unrecognised PWM value: {hex(pwm)} {pwm} at sample: {sample_num} in channel: {var}, lookup: {list(pwm_lookup.keys())}")
            if var == right_pwm_idx:
                sample_num += 1
            continue

        if var == left_pwm_idx:
            left = pwm_lookup[pwm]
            # if pwm in pwm_idle_words:
            #     print(f"Idle at {sample_num}")
        if var == right_pwm_idx:
            right = pwm_lookup[pwm]
            pairs.append((left, right))
            sample_num += 1

    return np.array(pairs, dtype=np.float64)


def check_hw_presence():
    run_cmd = f'xrun -l'
    stdout = subprocess.check_output(run_cmd, shell = True)
    run_output = stdout.decode("utf-8")

    return True if "No Available Devices Found" in run_output else False

def run_on_sim(binary, xscope_file, burn, num_loops, pause_at, exit_at):
    run_cmd = f'xsim --xscope "-offline {xscope_file}" --args {binary} {burn} {num_loops} {pause_at} {exit_at}'
    print("Running cmd: ", run_cmd)
    stdout = subprocess.check_output(run_cmd, shell = True)

    return stdout.decode("utf-8").splitlines()

def run_on_hw(binary, xscope_file, burn, num_loops, pause_at, exit_at):
    # Ensure we don't spin up two HW instances at the same time
    with FileLock("xrun.lock"):
        run_cmd = f'xrun --id 0 --xscope-file {xscope_file} --args {binary} {burn} {num_loops} {pause_at} {exit_at}'
        print("Running cmd: ", run_cmd)
        stdout = subprocess.check_output(run_cmd, shell = True)

        return stdout.decode("utf-8").splitlines()

"""
This test runs just the sigma delta (and PWM) thread. It feeds in a 1.5MHz sampled sinewave
of 1kHz and then captures the outputs to the ports over a channel.
These are then converted to a time series of DC voltage levels and filtered and optionally
resampled to something a bit more manageable than 1.5MHz.
The output is then fed into the THDN script to see if it is working.
NOTE - this isn't really a true performance test, more a regression test to make sure the
SD modulator and PWM aren't broken. The THDN script is useful but doesn't seem to produce
the expected THDN values of -90 or so unless we run for a long time on HW and downsample to 48k.
The test automatically detects if there is an available target or not and uses xsim or xrun.
"""
@pytest.mark.parametrize("burn", [0, 1])
@pytest.mark.timeout(60 * 6)
def test_sigma_delta(request, burn):
    test_name = "test_sigma_delta"

    build = "CHAN"
    cwd = Path(request.fspath).parent
    binary = Path(f'{cwd}/{test_name}/bin/{build}/{test_name}_{build}.xe')
    assert Path(binary).exists(), f"Cannot find {binary}"
    tmp_binary = Path(f'{cwd}/{test_name}/bin/{test_name}_{burn}.xe') # Needed for xdist
    create_if_needed("logs")
    with FileLock("file_copy.lock"):
        shutil.copy2(binary, tmp_binary)

    # Check to see if we have HW
    using_simuator = check_hw_presence()
    print(f"Using simulator: {using_simuator}")

    # About 2 mins on xsim and about 15s on xrun
    num_loops = 10000 if using_simuator else 1000000
    xscope_file = Path(f"logs/{test_name}_trace_{burn}_{num_loops}.vcd")

    if using_simuator:
        run_output = run_on_sim(tmp_binary, xscope_file, burn, num_loops, num_loops, num_loops) #pause_at/exit_at == num_loops so no pause
    else:
        run_output = run_on_hw(tmp_binary, xscope_file, burn, num_loops, num_loops, num_loops) #pause_at/exit_at == num_loops so no pause

    tmp_binary.unlink() # delete

    print("Parsing terminal output...")
    pwm_lookup, max_pwm_magnitude, timedout = parse_output(run_output)
    print("PWM Table (hex→index):")
    for k, v in pwm_lookup.items():
        print(f"  {hex(k)} → {v}")

    print(f"SD thread timed out: {timedout}")

    print("Parsing XSCOPE output...")
    pwm_vals = parse_xscope(xscope_file, pwm_lookup)

    pwm_array = np.array(pwm_vals, dtype=float)
    pwm_array /= max_pwm_magnitude # scale to +-1
    print(f"\nPWM output array shape: {pwm_array.shape}")



    # Filter like the one on demo board
    sos = butter(N=2, Wn=filter_cutoff, fs=pwm_rate, output='sos')
    # Brickwall filter
    brickwall = firwin(2001, filter_cutoff, fs=pwm_rate, window="hamming", pass_zero="lowpass")


    # Test pass/fail
    THDN_limits_xrun = {48000:-57, 96000:-47, 192000:-47} # xscope cannot keep up on HW at 1.5MHz so we get timeouts
                                                          # Hence target levels are low. We actually see a 833Hz sine
    THDN_limits_sim = {48000:-40, 96000:-37, 192000:-34} # We have less signal to measure so lower vals

    for sample_rate in THDN_limits_sim.keys():
        print(f"Resampling to: {sample_rate}")
        # Resample ratios
        down = 125
        up = sample_rate / (pwm_rate / down)
        for channel in [0, 1]:
            one_channel = pwm_array[:, channel]
            # filtered = sosfilt(sos, one_channel) # like xms0021 HW - second order linear-phase-ish
            filtered = lfilter(brickwall, 1.0, one_channel) # brickwall

            if using_simuator:
                skip = 40
            else:
                skip = int(0.001 * pwm_rate) # There is a delay on the filter so skip the first millisecond

            THDN, freq = THDN_and_freq(filtered[skip:], pwm_rate)
            print(f"Filtered channel {channel} THDN: {THDN} freq: {freq}")
            downsampled = resample_poly(filtered, up=up, down=down, axis=0)
            THDN, freq = THDN_and_freq(downsampled[skip:], sample_rate)
            print(f"Downsampled channel {channel} THDN: {THDN} freq: {freq}")

            if using_simuator:
                target_THDN = THDN_limits_sim[sample_rate]
            else:
                target_THDN = THDN_limits_xrun[sample_rate]

            assert THDN < target_THDN, f"Failed THDN at sample rate {sample_rate}, target: {target_THDN} actual: {THDN}"

        wave_name = Path(f"logs/{test_name}_{sample_rate}_{burn}_{num_loops}.wav")
        wavfile.write(wave_name, sample_rate, np.int16(downsampled * max_pcm))

        # xscope can't keep up on HW at 1.5MHz so we skip the timeout check
        if using_simuator:
            assert timedout == 0

        print()


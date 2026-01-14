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
from helpers import create_if_needed
from scipy.signal import butter, sosfilt, sosfiltfilt # Simulate the filter on xms0021
from scipy.signal import firwin, freqz, lfilter # Brickwall filter
from test_sigma_delta import parse_output, parse_xscope, run_on_sim

max_pcm = 32767
pwm_rate = 1500000
filter_cutoff = 48000

def analyze_signal(signal, fs):
    # --- RMS ---
    rms = np.sqrt(np.mean(signal**2))

    # --- Mean ---
    mean_val = np.mean(signal)

    # --- Approximate frequency (FFT method) ---
    fft_vals = np.fft.fft(signal)
    fft_freqs = np.fft.fftfreq(len(signal), 1/fs)

    pos_mask = fft_freqs > 0
    fft_vals = np.abs(fft_vals[pos_mask])
    fft_freqs = fft_freqs[pos_mask]
    fft_freq = fft_freqs[np.argmax(fft_vals)]

    return rms, mean_val, fft_freq

"""
Test to check whether we can recover from not feeding the SDM/PWM for a while.
Only works on sim because on HW, xscope_lossless sometimes backs up and breaks timing upstream.
This test runs just the sigma delta (and PWM) thread. It feeds in a 1.5MHz sampled sinewave
of 1kHz and then captures the outputs to the ports over a channel.
These are then converted to a time series of DC voltage levels and filtered and optionally
resampled to something a bit more manageable than 1.5MHz.
We check the output against RMS, freq and mean (DC level)
"""
@pytest.mark.parametrize("burn", [0, 1])
@pytest.mark.timeout(60 * 6)
def test_sigma_delta_idle(request, burn):
    dut_test_name = "test_sigma_delta" # Re-use same DUT app
    test_name = "test_sigma_delta_idle" # But rename it so we don't clash with other tests using xdist

    cwd = Path(request.fspath).parent
    binary = Path(f'{cwd}/{dut_test_name}/bin/CHAN/{dut_test_name}_CHAN.xe')
    assert Path(binary).exists(), f"Cannot find {binary}"
    tmp_binary = Path(f'{cwd}/{dut_test_name}/bin/{test_name}_{burn}.xe') # Needed for xdist
    create_if_needed("logs")
    with FileLock("file_copy.lock"):
        shutil.copy2(binary, tmp_binary)


    # About 2min on xsim
    num_loops = 10000
    pause_at = num_loops // 2
    xscope_file = Path(f"logs/{test_name}_trace_{burn}_{num_loops}.vcd")
    run_output = run_on_sim(tmp_binary, xscope_file, burn, num_loops, pause_at, num_loops)

    tmp_binary.unlink() # delete

    print("Parsing terminal output...")
    pwm_lookup, max_pwm_magnitude, timedout = parse_output(run_output)
    print("PWM Table (hex→index):")
    for k, v in pwm_lookup.items():
        print(f"  {hex(k)} → {v}")
    print(f"Timed out: {timedout}")


    print("Parsing XSCOPE output...")
    pwm_vals = parse_xscope(xscope_file, pwm_lookup)

    pwm_array = np.array(pwm_vals, dtype=float)
    pwm_array /= max_pwm_magnitude # scale to +-1
    print(f"\nPWM output array shape: {pwm_array.shape}")


    # Filter like the one on demo board
    sos = butter(N=2, Wn=filter_cutoff, fs=pwm_rate, output='sos')
    # Brickwall filter
    brickwall = firwin(2001, filter_cutoff, fs=pwm_rate, window="hamming", pass_zero="lowpass")

    # Doesn't matter which we choose, as it's idle + recovery we are checking for
    output_sample_rate = 48000

    # Resample ratios
    down = 125
    up = output_sample_rate / (pwm_rate / down)
    for channel in [0, 1]:
        print()
        one_channel = pwm_array[:, channel]
        # filtered = sosfilt(sos, one_channel) # like xms0021 HW - second order linear-phase-ish
        filtered = lfilter(brickwall, 1.0, one_channel) # brickwall

        skip = 40 # There is a delay on the filter so skip the first few
        downsampled = resample_poly(filtered, up=up, down=down, axis=0)
        len_samples = downsampled.shape[0]
        print(f"Resampling to: {output_sample_rate}, len_samples: {len_samples}")

        wave_name = Path(f"logs/{test_name}_{output_sample_rate}_{burn}_{num_loops}.wav")
        wavfile.write(wave_name, output_sample_rate, np.int16(downsampled * max_pcm))

        section_num = 1
        # These are tuned by looking at the output wav and measuring the signal, silence, signal sections
        sections = [[0, int(len_samples*0.40)],
                    [int(len_samples*0.45), int(len_samples*0.55)], # 10 ms idle section
                    [int(len_samples*0.60), len_samples]]
        expected = [[0.32, 0.0, 1000], # RMS, Mean, freq
                    [0.0, 0.0, None],
                    [0.32, 0.0, 1000]]

        for section, expected in zip(sections, expected):
            start, end = section
            rms, mean, freq = analyze_signal(downsampled[start:end], output_sample_rate)
            exp_rms, exp_mean, exp_freq  = expected 
            print(f"section {section_num} {section}, rms: {rms:.2f} ({exp_rms:.2f}), mean: {mean:.2f} ({exp_mean:.2f}), freq: {freq:.2f} ({exp_freq})")
            
            # Note because of short run, we have to be quite lax on RTOL
            assert np.isclose(exp_rms, rms, atol = 0.01), f"Expected RMS: {exp_rms} Actual RMS: {rms} - see {wave_name}"
            assert np.isclose(exp_mean, mean, atol = 0.01), f"Expected RMS: {exp_mean} Actual RMS: {mean} - see {wave_name}"
            if exp_freq is not None:
                assert np.isclose(exp_freq, freq, rtol = 0.05), f"Expected RMS: {exp_freq} Actual RMS: {freq} - see {wave_name}"

            section_num += 1

    assert timedout == 1





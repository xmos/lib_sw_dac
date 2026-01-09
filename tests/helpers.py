# Copyright 2025-2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import Pyxsim as px
from itertools import zip_longest
from pathlib import Path
from filelock import FileLock
import os
from scipy.signal.windows import blackmanharris
from numpy.fft import rfft, irfft
from numpy import argmax, sqrt, mean, absolute, arange, log10

# Print the comparison in human friendly format
def check_expected_vs_output(expected, capfd, verbose=False):
    out, err = capfd.readouterr()
    output = out.split('\n')[:-1] # Need to trim last line
    match = True
    printed_header = False

    with capfd.disabled():
        if err:
            print(f"Exceptions encountered: {err}") # Show any exceptions
        
        for e, o in zip_longest(expected, output, fillvalue = ''):
            line_match = e == o
            if verbose or not line_match:
                if not printed_header:
                    print(f"\n{'***EXPECTED***':<40}{'***ACTUAL***':<40}")
                    printed_header = True
                print(f"{e:<40}{o:<40}{"Same" if line_match else "Diff"}")
            if not line_match:
                match = False

    return output, match

# Thread safe create a folder
def create_if_needed(folder):
    lock_path = f"{folder}.lock"
    # xdist can cause race conditions so use a lock
    with FileLock(lock_path):
        if not os.path.exists(folder):
            os.makedirs(folder)
        return folder

def unit_filter_test(capfd, request, test_name):
    cwd = Path(request.fspath).parent
    binary = f'{cwd}/{test_name}/bin/{test_name}.xe'

    assert Path(binary).exists(), f"Cannot find {binary}"

    with open(cwd/f"expected/{test_name}.expect") as exp:
        expected = exp.read().splitlines()

    px.run_on_simulator_(
        binary,
        do_xe_prebuild=False,
        capfd=capfd
    )

    output, match = check_expected_vs_output(expected, capfd, verbose=False)

    assert match, output

def rms_flat(a, sample_rate):
    """
    Return the root mean square of all the elements of *a*, flattened out.
    """
    return sqrt(mean(absolute(a)**2))


def find_range(f, x):
    """
    Find range between nearest local minima from peak at index x
    """
    uppermin = lowermin = x
    for i in arange(x+1, len(f)):
        if f[i+1] >= f[i]:
            uppermin = i
            break
    for i in arange(x-1, 0, -1):
        if f[i] <= f[i-1]:
            lowermin = i + 1
            break
    return (lowermin, uppermin)


def THDN_and_freq(signal, sample_rate):
    """
    Measure the THD+N for a signal and print the results

    Prints the estimated fundamental frequency and the measured THD+N.  This is
    calculated from the ratio of the entire signal before and after
    notch-filtering.

    Currently this tries to find the "skirt" around the fundamental and notch
    out the entire thing.  A fixed-width filter would probably be just as good,
    if not better.
    """
    # Get rid of DC and window the signal
    signal -= mean(signal) # TODO: Do this in the frequency domain, and take any skirts with it?
    windowed = signal * blackmanharris(len(signal))  # TODO Kaiser?

    # Measure the total signal before filtering but after windowing
    total_rms = rms_flat(windowed, sample_rate)

    # Find the peak of the frequency spectrum (fundamental frequency), and
    # filter the signal by throwing away values between the nearest local
    # minima
    f = rfft(windowed)
    i = argmax(abs(f))
    corr = (f[i - 1] - f[i + 1]) / (2 * f[i] - f[i - 1] - f[i + 1])
    freq = (sample_rate * (i / len(windowed)))  # Not exact
    freq = abs(freq + corr.real)
    # print('Frequency: %f Hz' % freq) 
    lowermin, uppermin = find_range(abs(f), i)
    f[lowermin: uppermin] = 0

    # Transform noise back into the signal domain and measure it
    # TODO: Could probably calculate the RMS directly in the frequency domain instead
    noise = irfft(f)
    THDN = rms_flat(noise, sample_rate) / total_rms

    result = "THD+N:     %.4f%% or %.1f dB" % (THDN * 100, 20 * log10(THDN))
    # print(result)

    return 20 * log10(THDN) , freq

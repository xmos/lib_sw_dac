# Copyright 2025 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import pytest
import numpy as np
from pathlib import Path
from helpers import THDN_and_freq
import subprocess

bin_path = Path(__file__).parent / "test_filter_thdn" / "bin"
test_freq = 997
fsup = int(1.5 * (10 ** 6))

def float_to_qxx(arr_float, q = 31, dtype = np.int32):
  arr_int32 = np.clip((np.array(arr_float) * (1 << q)), np.iinfo(dtype).min, np.iinfo(dtype).max).astype(dtype)
  return arr_int32

def qxx_to_float(arr_int, q = 31):
  arr_float = np.array(arr_int).astype(np.float64) / (1 << q)
  return arr_float

@pytest.mark.parametrize("test_fs", [48000, 
                                     96000, 
                                     192000,
                                     44100,
                                     88200,
                                     176400
                                     ])
def test_filter_thdn(test_fs):
  # gen sig
  test_path = bin_path / str(test_fs)
  time = np.arange(0, 0.05, 1 / test_fs)
  sig_fl = 0.9 * np.sin(2 * np.pi * test_freq * time)
  sig_int = float_to_qxx(sig_fl)
  sig_int.tofile(test_path /"in.bin")

  # run bin
  
  run_cmd = "xsim " + str(test_path) + f"/test_filter_thdn_{test_fs}.xe"
  stdout = subprocess.check_output(run_cmd, cwd = test_path, shell = True)
  if 0: print("run msg:\n", stdout.decode())

  # get sig, thdn

  sig_bin = test_path / "out.bin"
  assert sig_bin.is_file(), "could not find the output bin"
  sig_out_int = np.fromfile(sig_bin, dtype=np.int32)

  thdn, freq = THDN_and_freq(sig_out_int.astype(np.float64), fsup)
  assert thdn < -43 # the signal is too short for a good thdn
  # slight frequency shift has been seen while running these tests
  # not sure if it's due to an algorithm or the analysis
  assert test_freq - 3 <= freq <= test_freq + 3
  print(f"C  {test_fs}k THDN: {thdn}, fc: {freq}")

  # import soundfile as sf
  # sig_out_fl = qxx_to_float(sig_out_int)
  # sf.write("out.wav", sig_out_fl, fsup, "PCM_24")

# Copyright 2025 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

from pathlib import Path
import Pyxsim as px

bin_path = Path(__file__).parent / "test_pre_distort" / "bin" / "test_pre_distort.xe"

def test_pre_distort(capfd):
  px.run_on_simulator_(bin_path, do_xe_prebuild=False)
  out, err = capfd.readouterr()
  assert out == "pass\n"

# Copyright 2025-2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import pytest
from pathlib import Path
import subprocess


def test_exit():
    bin_path = Path(__file__).parent / "test_exit" / "bin" / "test_exit.xe"
    run_cmd = f"xsim {bin_path}"
    stdout = subprocess.check_output(run_cmd, shell = True)
    print("run msg:\n", stdout.decode())

# Copyright 2025 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import pytest
import Pyxsim
from Pyxsim import testers
from pathlib import Path

@pytest.mark.xfail(reason="Expected fail", strict=True)
def test_template_fail(level, capfd):

    # Might want to pytest.skip if level == "smoke"

    binary = Path(__file__).parent / "test_template_fail" / "bin" / "test_template_fail.xe"

    expect_file = Path(__file__).parent / "test_template_fail" / "pass.expect"

    tester = testers.ComparisonTester(open(expect_file), regexp=True)

    max_cycles = 15000000

    simargs = [
        "--max-cycles",
        str(max_cycles),
    ]

    result = Pyxsim.run_on_simulator(
        binary,
        cmake=True,
        simargs=simargs,
        tester=tester,
        capfd=capfd,
        clean_before_build=False)

    assert result


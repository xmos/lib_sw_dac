xsim --xscope "-offline xscope.xmt" --vcd-tracing "-o trace.vcd -tile tile[0] -ports -clock-blocks -instructions -cores" --trace-to trace.txt --args bin/test_sigma_delta.xe 0 100
xobjdump -S bin/test_sigma_delta.xe > disass.txt


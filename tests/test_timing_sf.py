# Copyright 2025 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import re
import statistics
import pytest
from pathlib import Path
import Pyxsim as px
import shutil
from filelock import FileLock
from helpers import create_if_needed
import math

cpu_clock_hz = 600e6
sd_clock_hz = 25e6 # set to ref / 4 in fw test app
clock_factor = sd_clock_hz / 24e6 # Because we are not testing at the 24MHz design spec in the sim
num_channels = 2
trim_pcm_samples = 2 # We get weird effects at the end so trim some

def extract_from_trace(file_path):
    # regex to parse sim output lines
    line_re = re.compile(r"tile\[0\]@(\d+).([ \-PSDIApwa\.]+)([0123456789abcdef]+)? \(([^)]+)\).*?:\s*([^@]+)@(\d+)")

    # storage
    events = []

    hit_main = False
    num_threads = 0
    app_thread_id = 0
    sd_0_thread_id = 0
    sd_1_thread_id = 0

    with open(file_path) as f:
        for line in f:
            m = line_re.search(line)
            if not m:
                continue
            thread = int(m.group(1))  # thread id

            pause = ('P' in m.group(2))
            addr = int(m.group(3).strip(), 16)
            task = m.group(4).strip()
            instr = m.group(5).strip()
            tstamp = int(m.group(6))

            # discard anything at startup
            if "main" in task and not hit_main:
                hit_main = True
                print(f"Hit main @{tstamp}")

            if hit_main:
                num_threads = thread + 1 if (thread + 1) > num_threads else num_threads

                events.append({
                    "time": tstamp,
                    "thread": thread,
                    "pause": pause,
                    "addr" : addr,
                    "task": task,
                    "instr": instr
                })

                if "test_app" in task:
                    app_thread_id = thread
                if "filter_task" in task:
                    sd_0_thread_id = thread
                if "sigma_delta_1_5" in task:
                    sd_1_thread_id = thread

    print(f"Number of active threads: {num_threads}")

    return events, app_thread_id, sd_0_thread_id, sd_1_thread_id

"""
This task checks:
- Number of outs from the producer and calculates the overall rate
- Number of outs and pause time for SD stage 0 to stage 1
- Number of outpws 
"""
def analyse_extracted_trace(events, app_thread_id, sd_0_thread_id, sd_1_thread_id, skip_loops, target_pwm_rate, verbose=False):
    #now look for outs and pauses
    thread_events = [["out"],   # test_app
                    ["out"],   # sd 0
                    ["outpw"]] # sd 1

    sample_count = 0 # Start at zero from the first PCM
    pwm_count = 0
    last_pcm_out_time = 0
    sd_0_last_out_time = 0
    sd_0_paused_time = 0
    sd_0_was_paused = False

    # For stats
    app_thread_loop_times = []
    sd_0_pause_times_ns = []
    sd_1_pwm_frames_out = []

    for event in events:
        thread = event["thread"]
        instr = event["instr"]
        addr = event["addr"]
        time = event["time"]
        paused = event["pause"]
        
        # print(thread, instr, time, paused)

        if thread == app_thread_id and any(k in instr for k in thread_events[0]) and not paused: # PCM out instruction from test_app
            sample_count += 1
            if sample_count % num_channels == 0:
                pcm_out_time = time

                pcm_rate_hz = cpu_clock_hz / (pcm_out_time - last_pcm_out_time) / clock_factor
                if verbose: print(f"Thread app PCM rate: {pcm_rate_hz}")

                if sample_count > skip_loops * num_channels:
                    app_thread_loop_times.append(pcm_out_time - last_pcm_out_time)
                else:
                    if verbose: print("SKIPPED")
                last_pcm_out_time = pcm_out_time
        
        if thread == sd_0_thread_id and any(k in instr for k in thread_events[1]):
            if paused:
                if verbose: print(f"Thread sd_0 paused at addr: {hex(addr)} at time: {time}")
                sd_0_paused_time = time
                sd_0_was_paused = True
            else:
                # Pause time will be zero if no Pause on OUT
                if not sd_0_was_paused:
                    sd_0_paused_time = time

                sd_0_period_ns = (time - sd_0_last_out_time) / cpu_clock_hz * 1e9
                if verbose: print(f"Thread sd_0 period {sd_0_period_ns}ns at addr: {hex(addr)}, paused for time: {time - sd_0_paused_time}")
                sd_0_last_out_time = time

                if verbose: print(f"PWM out count since last 1->2: {pwm_count}")
                if sample_count > skip_loops * num_channels:
                    sd_1_pwm_frames_out.append(pwm_count)
                    sd_0_pause_time_ns = (time - sd_0_paused_time) / cpu_clock_hz * 1e9
                    sd_0_pause_times_ns.append(sd_0_pause_time_ns)

                pwm_count = 0
                sd_0_was_paused = False

        if thread == sd_1_thread_id and any(k in instr for k in thread_events[2]):
            # we have an output to the ports
            if not paused:
                pwm_count += 1

    #hack alert - last couple of results can be dodgy due to exit
    trim_last = 2
    app_thread_loop_times = app_thread_loop_times[0:-trim_last]
    sd_0_pause_times_ns = sd_0_pause_times_ns[0:-trim_last]

    pcm_rate_hz = cpu_clock_hz / statistics.fmean(app_thread_loop_times) / clock_factor
    sd_0_pause_pc = statistics.fmean(sd_0_pause_times_ns) / statistics.fmean(app_thread_loop_times) * 100
    sd_1_ave_pwm_frames = statistics.fmean(sd_1_pwm_frames_out) / num_channels
    pwm_output_rate = sd_1_ave_pwm_frames * pcm_rate_hz

    print("**Analysis complete**")
    print(f"App PCM rate average over {len(app_thread_loop_times)} loops: {pcm_rate_hz}")
    # print(app_thread_loop_times)
    print(f"sd_0 average pause time: {sd_0_pause_pc:.2f}%")
    print(f"sd_1 PWM average frames per PCM sample: {sd_1_ave_pwm_frames}")
    print(f"sd_1 PWM output rate: {pwm_output_rate:.2f}Hz ({target_pwm_rate:.2f}Hz)")
    print("\n")

    return pcm_rate_hz, sd_0_pause_pc, pwm_output_rate

@pytest.mark.parametrize("sample_rate", [44100, 48000, 88200, 96000, 176400, 192000])
@pytest.mark.parametrize("burn", [1, 0])
def test_thread_performance_sf(request, sample_rate, burn):
    cwd = Path(request.fspath).parent
    test_name = "timing_test_sf"
    binary = Path(f'{cwd}/{test_name}/bin/{test_name}.xe')
    assert Path(binary).exists(), f"Cannot find {binary}"
    tmp_binary = Path(f'{cwd}/{test_name}/bin/{test_name}_{sample_rate}_{burn}.xe') # Needed for xdist
    create_if_needed("logs")
    with FileLock(str("file.lock")):
        shutil.copy2(binary, tmp_binary)

    skip_loops = 14 # Compensate for startup pipe filling and rates stabilising, and control outs etc
    num_loops = 100 + skip_loops # Enough to get a nice even number of PCM outs to make average representative
    trace_file = f"logs/trace_{sample_rate}_{burn}_{num_loops}.txt"

    px.run_on_simulator_(
        tmp_binary,
        do_xe_prebuild=False,
        simargs=f"--trace-to {trace_file} --args {tmp_binary} {sample_rate} {burn} {num_loops}".split()
    )
    tmp_binary.unlink()

    # print(out, err)
    target_pwm_rate = 1.5e6

    events, app_thread_id, sd_0_thread_id, sd_1_thread_id = extract_from_trace(trace_file)
    print(f"IDs app: {app_thread_id}, sd0: {sd_0_thread_id}, sd1: {sd_1_thread_id}")
    pcm_rate_hz, sd_0_pause_pc, pwm_output_rate = analyse_extracted_trace(events,
                                                                              app_thread_id,
                                                                              sd_0_thread_id,
                                                                              sd_1_thread_id,
                                                                              skip_loops,
                                                                              target_pwm_rate,
                                                                              verbose=False)

    assert math.isclose(pwm_output_rate, target_pwm_rate, rel_tol=0.001), f"PWM rate not achieved: {pwm_output_rate:.2f} ({target_pwm_rate})"
    
    



--> Test aots/comp-opt-bug.aot <--
Time:  0.011
--> Test aots/comp-unopt-bug.aot <--
Time:  0.003
--> Test aots/indirect.aot <--
Time:  0.003
--> Test aots/input-dep.aot <--
Traceback (most recent call last):
  File "./run_tests.py", line 385, in <module>
    main()
  File "./run_tests.py", line 381, in main
    dispatch_fn(args, run_fn)
  File "./run_tests.py", line 324, in run_normal
    print("Time: ", run_inst(file_arg))
  File "./run_tests.py", line 258, in run_inst_normal
    result = subprocess.run(f"time iwasm {fpath}",
  File "/usr/lib/python3.8/subprocess.py", line 495, in run
    stdout, stderr = process.communicate(input, timeout=timeout)
  File "/usr/lib/python3.8/subprocess.py", line 1028, in communicate
    stdout, stderr = self._communicate(input, endtime, timeout)
  File "/usr/lib/python3.8/subprocess.py", line 1868, in _communicate
    ready = selector.select(timeout)
  File "/usr/lib/python3.8/selectors.py", line 415, in select
    fd_event_list = self._selector.poll(timeout)
KeyboardInterrupt

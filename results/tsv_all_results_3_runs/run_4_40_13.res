--> Batch: batch.comp-opt-bug <--
Traceback (most recent call last):
  File "./run_tests.py", line 385, in <module>
    main()
  File "./run_tests.py", line 381, in main
    dispatch_fn(args, run_fn)
  File "./run_tests.py", line 356, in run_tsv
    run_batch_tsv_test (test_name, args.batch, run_inst)
  File "./run_tests.py", line 300, in run_batch_tsv_test
    run_times.append(float(run_inst(part_file, header=False)))
  File "./run_tests.py", line 219, in run_inst_tsv
    result = subprocess.run(f"iwasm --native-lib=./libtsvd.so {fpath}",
  File "/usr/lib/python3.8/subprocess.py", line 495, in run
    stdout, stderr = process.communicate(input, timeout=timeout)
  File "/usr/lib/python3.8/subprocess.py", line 1028, in communicate
    stdout, stderr = self._communicate(input, endtime, timeout)
  File "/usr/lib/python3.8/subprocess.py", line 1868, in _communicate
    ready = selector.select(timeout)
  File "/usr/lib/python3.8/selectors.py", line 415, in select
    fd_event_list = self._selector.poll(timeout)
KeyboardInterrupt

#!/usr/bin/python3

import sys
import struct
import subprocess
from pathlib import Path
from argparse import ArgumentParser

shared_acc_dir = Path('shared_access')
aot_dir = Path('aots')


def _parse_main():
    p = ArgumentParser(
            description="Shared access instrumentation script")
    p.add_argument("--batch", default = 0, type=int,
            help = "Run in batch mode with given batch size")
    p.add_argument("files", nargs='+',
            help = "Files to run ('all' for all files)")
    return p


def file_type(s):
    name = s.name
    if name.startswith('batch'):
        return 'batch'
    elif name.startswith('part'):
        return 'part'
    else:
        return 'norm'


def run_inst(exec_path, header=True):
    fpath = str(exec_path)
    if header:
        print (f"--> Test {fpath} <--")
    result = subprocess.run(f"iwasm --native-lib=./libaccess.so {fpath}",
            shell=True, check=True, capture_output=True, text=True,
            universal_newlines=True)
    filename = '.'.join(exec_path.name.split('.')[:-2])
    bin_target = shared_acc_dir / Path(filename + '.shared_acc.bin')
    subprocess.run(f"mkdir -p {shared_acc_dir}; "
            f"mv shared_mem.bin {str(bin_target)}", shell=True)
    
    return result.stderr[12:-1]


def aggregate_bins(bin_paths, out_path):

    bin_path_list = list(bin_paths)

    int_list = []
    # Read binary data from all files
    for bin_path in bin_path_list:
        with open(bin_path, 'rb') as f:
            while True:
                int_bytes = f.read(4)
                if not int_bytes:
                    break
                int_val = struct.unpack('<i', int_bytes)[0]
                int_list.append(int_val)

    # Sort and remove duplicates from the list
    sorted_ints = sorted(set(int_list))

    # Write sorted and unique integers to new binary file
    with open(out_path, 'wb') as outfile:
        for int_val in sorted_ints:
            outfile.write(struct.pack('<i', int_val))

    # Remove files
    for bin_path in bin_path_list:
        bin_path.unlink()

    return sorted_ints


def run_batch_test (test_name, batch_size):
    print(f"<-- Batch: {test_name} -->")
    run_times = []
    for part_file in sorted(aot_dir.glob(f"part*.{test_name}.aot.accinst")):
        batch_id = int(part_file.name.split('.')[0][4:])
        if batch_id <= batch_size:
            run_times.append(float(run_inst(part_file, header=False)))

    out_path = shared_acc_dir / f"batch.{test_name}.shared_acc.bin"
    # Aggregate results from run
    sorted_idxs = aggregate_bins ( shared_acc_dir.glob(f"part*.{test_name}.shared_acc.bin"), \
                        out_path)

    print("Max Time: ", max(run_times))
    # Print accuracy if possible
    try:
        single_result = shared_acc_dir / f"{test_name}.shared_acc.bin"
        optimal_size = single_result.stat().st_size // 4
        print("Accuracy: ", (len(sorted_idxs) / optimal_size) * 100)
    except OSError:
        print("Accuracy: N/A")


def main():
    p = _parse_main()
    args = p.parse_args()
    

    if args.batch:
        if args.files[0] == "all":
            test_map = {}
            test_names = { str(part_file).split('.')[1] for part_file in aot_dir.glob(f"part*.aot.accinst") }

            for test_name in sorted(test_names):
                run_batch_test (test_name, args.batch)

        else:
            for test_name in args.files:
                run_batch_test (Path(test_name), args.batch)

    else:
        if args.files[0] == "all":
            for exec_path in sorted(aot_dir.glob('*.aot.accinst')):
                if (file_type(exec_path) == "norm"):
                    print("Time: ", run_inst(exec_path))

        else:
            for exec_path in args.files:
                print("Time: ", run_inst(Path(exec_path)))
            

if __name__ == '__main__':
    main()

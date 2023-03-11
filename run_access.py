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
    p.add_argument("--mode", default = "single", 
            choices = ["single", "batch"], 
            help = "Mode to run")
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


def run_inst(exec_path):
    fpath = str(exec_path)
    print (f"--> Test {fpath} <--")
    subprocess.run("iwasm --native-lib=./libaccess.so "
            f"{fpath} > /dev/null", shell=True, check=True)   
    filename = '.'.join(exec_path.name.split('.')[:-2])
    bin_target = shared_acc_dir / Path(filename + '.shared_acc.bin')
    subprocess.run(f"mkdir -p {shared_acc_dir}; "
            f"mv shared_mem.bin {str(bin_target)}", shell=True)


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

    print(sorted_ints)


def run_batch_test (test_name):
    for part_file in aot_dir.glob(f"part*.{test_name}.aot.accinst"):
        run_inst(part_file)

    out_path = shared_acc_dir / Path(f"batch.{test_name}.shared_acc.bin")
    # Aggregate results from run
    aggregate_bins ( shared_acc_dir.glob(f"part*.{test_name}.shared_acc.bin"), \
                        out_path)


def main():
    p = _parse_main()
    args = p.parse_args()
    
    if args.mode == "single":
        if args.files[0] == "all":
            for exec_path in aot_dir.glob('*.aot.accinst'):
                if (file_type(exec_path) == "norm"):
                    run_inst(exec_path)
        else:
            for exec_path in args.files:
                run_inst(exec_path)

    else:
        if args.files[0] == "all":
            test_map = {}
            test_names = { str(part_file).split('.')[1] for part_file in aot_dir.glob(f"part*.aot.accinst") }

            for test_name in test_names:
                run_batch_test (test_name)

        else:
            for test_name in args.files:
                run_batch_test (test_name)
            

if __name__ == '__main__':
    main()

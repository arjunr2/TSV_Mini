#!/usr/bin/python3

import sys
import re
import struct
import subprocess
from pathlib import Path
from argparse import ArgumentParser
from collections import namedtuple

shared_acc_dir = Path('shared_access')
violation_dir = Path('violation_logs')
aot_dir = Path('aots')

run_modes = ["normal", "access", "tsv"]

class ViolationPair():
    def __init__(self, i1, i2):
        self.i1 = i1
        self.i2 = i2

    def __eq__(self, other):
        return  (self.i1 == other.i1 and self.i2 == other.i2) or \
                (self.i1 == other.i2 and self.i2 == other.i1)

    def __hash__(self):
        return hash((self.i1, self.i2)) ^ hash((self.i2, self.i1))



def _parse_main():
    p = ArgumentParser(
            description="Shared access instrumentation script")
    p.add_argument("--mode", default = "normal", type=str,
            choices = run_modes,
            help = "Select whether to run uninstrumented (normal), "
            "phase 1 access profiling (access) or final TSV detection (tsv)")
    p.add_argument("--batch", default = 0, type=int,
            help = "Run in batch mode with given batch size (only applies to access and tsv mode")
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


# Merge commit operations
def clean_files (fpath_list, out_path):
    # Remove files (unless writing to same file as reading)
    for fpath in fpath_list:
        if fpath != out_path:
            fpath.unlink()


    

def merge_tsv_violations (violation_paths, out_path):
    vpath_list = list(violation_paths)

    violation_ct = 0
    violations_merged = {}
    comp_str = r"Instructions \((\d+)\s*,\s*(\d+)\s*\)"
    total_len = 0
    for vpath in vpath_list:
        with open(vpath, 'r') as f:
            content = f.readlines()
            total_len += len(content)
            violations_merged.update ( \
                { ViolationPair(*re.search(comp_str, line).group(1,2)) : line \
                for line in content } )

    with open(out_path, 'w') as outfile:
        outfile.writelines(violations_merged.values())

    clean_files (vpath_list, out_path)

    return len(violations_merged)


# Merge processing operations
AccessRecord = namedtuple('AccessRecord', ['tid', 'has_write', 'inst_idxs'])

def merge_write_and_clean(sorted_ints, bin_path_list, out_path):
    # Write sorted and unique integers to new binary file
    with open(out_path, 'wb') as outfile:
        for int_val in sorted_ints:
            outfile.write(struct.pack('<i', int_val))

    clean_files(bin_path_list, out_path)

def merge_access_bins(bin_paths, out_path):
    
    bin_path_list = list(bin_paths)

    shared_idxs = set()
    shared_addrs = set()
    partials = {}

    byte_offsets = {}

    # Obtain shared data
    for bin_path in bin_path_list:
        offset = 0

        with open(bin_path, 'rb') as f:
            byte_str = f.read()
            # Read shared instructions
            sh_first = "<I"
            (sh_ct,), offset = struct.unpack_from(sh_first, byte_str, offset), \
                offset + struct.calcsize(sh_first)
            sh_fmt = f"<{sh_ct}I"
            vec_end = offset + struct.calcsize(sh_fmt)
            shared_idxs |= set(struct.unpack_from(sh_fmt, byte_str, offset))
            offset = vec_end

            # Read shared addrs
            (sh_ct,), offset = struct.unpack_from(sh_first, byte_str, offset), \
                offset + struct.calcsize(sh_first)
            sh_fmt = f"<{sh_ct}I"
            vec_end = offset + struct.calcsize(sh_fmt);
            shared_addrs |= set(struct.unpack_from(sh_fmt, byte_str, offset))
            offset = vec_end

            byte_offsets[bin_path.name] = offset

    # Process partials
    for bin_path in bin_path_list:
        offset = byte_offsets[bin_path.name]

        with open(bin_path, 'rb') as f:
            byte_str = f.read()
            # Read partials
            while offset != len(byte_str):
                first = "<Iq?I"
                acc, offset = struct.unpack_from(first, byte_str, offset), \
                    offset + struct.calcsize(first)
                addr, addr_rec = acc[0], list(acc[1:])

                second = f"<{addr_rec[-1]}I"
                entry_list, offset = set(struct.unpack_from(second, byte_str, offset)), \
                    offset + struct.calcsize(second)
                addr_rec[-1] = entry_list

                acc_record = AccessRecord._make(addr_rec)
                # Add as shared idx if accessing a shared address
                if addr in shared_addrs:
                    shared_idxs |= acc_record.inst_idxs
                else:
                    # Add as shared idx if diff tids conflict (R/W) 
                    # with any other partials
                    if addr in partials:
                        has_write = partials[addr].has_write or acc_record.has_write
                        diff_tid = partials[addr].tid != acc_record.tid
                        comb_set = partials[addr].inst_idxs | acc_record.inst_idxs
                        # Found a conflict
                        if has_write and diff_tid:
                            shared_addrs.add(addr)
                            shared_idxs |= comb_set
                            del partials[addr]
                        # NOTE we are setting tid=0 if diff since no threads should
                        # ever have that
                        else:
                            tid = 0 if diff_tid else acc_record.tid
                            partials[addr]._replace (tid = tid, \
                                has_write = has_write, inst_idxs = comb_set)

                    else:
                        partials[addr] = acc_record

    #print("Len: ", len(shared_idxs))
    #print("Shared Idxs: ", shared_idxs)
    #print("Shared Addrs: ", shared_addrs)
    #print("Partials: ", partials)
    # Write merge and remove partitions
    merge_write_and_clean(shared_idxs, bin_path_list, out_path)

    return shared_idxs


# Legacy aggregator: Use merge_access_bins
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

    # Write merge and remove partitions
    merge_write_and_clean(sorted_ints, bin_path_list, out_path)

    return sorted_ints



# Instance runs for each mode
def run_inst_tsv(exec_path, header=True):
    fpath = str(exec_path)
    if header:
        print (f"--> Test {fpath} <--")
    result = subprocess.run(f"iwasm --native-lib=./libtsvd.so {fpath}",
            shell=True, check=True, capture_output=True, text=True,
            universal_newlines=True)
    filename = '.'.join(exec_path.name.split('.')[:-2])
    res_target = violation_dir / Path(filename + '.violation')
    subprocess.run(f"mkdir -p {violation_dir}; "
            f"mv violations.bin {str(res_target)}", shell=True)
    
    time_str = re.search("Time:\s*(.*)", result.stderr).group(1)

    return time_str


def postprocess_access(exec_path, out_path):
    merge_access_bins ([out_path], out_path)

def run_inst_access(exec_path, header=True, \
                            postprocessor = lambda exec_path, out_path: None):
    fpath = str(exec_path)
    if header:
        print (f"--> Test {fpath} <--")
    result = subprocess.run(f"iwasm --native-lib=./libaccess.so {fpath}",
            shell=True, check=True, capture_output=True, text=True,
            universal_newlines=True, executable='/bin/bash')
    filename = '.'.join(exec_path.name.split('.')[:-2])
    bin_target = shared_acc_dir / Path(filename + '.shared_acc.bin')
    subprocess.run(f"mkdir -p {shared_acc_dir}; "
            f"mv shared_mem.bin {str(bin_target)}", shell=True)
    
    time_str = re.search("Time:\s*(.*)", result.stderr).group(1)

    postprocessor (exec_path, bin_target)
    
    return time_str


def run_inst_normal(exec_path):
    fpath = str(exec_path)
    print (f"--> Test {fpath} <--")
    result = subprocess.run(f"time iwasm {fpath}",
            shell=True, check=True, capture_output=True, text=True,
            universal_newlines=True)

    mins, secs = re.search("real\s*(.*)m(.*)s", result.stderr).group(1, 2)
    exec_time = float(mins) * 60 + float(secs)
    
    return exec_time


# Batch runs for each mode
def run_batch_access_test (test_name, batch_size, run_inst):
    print(f"--> Batch: {test_name} <--")
    run_times = []
    for part_file in sorted(aot_dir.glob(f"part*.{test_name}.aot.accinst")):
        part_id = int(part_file.name.split('.')[0][4:])
        if part_id <= batch_size:
            run_times.append(float(run_inst(part_file, header=False)))

    out_path = shared_acc_dir / f"batch.{test_name}.shared_acc.bin"
    # Aggregate results from run
    #sorted_idxs = aggregate_bins ( shared_acc_dir.glob(f"part*.{test_name}.shared_acc.bin"), \
    #                    out_path)
    sorted_idxs = merge_access_bins ( shared_acc_dir.glob(f"part*.{test_name}.shared_acc.bin"), \
                        out_path)

    print("Max Time: ", max(run_times))
    # Print accuracy if possible
    try:
        single_result = shared_acc_dir / f"{test_name}.shared_acc.bin"
        optimal_size = single_result.stat().st_size // 4
        print("Accuracy: {:.2f}".format((len(sorted_idxs) / optimal_size) * 100))
    except OSError:
        print("Accuracy: N/A")


def run_batch_tsv_test (test_name, batch_size, run_inst):
    print(f"--> Batch: {test_name} <--")
    run_times = []
    for part_file in sorted(aot_dir.glob(f"part*.{test_name}.aot.tsvinst")):
        part_id = int(part_file.name.split('.')[0][4:])
        if part_id <= batch_size:
            run_times.append(float(run_inst(part_file, header=False)))

    out_path = violation_dir / f"disp.{test_name}.violation"
    # Aggregate results from run
    num_violations = merge_tsv_violations ( violation_dir.glob(f"part*.{test_name}.violation"), \
                        out_path)

    print("Max Time: ", max(run_times))
    # Print accuracy if possible
    try:
        single_result = violation_dir / f"{test_name}.violation"
        with open(single_result, 'r') as f:
            single_violations = len(f.readlines())
        print("Relative Violations: {}".format(num_violations - single_violations))
    except OSError:
        print("Relative Violations: N/A")


# Main run dispatchers
def run_normal(args, run_inst):
    file_args = aot_dir.glob('*.aot') if args.files[0] == "all" \
                    else [aot_dir / Path(f+'.aot') for f in args.files]

    for file_arg in sorted(file_args):
        print("Time: ", run_inst(file_arg))


def run_access(args, run_inst):
    if args.batch:
        test_names = { str(part_file).split('.')[1] for part_file in \
                            aot_dir.glob(f"part*.aot.accinst") } \
                            if args.files[0] == "all" \
                            else args.files

        for test_name in sorted(test_names):
            run_batch_access_test (test_name, args.batch, run_inst)
    
    else:
        file_args = [f for f in aot_dir.glob('*.aot.accinst') \
                        if file_type(f) == "norm"] if args.files[0] == "all" \
                        else [aot_dir / Path(f+'.aot.accinst') for f in args.files]

        for file_arg in sorted(file_args):
            print("Time: ", run_inst(file_arg, header=True, \
                                postprocessor = postprocess_access))


def run_tsv(args, run_inst):
    if args.batch:
        # Use only batch files for now
        test_names = { '.'.join(part_file.name.split('.')[1:3]) \
                            for part_file in aot_dir.glob(f"part*.aot.tsvinst") \
                            if part_file.name.split('.')[1] == "batch" } \
                        if args.files[0] == "all" \
                        else args.files
        for test_name in sorted(test_names):
            run_batch_tsv_test (test_name, args.batch, run_inst)

    else:
        # Use batch file over normal file, if it exists
        file_args = [f for f in aot_dir.glob('*.aot.tsvinst') \
                        if file_type(f) == "batch" or 
                            (file_type(f) == "norm" and not (f.parent/f"batch.{f.name}").is_file())] \
                        if args.files[0] == "all" \
                        else [aot_dir / Path(f+'.aot.tsvinst') for f in args.files]

        for file_arg in sorted(file_args):
            print("Time: ", run_inst(file_arg))

# Mappings
run_scripts = {
    "normal": (run_normal, run_inst_normal),
    "access": (run_access, run_inst_access),
    "tsv": (run_tsv, run_inst_tsv)
}

def main():
    p = _parse_main()
    args = p.parse_args()
   
    dispatch_fn, run_fn = run_scripts[args.mode]
    dispatch_fn(args, run_fn)


if __name__ == '__main__':
    main()

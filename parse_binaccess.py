import sys
import struct
from collections import namedtuple

AccessRecord = namedtuple('AccessRecord', ['addr', 'tid', 'has_write', 'inst_idxs'])


def main():
    bin_path = sys.argv[1]
    offset = 0
    mylist = []
    shared_idxs = []
    shared_addrs = set()

    with open(bin_path, 'rb') as f:
        byte_str = f.read()
        # Read shared instructions
        sh_first = "<I"
        (sh_ct,), offset = struct.unpack_from(sh_first, byte_str, offset), \
            offset + struct.calcsize(sh_first)

        sh_fmt = f"<{sh_ct}I"
        vec_end = offset + struct.calcsize(sh_fmt)
        shared_idxs += list(struct.unpack_from(sh_fmt, byte_str, offset))
        offset = vec_end
        print(shared_idxs)

        # Read shared addrs
        (sh_ct,), offset = struct.unpack_from(sh_first, byte_str, offset), \
            offset + struct.calcsize(sh_first)
        sh_fmt = f"<{sh_ct}I"
        vec_end = offset + struct.calcsize(sh_fmt);
        shared_addrs |= set(struct.unpack_from(sh_fmt, byte_str, offset))
        offset = vec_end
        print(shared_addrs)

        # Read partials
        while offset != len(byte_str):
            first = "<Iq?I"
            acc, offset = AccessRecord._make(struct.unpack_from(first, byte_str, offset)), \
                offset + struct.calcsize(first)
            second = f"<{acc.inst_idxs}I"
            (entry_list,), offset = struct.unpack_from(second, byte_str, offset), \
                offset + struct.calcsize(second)
            acc = acc._replace(inst_idxs=entry_list)
            print(acc)
            mylist.append(acc)

    

main()

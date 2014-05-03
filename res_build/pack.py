#!/usr/bin/env python
# -*- coding: UTF-8 -*-

FMT_HEADER = '4sII'
FMT_INDEX = 'III'

def write_new_pack(pack_path, res_rid_name_path_list):
    import struct
    with open(pack_path, 'wb') as f:
        f.write(struct.pack(FMT_HEADER, '????', 0, 0))
        res_rid_name_path_list = sorted(res_rid_name_path_list)

        pos = struct.calcsize(FMT_HEADER)
        res_rid_pos_size_name_list = []
        import os.path
        for rid, name, path in res_rid_name_path_list:
            with open(path, 'rb') as g:
                d = g.read()
            size = len(d)
            if size == 0:
                continue
            f.write(d)
            res_rid_pos_size_name_list.append((rid, pos, size, name))
            pos += size

        index_pos = pos
        for rid, pos, size, name in res_rid_pos_size_name_list:
            f.write(struct.pack(FMT_INDEX, rid, pos, size))
            print '%08x' % rid, name, pos, size

        f.seek(0)
        f.write(struct.pack(FMT_HEADER, 'PACK', len(res_rid_name_path_list), index_pos))

def fnv_hash(s):
    FNV_PRIME = 16777619
    UINT32_MAX = 2 ** 32
    h = 2166136261
    for c in s:
        h = h ^ ord(c)
        h = (h * FNV_PRIME) % UINT32_MAX
    # tail '\0'
    h = (h * FNV_PRIME) % UINT32_MAX
    return h

def main():
    import argparse
    parser = argparse.ArgumentParser(description='Pack resource.')
    parser.add_argument('-o', '--output', help='Output file.', required=True)
    parser.add_argument('-l', '--list', help='Resource list file.', required=True)
    args = parser.parse_args()
    res_rid_name_path_list = []
    import os
    print os.getcwd()
    with open(args.list, 'r') as f:
        for line in f:
            parts = line.split()
            if len(parts) != 2:
                continue
            name, path = parts
            res_rid_name_path_list.append((fnv_hash(name), name, path))
    write_new_pack(args.output, res_rid_name_path_list)

if __name__ == '__main__':
    main()

#!/bin/sh

mkdir -p build
if [[ ! -e src/bpf/vmlinux.h ]]; then
    bpftool btf dump file /sys/kernel/btf/vmlinux format c > src/bpf/vmlinux.h
fi
clang -g -O2 -target bpf -D__TARGET_ARCH_x64 -c src/bpf/trackers.bpf.c -o build/trackers.bpf.o
bpftool gen skeleton build/trackers.bpf.o > src/bpf/trackers.skel.h
clang++ --std=c++20 -g src/*.cpp -I /usr/include/bpf/ -I . -I src -I src/bpf -lbpf -lsqlite3 -o connectiontracker 

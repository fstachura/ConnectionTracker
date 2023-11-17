#!/bin/sh

bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h
clang -g -O2 -target bpf -D__TARGET_ARCH_x64 -c src/bpf/trackers.bpf.c -o build/trackers.bpf.o
bpftool gen skeleton build/trackers.bpf.o > src/bpf/trackers.skel.h
clang++ src/*.cpp -I /usr/include/bpf/ -I . -I src -I src/bpf -lbpf -o connectiontracker

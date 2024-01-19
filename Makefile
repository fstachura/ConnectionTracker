src/bpf/vmlinux.h: 
	bpftool btf dump file /sys/kernel/btf/vmlinux format c > src/bpf/vmlinux.h

build/trackers.bpf.o: src/bpf/vmlinux.h
	clang -g -O2 -target bpf -D__TARGET_ARCH_x64 -c src/bpf/trackers.bpf.c -o build/trackers.bpf.o

src/bpf/trackers.skel.h: build/trackers.bpf.o
	bpftool gen skeleton build/trackers.bpf.o > src/bpf/trackers.skel.h

connectiontracker: src/bpf/trackers.skel.h $(wildcard src/*.cpp) $(wildcard src/*.hpp)
	clang++ --std=c++20 -g src/*.cpp -I /usr/include/bpf/ -I . -I src -I src/bpf -lbpf -lsqlite3 -luring -o connectiontracker 

build: connectiontracker

clean:
	rm -r build/*
	rm connectiontracker


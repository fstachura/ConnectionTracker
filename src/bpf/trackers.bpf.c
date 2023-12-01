#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>
#include "trackers.h"

char LICENSE[] SEC("license") = "GPL";

#define ENOBUFS 105

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} rb SEC(".maps");

SEC("lsm/socket_connect")
int BPF_PROG(socket_connect_tracker, struct socket *sock, struct sockaddr* address, int socklen, int ret) {
    if(socklen > 0 && socklen < MAX_SOCKADDR_LEN) {
        struct connection_event* reserve_ptr = bpf_ringbuf_reserve(&rb, 
                sizeof(struct connection_event)+MAX_SOCKADDR_LEN, 0);
        if(!reserve_ptr) {
            bpf_printk("Failed to reserve ringbuf data\n");
            return -ENOBUFS;
        }

        long err = bpf_probe_read_kernel(
                &reserve_ptr->sockaddr, 
                MAX_SOCKADDR_LEN,
                (void*)address);
        if(err < 0) {
            bpf_printk("Failed to read from kernelspace %ld.\n", err);
            bpf_ringbuf_discard(reserve_ptr, 0);
            return -ENOBUFS;
        }
        if(reserve_ptr->sockaddr.sa_family == 0) {  // AF_UNSPEC
            bpf_ringbuf_discard(reserve_ptr, 0);
            return ret;
        }

        reserve_ptr->pid = bpf_get_current_pid_tgid() >> 32;
        reserve_ptr->sock_type = sock->type;
        reserve_ptr->boottime = bpf_ktime_get_boot_ns();
        bpf_get_current_comm(&reserve_ptr->comm, sizeof(reserve_ptr->comm));

        bpf_ringbuf_submit(reserve_ptr, 0);
        bpf_printk("%d\n", sock->type);
    } 
    return ret;
}

//SEC("tp/sched/sched_process_exec")
//int handle_exec_tp(struct trace_event_raw_sched_process_exec* ctx) {
//    return 0;
//}

//SEC("tp/syscalls/sched_process_fork") 
//int handle_fork_tp(struct trace_event_raw_sched_process_fork* ctx) {
//    return 0;
//}

//sendto
//sendmsg
//sendmmsg 
//all call sock_sendmsg, which recives msghdr with address
//get time

//events: connect, sock_sendmsg on unconnected

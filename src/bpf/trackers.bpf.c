#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>
#include "trackers.h"

char LICENSE[] SEC("license") = "GPL";

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} rb SEC(".maps");

SEC("tp/syscalls/sys_enter_connect")
int handle_connect_tp(struct trace_event_raw_sys_enter *ctx) {
    u32 socklen = (u32)ctx->args[2];
    u32 fd = (u32)ctx->args[1];
    u32 len = MAX_SOCKADDR_LEN > socklen ? socklen : MAX_SOCKADDR_LEN;

    struct connection_event* reserve_ptr = bpf_ringbuf_reserve(&rb, 
            sizeof(struct connection_event)+MAX_SOCKADDR_LEN, 0);
    if(!reserve_ptr) {
        bpf_printk("Failed to reserve ringbuf data\n");
        return 0;
    }

    //bpf_sock_from_file
    //track time
    struct task_struct* task = (struct task_struct*)bpf_get_current_task();
    struct fdtable* table = BPF_CORE_READ(task, files, fdt);
    void* pd = (*(table[fd].fd))->private_data;

    long err = bpf_probe_read_user(
            &reserve_ptr->sockaddr, 
            len, 
            (void*)ctx->args[1]);
    if(err != 0) {
        bpf_printk("Failed to read from userspace %ld.\n", err);
        bpf_ringbuf_discard(reserve_ptr, 0);
        return 0;
    }
    if(reserve_ptr->sockaddr.sa_family == 0) {  // AF_UNSPEC
        bpf_ringbuf_discard(reserve_ptr, 0);
        return 0;
    }

    reserve_ptr->pid = bpf_get_current_pid_tgid() >> 32;
    bpf_get_current_comm(&reserve_ptr->comm, sizeof(reserve_ptr->comm));

    bpf_ringbuf_submit(reserve_ptr, 0);
    return 0;
}

//SEC("tp/sched/sched_process_exec")
//int handle_exec_tp(struct trace_event_raw_sched_process_exec* ctx) {
//    return 0;
//}
//
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

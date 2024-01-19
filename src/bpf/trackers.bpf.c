#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>
#include "trackers.h"

char LICENSE[] SEC("license") = "GPL";

#define ENOBUFS 105

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * sizeof(struct connection_event));
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
            //bpf_printk("%d %d\n", sock->sk->sk_type, sock->sk->sk_protocol);
            bpf_ringbuf_discard(reserve_ptr, 0);
            return ret;
        }

        reserve_ptr->pid = bpf_get_current_pid_tgid() >> 32;
        reserve_ptr->sock_type = sock->type;
        reserve_ptr->boottime = bpf_ktime_get_boot_ns();
        bpf_get_current_comm(&reserve_ptr->comm, sizeof(reserve_ptr->comm));

        struct task_struct* task = bpf_get_current_task_btf();
        size_t size = task->mm->arg_end-task->mm->arg_start;
        bpf_probe_read_user(&reserve_ptr->cmdline, size > MAX_CMDLINE_SIZE ? MAX_CMDLINE_SIZE : size, 
                (const void*)task->mm->arg_start);
        reserve_ptr->cmdline_size = size;

        bpf_ringbuf_submit(reserve_ptr, 0);
        return ret;
    } else {
        return -ENOBUFS;
    }
}

//SEC("tp/sched/sched_process_exec")
//int handle_exec_tp(struct trace_event_raw_sched_process_exec* ctx) {
//    return 0;
//}

// task_alloc
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

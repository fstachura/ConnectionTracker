#ifndef __TRACKERS_H
#define __TRACKERS_H

#define MAX_SOCKADDR_LEN sizeof(struct __kernel_sockaddr_storage)

struct connection_event_sockaddr {
   sa_family_t sa_family;
   char sa_data[];
};

struct connection_event {
   int pid;
   char comm[16];
   short sock_type;
   uint64_t boottime;
   struct connection_event_sockaddr sockaddr;
};

#endif

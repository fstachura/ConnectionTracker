#ifndef __TRACKERS_H
#define __TRACKERS_H

#define MAX_SOCKADDR_LEN 256

struct connection_event_sockaddr {
   sa_family_t sa_family;
   char sa_data[];
};

struct connection_event {
   int pid;
   char comm[16];
   struct connection_event_sockaddr sockaddr;
};

#endif

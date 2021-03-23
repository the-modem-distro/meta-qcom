#ifndef _IPCSECURITY_H_
#define _IPCSECURITY_H_
struct rule {
   int rl_no;
   uint32_t service;
   uint32_t instance;
   unsigned dummy;
   gid_t group_id[0];
};

#define IOCTL_RULES	_IOR(0xC3, 5,  struct rule)

#endif
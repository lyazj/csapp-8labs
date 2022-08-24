#ifndef __PROXY_MESSAGE_HDR_
#define __PROXY_MESSAGE_HDR_

#include "csapp.h"
#include "head.h"
#include "body.h"

#define HOST_MAX (256)
#define PORT_MAX (16)

struct message {
  struct head *hd;
  struct body *bd;
};

struct message *read_message(rio_t *rio);
int write_message(int fd, const struct message *req);
void free_message(struct message *req);

#endif /* __PROXY_MESSAGE_HDR_ */

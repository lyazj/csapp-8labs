#ifndef __PROXY_REQUEST_HDR_
#define __PROXY_REQUEST_HDR_

#include "csapp.h"
#include "message.h"

struct request {
  struct message  *msg;
  char            s_host[HOST_MAX];
  char            s_port[PORT_MAX];
  char            d_host[HOST_MAX];
  char            d_port[HOST_MAX];
};

char *get_req_host(struct req_head *hd);
char *split_req_port(char *host);
int is_req_local(struct request *req);

struct request *read_request(rio_t *rio,
    struct sockaddr *s_addr, socklen_t s_addrlen);
int write_request(int fd, const struct request *req);
void free_request(struct request *req);

int send_request(const struct request *req);
void process_request(int fd, struct sockaddr *addr, socklen_t addrlen);

#endif /* __PROXY_REQUEST_HDR_ */

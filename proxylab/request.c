#include "request.h"
#include "response.h"

char *get_req_host(struct req_head *hd)
{
  char *host, *next, *header_host;

  host = get_path_host(hd->pth, (const char **)&next);
  header_host = get_header(hd->hdr, "Host");
  if(host)
    remove_path_host(hd->pth, next);
  else
  {
    if(!header_host)
      return NULL;
    host = Malloc(strlen(header_host) + 1);
    strcpy(host, header_host);
  }
  if(!header_host)
    add_header(&hd->hdr, "Host", host, HEADER_EXCLUSIVE);
  return host;
}

char *split_req_port(char *host)
{
  char *delim = strchr(host, ':');
  if(delim)
  {
    *delim++ = 0;
    return delim;
  }
  return NULL;
}

int is_req_local(struct request *req)
{
  const char *host = req->d_host;
  return !strcmp(host, "127.0.0.1")
    || !strcmp(host, "localhost");
}

struct request *read_request(rio_t *rio,
    struct sockaddr *s_addr, socklen_t s_addrlen)
{
  int r;
  char s_host[HOST_MAX], s_port[PORT_MAX];
  const char *ptc;
  char *d_host;
  char *d_port;
  size_t d_hostlen;
  struct message *msg;
  struct req_head *hd;
  struct request *req;

  msg = read_message(rio);
  if(!msg)
    return NULL;
  /* CLEANUP_parse_request_0 */

  hd = (struct req_head *)msg->hd;
  ptc = hd->ptc;
  if(strncmp(ptc, "HTTP/1.", 7) || (ptc[7] != '0' && ptc[7] != '1'))
  {
    fprintf(stderr,
        "ERROR: read_request: unsupported protocol: %s\n", ptc);
    goto CLEANUP_parse_request_0;
  }

  d_host = get_req_host(hd);
  if(!d_host)
  {
    const char *host = "localhost";
    d_host = Malloc(strlen(host) + 1);
    strcpy(d_host, host);
  }
  d_port = split_req_port(d_host);
  /* CLEANUP_parse_request_1 */

  d_hostlen = strlen(d_host);
  if(d_hostlen > HOST_MAX - 1)
  {
    fprintf(stderr, "ERROR: read_request: target host too long\n");
    goto CLEANUP_parse_request_1;
  }

  r = getnameinfo(s_addr, s_addrlen,
      s_host, sizeof s_host, s_port, sizeof s_port, NI_NUMERICSERV);
  if(r)
  {
    fprintf(stderr,
        "ERROR: read_request: getnameinfo: %s\n", gai_strerror(r));
    goto CLEANUP_parse_request_1;
  }
  fprintf(stderr, "INFO: got request from %s:%s to %s%s%s\n",
      s_host, s_port, d_host, d_port ? ":" : "", d_port ? d_port : "");

  req = Malloc(sizeof *req);
  req->msg = msg;
  strcpy(req->d_host, d_host);
  strcpy(req->d_port, d_port ? d_port : "");
  strcpy(req->s_host, s_host);
  strcpy(req->s_port, s_port);
  Free(d_host);
  return req;

  CLEANUP_parse_request_1:
  {
    Free(d_host);
  }
  /* fall through */
  CLEANUP_parse_request_0:
  {
    Free(msg);
    return NULL;
  }
}

void free_request(struct request *req)
{
  free_message(req->msg);
  Free(req);
}

int write_request(int fd, const struct request *req)
{
  return write_message(fd, req->msg);
}

int send_request(const struct request *req)
{
  int fd;

  fd = open_clientfd(
      (char *)req->d_host, (char *)(*req->d_port ? req->d_port : "80"));
  if(fd < 0)
  {
    fprintf(stderr, "ERROR: send_request: error opening client fd\n");
    return -1;
  }
  if(write_request(fd, req))
  {
    fprintf(stderr, "ERROR: send_request: error writing request\n");
    if(close(fd))
      perror("ERROR: send_request");
    return -1;
  }

  return fd;
}

void process_request(int fd,
    struct sockaddr *addr, socklen_t addrlen)
{
  int pid, r;
  struct request *req;
  rio_t rio;

  pid = Fork();
  if(pid)  /* parent */
    return;
  fprintf(stderr, "INFO: child process %d started\n", getpid());
  
  rio_readinitb(&rio, fd);
 
  req = read_request(&rio, addr, addrlen);
  if(!req)
  {
    fprintf(stderr,
        "ERROR: process_request: error reading request\n");
    r = 1;
    goto EXIT_process_request;
  }
  /* CLEANUP_process_request_0 */

  r = response(fd, req) < 100;

  CLEANUP_process_request_0:
  {
    free_request(req);
  }
  EXIT_process_request:
    exit(r);
}

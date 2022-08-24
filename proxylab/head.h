#ifndef __PROXY_HEAD_HDR_
#define __PROXY_HEAD_HDR_

#include "csapp.h"

/* Mutable constants */
#define LINE_BUFSIZ       BUFSIZ
#define HEADER_BUFSIZ     LINE_BUFSIZ
#define HEADER_EXCLUSIVE  (0)
#define HEADER_OVERRIDE   (1)
#define HEADER_MULTIPLE   (2)

/* Predefined headers */
#define USER_AGENT        "Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3)" \
                          " Gecko/20120305 Firefox/10.0.3"
#define CONNECTION        "close"
#define PROTOCOL          "HTTP/1.0"

char *readlineb_g(rio_t *rio, size_t *plen);

struct header {
  char          *key;
  char          *value;
  struct header *next;
};

struct header *read_headers(rio_t *rio, int *rst);
int add_header(struct header **phdr,
    const char *key, const char *value, int flag);
int remove_header(struct header **phdr, const char *key);
struct header **find_header(struct header *const *phdr, const char *key);
char *get_header(const struct header *hdr, const char *key);
int write_headers(int fd, const struct header *hdr);
void free_headers(struct header *hdr);

struct head {
  char          *f1;
  char          *f2;
  char          *f3;
  struct header *hdr;
};

struct req_head {
  char          *mtd;
  char          *pth;
  char          *ptc;
  struct header *hdr;
};

struct res_head {
  char          *ptc;
  char          *cod;
  char          *msg;
  struct header *hdr;
};

struct head *read_head(rio_t *rio);
void set_headline(struct head *hd,
    const char *f1, const char *f2, const char *f3);
char *get_path_host(const char *path, const char **pnext);
void remove_path_host(char *path, const char *next);
int write_head(int fd, const struct head *hd);
void free_head(struct head *hd);

inline int write_req_head(int fd, const struct req_head *hd)
{
  return write_head(fd, (struct head *)hd);
}

inline int write_res_head(int fd, const struct res_head *hd)
{
  return write_head(fd, (struct head *)hd);
}

inline void free_req_head(struct req_head *hd)
{
  return free_head((struct head *)hd);
}

inline void free_res_head(struct res_head *hd)
{
  return free_head((struct head *)hd);
}

#endif  /* __PROXY_HEAD_HDR_ */

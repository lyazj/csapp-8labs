#ifndef __PROXY_BODY_HDR_
#define __PROXY_BODY_HDR_

#include "csapp.h"
#include "head.h"

int is_body_chunked(const struct head *hd);
int get_body_size(const struct head *hd, size_t *psz);

struct body {
  char    *buf;
  size_t  bufsiz;
};

struct body *read_body_len(rio_t *rio, size_t len);
struct body *read_body_chk(rio_t *rio);
struct body *read_body(rio_t *rio, const struct head *hd);
int write_body(int fd, const struct body *bd);
void free_body(struct body *bd);

int read_chunk_head(rio_t *rio, size_t *plen);
char *read_chunk_body(rio_t *rio, size_t len);
char *read_chunks(rio_t *rio, size_t *plen);

#endif  /* __PROXY_BODY_HDR_ */

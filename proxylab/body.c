#include "body.h"

int is_body_chunked(const struct head *hd)
{
  const char *transfer_encoding = get_header(hd->hdr, "Transfer-Encoding");
  if(!transfer_encoding)
    return 0;
  return !strcmp(transfer_encoding, "chunked");
}

int get_body_size(const struct head *hd, size_t *psz)
{
  const char *content_length = get_header(hd->hdr, "Content-Length");
  if(!content_length)
    return -1;  /* error */
  if(sscanf(content_length, " %zu", psz) != 1)
    return -1;
  return 0;  /* success */
}

struct body *read_body_len(rio_t *rio, size_t len)
{
  ssize_t r;
  char *buf;
  struct body *bd;

  /* len too large */
  if(len >> (((sizeof len) << 3) - 1))
  {
    fprintf(stderr, "ERROR: read_body_len: body too large\n");
    return NULL;
  }

  buf = Malloc(len);
  r = rio_readnb(rio, buf, len);
  if(r < (ssize_t)len)
  {
    Free(buf);
    perror("ERROR: read_body: rio_readnb");
    return NULL;
  }

  bd = Malloc(sizeof *bd);
  bd->buf = buf;
  bd->bufsiz = len;
  return bd;
}

int read_chunk_head(rio_t *rio, size_t *plen)
{
  char *buf = readlineb_g(rio, NULL);
  if(!buf)  /* EOF or error */
  {
    fprintf(stderr, "ERROR: read_chunk_head: readlineb_g returned NULL\n");
    return -1;  /* failure */
  }
  /* CLEANUP: buf */

  if(sscanf(buf, "%zx", plen) != 1)
  {
    fprintf(stderr, "ERROR: read_chunk_head: invalid chunk head\n");
    Free(buf);
    return -1;  /* failure */
  }

  Free(buf);
  return 0;  /* success */
}

char *read_chunk_body(rio_t *rio, size_t len)
{
  ssize_t r;
  char *buf;
  char *lbuf;

  if(len >> 31)
  {
    fprintf(stderr, "ERROR: read_chunk_body: chunk too large\n");
    return NULL;
  }
  buf = Malloc(len);
  /* CLEANUP: buf */

  r = rio_readnb(rio, buf, len);
  if(r < 0)
  {
    perror("ERROR: read_chunk_body");
    Free(buf);
    return NULL;
  }
  if((size_t)r < len)
  {
    fprintf(stderr, "ERROR: read_chunk_body: unexpected EOF\n");
    Free(buf);
    return NULL;
  }

  lbuf = readlineb_g(rio, NULL);
  /* CLEANUP: lbuf */

  if(strcmp(lbuf, "\r\n"))
  {
    fprintf(stderr, "ERROR: read_chunk_body: more bytes than expected\n");
    Free(lbuf);
    Free(buf);
    return NULL;
  }
  Free(lbuf);
  /* NOCLEANUP: lbuf */

  return buf;
}

char *read_chunks(rio_t *rio, size_t *plen)
{
  size_t len, bufsiz = 0;
  char *buf = Malloc(bufsiz);
  /* CLEANUP: buf */

  for(;;)
  {
    char *cbuf;
    size_t sumsiz;

    if(read_chunk_head(rio, &len))
    {
      fprintf(stderr, "ERROR: read_chunks: error reading chunk head\n");
      Free(buf);
      return NULL;
    }

    if(!len)
    {
      fprintf(stderr, "INFO: chunks ended with size 0\n");
      break;
    }
    fprintf(stderr, "INFO: chunk size: %zu\n", len);
    
    cbuf = read_chunk_body(rio, len);
    if(!cbuf)
    {
      fprintf(stderr,
          "ERROR: read_chunks: read_chunk_body returned NULL\n");
      Free(buf);
      return NULL;
    }
    /* CLEANUP: cbuf */

    sumsiz = bufsiz + len;
    buf = Realloc(buf, sumsiz);
    memcpy(buf + bufsiz, cbuf, len);
    bufsiz = sumsiz;
    Free(cbuf);
    /* NOCLEANUP: cbuf */
  }

  if(plen)
    *plen = bufsiz;
  return buf;
}

struct body *read_body_chk(rio_t *rio)
{
  char *buf;
  size_t bufsiz;

  buf = read_chunks(rio, &bufsiz);
  if(!buf)
  {
    fprintf(stderr, "ERROR: read_body_chk: error reading chunks\n");
    return NULL;
  }
  /* CLEANUP: buf */

  struct body *bd = Malloc(sizeof *bd);
  bd->buf = buf;
  bd->bufsiz = bufsiz;
  return bd;
}

struct body *read_body(rio_t *rio, const struct head *hd)
{
  size_t len;

  if(is_body_chunked(hd))
  {
    fprintf(stderr, "INFO: transferring chunked body...\n");
    return read_body_chk(rio);
  }
  if(get_body_size(hd, &len))
  {
    fprintf(stderr, "INFO: no body size information detacted\n");
    len = 0;
  }
  else
    fprintf(stderr, "INFO: body size: %zu\n", len);
  return read_body_len(rio, len);
}

void free_body(struct body *bd)
{
  Free(bd->buf);
  Free(bd);
}

int write_body(int fd, const struct body *bd)
{
  if(rio_writen(fd, bd->buf, bd->bufsiz) < 0)
  {
    perror("ERROR: write_body: rio_writen");
    return -1;  /* failure; partially written possible */
  }
  return 0;  /* success */
}

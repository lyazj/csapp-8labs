#include "head.h"

/* Guaranteed to read a line terminated by '\n',
 *     return a string allocated by calling malloc() on success,
 *     or NULL on failure, free() calling is needed by caller
 */
char *readlineb_g(rio_t *rio, size_t *plen)
{
  /* initialize buffer */
  size_t bufsiz = LINE_BUFSIZ;
  char *buf = Malloc(bufsiz);

  /* initialize cursor position and size afterward */
  char *cur = buf;
  size_t bufrem = bufsiz;
  size_t bufuse;

  /* receive return value from rio functions */
  ssize_t r;

  for(;;)
  {
    r = rio_readlineb(rio, cur, bufrem);
    if(r <= 0)  /* error or EOF */
    {
      if(r < 0)
        perror("ERROR: readlineb_g: rio_readlineb");
      else
        fprintf(stderr,
            "ERROR: readlineb_g: rio_readlineb: unexpected EOF\n");
      Free(buf);
      return NULL;
    }

    /* check if we run out of the buffer but no '\n' was caught */
    if((size_t)r == bufrem - 1 && cur[r - 1] != '\n')
    {
      /* stop if the size is too large (this is nearly imposible) */
      if(bufsiz >> (((sizeof bufsiz) << 3) - 1))
      {
        fprintf(stderr, "ERROR: readlineb_g: buffer size too large\n");
        Free(buf);
        return NULL;
      }

      /* double the buffer and proceed to read */
      bufrem = bufsiz + 1;
      bufsiz <<= 1;
      buf = Realloc(buf, bufsiz);
      cur = buf + (bufrem - 2);
      continue;
    }
    break;
  }

  bufuse = bufsiz - bufrem + r;
  buf = Realloc(buf, bufuse + 1);
  if(plen)
    *plen = bufuse;
  return buf;
}

/* Return 1 on success, 0 on EOH, or -1 on error */
static int read_header(rio_t *rio, struct header **phdr)
{
  struct header *hdr;
  size_t len;
  char *buf = readlineb_g(rio, &len), *cur;

  if(!buf)  /* error or EOF */
  {
    fprintf(stderr, "ERROR: read_header: readlineb_g returned NULL\n");
    return -1;
  }
  if(len < 2)  /* too short */
  {
    fprintf(stderr, "ERROR: read_header: line too short\n");
    Free(buf);
    return -1;
  }
  if(!strcmp(buf, "\r\n"))  /* EOH */
  {
    fprintf(stderr, "INFO: EOH reached\n");
    Free(buf);
    return 0;
  }

  cur = strchr(buf, ':');
  if(!cur)
  {
    fprintf(stderr, "ERROR: read_header: delimiter not found\n");
    Free(buf);
    return -1;  /* delimiter not found */
  }
  *cur = 0;
  while(*++cur == ' ');
  if(buf[len - 2] != '\r')  /* not terminated by "\r\n" */
  {
    fprintf(stderr, "ERROR: read_header: not terminated by \"\\r\\n\"\n");
    Free(buf);
    return -1;
  }
  buf[len - 2] = 0;

  hdr = Malloc(sizeof *hdr);
  hdr->key = buf;
  hdr->value = cur;
  *phdr = hdr;
  return 1;  /* success */
}

static void free_header(struct header *hdr)
{
  if(hdr)
  {
    Free(hdr->key);
    Free(hdr);
  }
}

void free_headers(struct header *hdr)
{
  if(hdr)
  {
    free_headers(hdr->next);
    Free(hdr->key);
    Free(hdr);
  }
}

/* On success, rst is set as 0, otherwise -1;
 *     if no header is read before the terminator "\r\n",
 *     returning NULL, but it does NOT mean a failure
 */
struct header *read_headers(rio_t *rio, int *rst)
{
  int r;
  struct header *hdr, **pcur = &hdr;

  fputs("INFO: reading headers...\n", stderr);
  for(;;)
  {
    r = read_header(rio, pcur);
    if(r == 0)  /* EOH */
    {
      *pcur = NULL;
      if(rst)
        *rst = 0;  /* success */
      fprintf(stderr, "INFO: head reading terminated at EOH\n");
      return hdr;
    }
    if(r < 0)  /* error */
    {
      *pcur = NULL;
      break;
    }
    fprintf(stderr, "INFO: got header {%s: %s}\n",
        (*pcur)->key, (*pcur)->value);
    pcur = &(*pcur)->next;
  }

  if(rst)
    *rst = -1;  /* failure */
  free_headers(hdr);
  return NULL;
}

struct header **find_header(struct header *const *phdr,
    const char *key)
{
  while(*phdr && strcmp(key, (*phdr)->key))
    phdr = &(*phdr)->next;
  return (struct header **)phdr;
}

char *get_header(const struct header *hdr,
    const char *key)
{
  hdr = *find_header((struct header **)&hdr, key);
  return hdr ? hdr->value : NULL;
}

int add_header(struct header **phdr,
    const char *key, const char *value, int flag)
{
  struct header *hdr;
  size_t key_len = strlen(key);
  size_t value_len = strlen(value);

  phdr = find_header(phdr, key);
  if(*phdr)
  {
    switch(flag)
    {
    default:
      fprintf(stderr, "ERROR: add_header: invalid flag\n");
      return -1;
    case HEADER_EXCLUSIVE:
      return -1;
    case HEADER_OVERRIDE:
      Free((*phdr)->key);
      break;
    case HEADER_MULTIPLE:
      do
        phdr = &(*phdr)->next;
      while(*phdr && !strcmp((*phdr)->key, key));
      hdr = Malloc(sizeof *hdr);
      hdr->next = *phdr;
      *phdr = hdr;
      break;
    }
  }
  else
  {
    *phdr = Malloc(sizeof **phdr);
    (*phdr)->next = NULL;
  }
  hdr = *phdr;

  hdr->key = Malloc(key_len + value_len + 2);
  strcpy(hdr->key, key);
  hdr->value = hdr->key + key_len + 1;
  strcpy(hdr->value, value);
  return 0;  /* success */
}

int remove_header(struct header **phdr,
    const char *key)
{
  struct header *hdr;

  phdr = find_header(phdr, key);
  hdr = *phdr;
  if(!hdr)
    return -1;  /* not found */

  *phdr = hdr->next;
  free_header(hdr);
  return 0;  /* success */
}

int write_headers(int fd, const struct header *hdr)
{
  if(hdr)
  {
    {
      size_t key_len = strlen(hdr->key);
      size_t value_len = strlen(hdr->value);
      size_t len = key_len + value_len + 4;
      char *buf = Malloc(len);

      sprintf(buf, "%s: %s\r", hdr->key, hdr->value);
      buf[len - 1] = '\n';
      if(rio_writen(fd, buf, len) < 0)
      {
        perror("ERROR: write_headers: rio_writen");
        Free(buf);
        return -1;  /* failure; partially written possible */
      }
      Free(buf);
    }
    return write_headers(fd, hdr->next);
  }
  if(rio_writen(fd, "\r\n", 2) < 0)
  {
    perror("ERROR: write_headers: rio_writen");
    return -1;  /* failure; partially written possible */
  }
  return 0;  /* success */
}

struct head *read_head(rio_t *rio)
{
  struct head *hd;
  struct header *hdr;
  char *ptr[3], *buf, *cur;
  int r;

  fputs("INFO: reading head...\n", stderr);
  buf = readlineb_g(rio, NULL);
  if(!buf)  /* error or EOF */
  {
    fprintf(stderr, "ERROR: read_head: readlineb_g returned NULL\n");
    return NULL;
  }
  cur = buf;

  for(int i = 0; i < 2; ++i)
  {
    register char *c = strchr(cur, ' ');
    if(!c)  /* fewer delimiters than expected */
    {
      fprintf(stderr,
          "ERROR: read_head: fewer delimiters than expected\n");
      Free(buf);
      return NULL;
    }
    *c = 0;
    ptr[i] = cur;
    while(*++c == ' ');
    cur = c;
  }
  ptr[2] = cur;
  for(;;) switch(*cur)
  {
  case '\r':
    *cur = 0;
    if(cur[1] == '\n')
      goto LBE_read_head_0;
    /* fall through */
  case 0:  /* not terminated by "\r\n" */
    fprintf(stderr, "ERROR: read_head: not terminated by \"\\r\\n\"\n");
    Free(buf);
    return NULL;
  default:
    ++cur;
    break;
  }
  LBE_read_head_0:
  {
    fprintf(stderr, "INFO: got message line field 1: %s\n", ptr[0]);
    fprintf(stderr, "INFO: got message line field 2: %s\n", ptr[1]);
    fprintf(stderr, "INFO: got message line field 3: %s\n", ptr[2]);
  }

  hdr = read_headers(rio, &r);
  if(r)
  {
    Free(buf);
    return NULL;
  }

  hd = Malloc(sizeof *hd);
  hd->f1 = ptr[0];  /* buf */
  hd->f2 = ptr[1];
  hd->f3 = ptr[2];
  hd->hdr = hdr;
  return hd;  /* success */
}

void free_head(struct head *hd)
{
  Free(hd->f1);
  free_headers(hd->hdr);
  Free(hd);
}

int write_head(int fd, const struct head *hd)
{
  size_t l1 = strlen(hd->f1);
  size_t l2 = strlen(hd->f2);
  size_t l3 = strlen(hd->f3);
  size_t len = l1 + l2 + l3 + 4;
  char *buf = Malloc(len);

  sprintf(buf, "%s %s %s\r", hd->f1, hd->f2, hd->f3);
  buf[len - 1] = '\n';
  if(rio_writen(fd, buf, len) < 0)
  {
    Free(buf);
    perror("ERROR: write_head: rio_writen");
    return -1;
  }
  Free(buf);
  return write_headers(fd, hd->hdr);
}

/* Free needed after using */
char *get_path_host(const char *path, const char **pnext)
{
  const char *beg = strstr(path, "//"), *end;
  char *host;
  size_t n;
  if(!beg)
    return NULL;
  beg += 2;

  end = beg;
  while(*end && *end != '/')
    ++end;
  n = end - beg;
  if(pnext)
    *pnext = end;

  host = Malloc(n + 1);
  strncpy(host, beg, n);
  host[n] = 0;
  return host;
}

void remove_path_host(char *path, const char *next)
{
  if(!next)
  {
    char *host = get_path_host(path, &next);
    if(!host)
      return;
    Free(host);
  }
  if(!*next)
  {
    strcpy(path, "/");
    return;
  }
  while(*next)
    *path++ = *next++;
  *path = 0;
}

static void generate_headline(
    const char *f1, const char *f2, const char *f3,
    char **pf1, char **pf2, char **pf3)
{
  const char *const str[3] = {f1, f2, f3};
  char **const pstr[3] = {pf1, pf2, pf3};
  size_t len[4];
  char *buf;

  len[0] = 0;
  for(register int i = 0; i < 3; ++i)
    len[i + 1] = len[i] + strlen(str[i]) + 1;
  buf = Malloc(len[3]);
  for(register int i = 0; i < 3; ++i)
    strcpy(*pstr[i] = buf + len[i], str[i]);
}

void set_headline(struct head *hd,
    const char *f1, const char *f2, const char *f3)
{
  Free(hd->f1);
  generate_headline(f1, f2, f3, &hd->f1, &hd->f2, &hd->f3);
}

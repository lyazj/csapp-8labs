#include "csapp.h"
#include "response.h"

const char *get_html_head(void)
{
  return
    "<!DOCTYPE html>"
    "<html lang=\"en-us\">"
    "<head>"
    "<meta charset=\"utf-8\" />"
    "</head>"
    "<body>"
  ;
}

const char *get_html_tail(void)
{
  return 
    "</body>"
    "</html>"
  ;
}

const char *get_date(void)
{
  static char buf[64];
  time_t now = time(NULL);
  memset(buf, 0, sizeof buf);
  strftime(buf, sizeof buf - 1,
      "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
  return buf;
}

const char *get_filename_extension(const char *filename)
{
  const char *pdot = strrchr(filename, '.');
  if(!pdot)
    return NULL;
  return pdot + 1;
}

const char *get_mime(const char *ext)
{
  static const char *mime[] = {
    "html", "text/html; charset=utf-8",
    "txt",  "text/plain; charset=utf-8",
    "c",    "text/plain; charset=utf-8",
    "h",    "text/plain; charset=utf-8",
    "gif",  "image/gif",
    "png",  "image/png",
    "jpg",  "image/jpeg",
    "jpe",  "image/jpeg",
    "jpeg", "image/jpeg",
    NULL,   MIME_UNKNOWN,
  };
  register const char **p;

  if(!ext)
    return MIME_UNKNOWN;
  for(p = mime; *p; p += 2)
    if(!strcmp(ext, *p))
      break;
  return p[1];
}

struct message *generate_res_message(int code, const char *code_msg)
{
  const char *ptc = PROTOCOL;
  char cod[16];
  struct message *msg;

  sprintf(cod, "%d", code);

  msg = Malloc(sizeof *msg);
  msg->hd = Malloc(sizeof *msg->hd);
  msg->hd->f1 = NULL;
  set_headline(msg->hd, ptc, cod, code_msg);

  msg->hd->hdr = NULL;
  add_header(&msg->hd->hdr, "Content-Length", "0", HEADER_OVERRIDE);
  add_header(&msg->hd->hdr, "Content-Type", "text/html; charset=utf-8", HEADER_OVERRIDE);
  add_header(&msg->hd->hdr, "Connection", "close", HEADER_OVERRIDE);
  add_header(&msg->hd->hdr, "Date", get_date(), HEADER_OVERRIDE);
  add_header(&msg->hd->hdr, "Server", "lyazj's proxy@5861", HEADER_OVERRIDE);

  msg->bd = Malloc(sizeof *msg->bd);
  msg->bd->buf = NULL;
  msg->bd->bufsiz = 0;

  return msg;
}

int response_error(int fd, int code, const char *code_msg)
{
  int r;
  struct message *msg = generate_res_message(code, code_msg);

  msg->bd->buf = Malloc(strlen(code_msg) + HTML_RESERVED_SIZE);
  strcpy(msg->bd->buf, get_html_head());
  strcat(msg->bd->buf, "<h1>");
  strcat(msg->bd->buf, msg->hd->f2);
  strcat(msg->bd->buf, " ");
  strcat(msg->bd->buf, msg->hd->f3);
  strcat(msg->bd->buf, "</h1>");
  strcat(msg->bd->buf, get_html_tail());
  msg->bd->bufsiz = strlen(msg->bd->buf);

  r = write_message(fd, msg);
  free_message(msg);
  if(r)
    return -1;
  return code;
}

int response_file(int fd, const char *fn, int code, const char *code_msg)
{
  const char *fn_ext, *mime;
  char filename[FILENAME_MAX];
  int fd0;
  struct stat sbuf;
  char *buf;
  size_t bufsiz;
  ssize_t r;
  struct message *msg;

  if(strlen(fn) + 5 >= FILENAME_MAX)
    return response_error(fd, 404, "Not found");
  if(strstr(fn, ".."))
    return response_error(fd, 400, "Bad request");
  sprintf(filename, "tiny/%s", fn);

  if(stat(filename, &sbuf) < 0)
  {
    if(errno == ENOENT)  /* not exist */
      return response_error(fd, 404, "Not found");
    else
    {
      perror("ERROR: response_file");
      return response_error(fd, 500, "Internal server error");
    }
  }
  if(!S_ISREG(sbuf.st_mode))
    return response_error(fd, 403, "Forbidden");

  fd0 = open(filename, O_RDONLY, 0);
  if(fd0 < 0)
  {
    if(errno == EACCES)
      return response_error(fd, 403, "Forbidden");
    else
    {
      perror("ERROR: response_file");
      return response_error(fd, 500, "Internal server error");
    }
  }

  bufsiz = sbuf.st_size;
  buf = Malloc(bufsiz);
  r = rio_readn(fd0, buf, bufsiz);
  close(fd0);

  /* I believe that (bufsiz & (1 << 63) == 0) is true */
  if(r < (ssize_t)bufsiz)
  {
    perror("ERROR: response_file: rio_readn");
    Free(buf);
    return response_error(fd, 500, "Internal server error");
  }

  msg = generate_res_message(code, code_msg);
  msg->bd->buf = buf;
  msg->bd->bufsiz = bufsiz;

  fn_ext = get_filename_extension(fn);
  mime = get_mime(fn_ext);
  add_header(&msg->hd->hdr, "Content-Type", mime, HEADER_OVERRIDE);

  r = write_message(fd, msg);
  free_message(msg);
  if(r)
    return -1;
  return code;
}

int response_local(int fd, struct request *req)
{
  int r;

  if(strcmp(req->msg->hd->f1, "GET"))
    r = response_error(fd, 501, "Not implemented");
  else
    r = response_file(fd, req->msg->hd->f2, 200, "OK");
  if(r == -1)
    fprintf(stderr, "ERROR: response_error: error writing message\n");
  else
    fprintf(stderr, "INFO: response %d\n", r);
  return r;
}

int response_upward(int fd, struct request *req)
{
  int rd;
  struct req_head *hd = (struct req_head *)req->msg->hd;
  rio_t rio;
  struct message *msg;
  int status;
  
  add_header(&hd->hdr, "User-Agent", USER_AGENT, HEADER_OVERRIDE);
  add_header(&hd->hdr, "Connection", CONNECTION, HEADER_OVERRIDE);
  add_header(&hd->hdr, "Proxy-Connection", CONNECTION, HEADER_OVERRIDE);

  fprintf(stderr, "INFO: forwarding resquest to %s%s%s...\n",
      req->d_host, *req->d_port ? ":" : "", *req->d_port ? req->d_port : "");
  rd = send_request(req);
  if(rd < 0)
  {
    fprintf(stderr, "ERROR: response_upward: error forwarding request\n");
    return response_error(fd, 502, "Bad gateway");
  }
  /* CLEANUP: rd */

  rio_readinitb(&rio, rd);
  msg = read_message(&rio);
  if(!msg)
  {
    fprintf(stderr,
        "ERROR: response_upward: error reading response upward\n");
    if(close(rd))
      perror("ERROR: response_upward: close");
    return response_error(fd, 502, "Bad gateway");
  }
  /* CLEANUP: msg */

  if(close(rd))
    perror("ERROR: response_upward: close");
  /* NOCLEANUP: rd */

  fprintf(stderr, "INFO: forwarding response from %s%s%s...\n",
      req->d_host, *req->d_port ? ":" : "", *req->d_port ? req->d_port : "");
  if(write_message(fd, msg))
  {
    fprintf(stderr,
        "ERROR: response_upward: error forwarding response upward\n");
    free_message(msg);
    /* NOCLEANUP: msg */
    return -1;
  }

  status = atoi(msg->hd->f2);
  free_message(msg);
  /* NOCLEANUP: msg */

  if(!status)
    fprintf(stderr, "ERROR: error getting response status code upward\n");
  else
    fprintf(stderr, "INFO: response %d\n", status);
  return status;
}

int response(int fd, struct request *req)
{
  if(is_req_local(req))
    return response_local(fd, req);
  return response_upward(fd, req);
}

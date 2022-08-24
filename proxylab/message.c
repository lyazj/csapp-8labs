#include "message.h"

struct message *read_message(rio_t *rio)
{
  struct head *hd;
  struct body *bd;
  struct message *msg;

  hd = read_head(rio);
  if(!hd)
  {
    fprintf(stderr, "ERROR: read_message: error reading head\n");
    return NULL;
  }
  /* CLEANUP_read_message_0 */

  bd = read_body(rio, hd);
  if(!bd)
  {
    fprintf(stderr, "ERROR: read_message: error reading body\n");
    goto CLEANUP_read_message_0;
  }

  msg = Malloc(sizeof *msg);
  msg->hd = hd; msg->bd = bd;
  return msg;

  CLEANUP_read_message_0:
  {
    free_head(hd);
    return NULL;
  }
}

void free_message(struct message *msg)
{
  free_head(msg->hd);
  free_body(msg->bd);
  Free(msg);
}

int write_message(int fd, const struct message *msg)
{
  char buf[64];
  sprintf(buf, "%zd", msg->bd->bufsiz);
  remove_header(&msg->hd->hdr, "Transfer-Encoding");
  add_header(&msg->hd->hdr, "Content-Length", buf, HEADER_OVERRIDE);

  if(write_head(fd, msg->hd))
  {
    fprintf(stderr, "ERROR: write_message: error writing head\n");
    return -1;
  }
  if(write_body(fd, msg->bd))
  {
    fprintf(stderr, "ERROR: write_message: error writing body\n");
    return -1;
  }
  return 0;
}

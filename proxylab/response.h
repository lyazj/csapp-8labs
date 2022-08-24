#ifndef __PROXY_RESPONSE_HDR_
#define __PROXY_RESPONSE_HDR_

#include "csapp.h"
#include "request.h"

#define HTML_RESERVED_SIZE  (1024)
#define MIME_UNKNOWN        "application/octet-stream"

const char *get_html_head(void);
const char *get_html_tail(void);
const char *get_date(void);
const char *get_filename_extension(const char *filename);
const char *get_mime(const char *ext);

struct message *generate_res_message(int code, const char *code_msg);
int response_error(int fd, int code, const char *message);
int response_file(int fd, const char *fn, int code, const char *code_msg);

int response_local(int fd, struct request *req);
int response_upward(int fd, struct request *req);
int response(int fd, struct request *req);

#endif /* __PROXY_RESPONSE_HDR_ */

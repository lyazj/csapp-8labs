#include "csapp.h"
#include "request.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

void sigchld_handler(/* int signum */)
{
  int _errno = errno;
  int pid;

  for(;;)
  {
    pid = waitpid(-1, NULL, WNOHANG);
    if(pid <= 0)
    {
      if(pid < 0 && errno != ECHILD)
      {
        perror("ERROR: sigchld_handler");
        fflush(stderr);
        abort();  /* for better debugging */
      }
      break;
    }
    fprintf(stderr, "INFO: child process %d reaped\n", pid);
  }

  errno = _errno;
}

int main(int argc, char *argv[])
{
  char *port;
  int fd, cd;

  if(argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    return 1;
  }

  Signal(SIGPIPE, SIG_IGN);
  Signal(SIGCHLD, sigchld_handler);

  port = argv[1];
  fd = Open_listenfd(port);

  for(;;)
  {
    struct sockaddr addr;
    socklen_t addrlen = sizeof addr;

    fprintf(stderr, "INFO: waiting for request...\n");
    cd = accept(fd, &addr, &addrlen);
    if(cd < 0)
    {
      perror("ERROR: main: accept");
      continue;
    }

    process_request(cd, &addr, addrlen);
    if(close(cd))
      perror("ERROR: main: close");
  }

  Close(fd);
  return 0;
}

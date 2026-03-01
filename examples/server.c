#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cipc.h"

int main(void) {
  printf("hello\n");

  int pid = 0; // getpid()
  cipc_endpoint endpoint = { "hello", pid };
  cipc_listener *listener = cipc_create_listener(endpoint, cipc_flag_nonblocking);
  if (!listener) {
    return 1;
  }

  printf("waiting for client!\n");


  cipc_stream *stream = NULL;
  while (stream == NULL) {
    stream = cipc_accept(listener, cipc_flag_nonblocking);
    sleep(1);
  }
  printf("accepted client!\n");
  char msg[32];
  memset(msg, 0, sizeof(msg));
  if (cipc_read(stream, (uint8_t*)msg, sizeof(msg)) > 0) {
    msg[sizeof(msg) - 1] = 0;
    printf("received: %s\n", msg);
  } else {
    perror("read");
  }

  cipc_destroy_listener(listener);

  return 0;
}


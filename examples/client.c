#include <stdio.h>
#include <unistd.h>

#include "cipc.h"

int main(void) {
  printf("hello\n");

  int pid = 0; // getpid()
  cipc_endpoint endpoint = { "hello", pid };
  cipc_stream *stream= cipc_connect(endpoint, cipc_flags_none);
  if (!stream) {
    return 1;
  }

  printf("connected to server!\n");
  const char msg[] = "what's up";
  cipc_write(stream, (uint8_t*)msg, sizeof(msg));

  cipc_close(stream);

  return 0;
}


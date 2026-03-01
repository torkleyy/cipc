#ifndef CIPC_H
#define CIPC_H

#include <stddef.h>
#include <stdint.h>

typedef struct cipc_listener cipc_listener;

typedef struct cipc_endpoint {
  /**
   * Unique identifier of the endpoint.
  */
  const char *name;
  /**
   * The id of the process that owns the endpoint; "service" endpoints
   * use pid=0 because they are served by a singleton process.
  */
  int pid;
} cipc_endpoint;

typedef enum cipc_flags {
  cipc_flags_none = 0,
  cipc_flag_nonblocking = 0x1,
} cipc_flags;

cipc_listener* cipc_create_listener(cipc_endpoint endpoint, cipc_flags flags);
void cipc_destroy_listener(cipc_listener* listener);

typedef struct cipc_stream cipc_stream;

cipc_stream* cipc_accept(cipc_listener* listener, cipc_flags flags);
cipc_stream* cipc_connect(cipc_endpoint endpoint, cipc_flags flags);

long cipc_read(cipc_stream *stream, uint8_t *buf, size_t len);
long cipc_write(cipc_stream *stream, const uint8_t *buf, size_t len);

void cipc_close(cipc_stream *stream);

#endif

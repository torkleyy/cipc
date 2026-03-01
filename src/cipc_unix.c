#include "cipc.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static int fmt_sockaddr(struct sockaddr_un *addr, cipc_endpoint e) {
  memset(addr, 0, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;

  int ret = snprintf(addr->sun_path, sizeof(addr->sun_path), "/tmp/ipc_%d_%s",
                     e.pid, e.name);
  if (ret >= (int)sizeof(addr->sun_path)) {
    perror("file name too long");
    return 0;
  }

  return 1;
}

static int bind_force_stale(int fd, const struct sockaddr_un *addr) {
  const struct sockaddr *saddr = (const struct sockaddr *)addr;
  const size_t addrsize = sizeof(struct sockaddr_un);

  for (int attempt = 0; attempt < 3; ++attempt) {
    int ret = bind(fd, saddr, addrsize);
    if (ret == 0)
      return 1; // success

    if (errno != EADDRINUSE) {
      // we don't expect to recover from any other error
      perror("bind");
      return 0;
    }

    // socket file already exists, let's check if it's a corpse
    int probe = socket(PF_LOCAL, SOCK_STREAM, 0);
    int connect_ret = connect(probe, saddr, addrsize);
    if (connect_ret == 0) {
      perror("service appears to be running already");
      close(probe);
      return 0;
    }

    // socket file exists, but appears stale
    // NOTE: it is possible for a socket to bind to the file,
    // but refuse the connection for other reasons.
    // These include: listen was not called yet,
    // kernel accept queue is full, socket is dgram
    // TODO: is this approach too aggressive?
    unlink(addr->sun_path);
    close(probe);
  }

  fprintf(stderr,
          "looks like multiple processes are racing for this socket file\n");
  return 0;
}

struct cipc_listener {
  int fd;
  char file[128];
};

cipc_listener *cipc_create_listener(cipc_endpoint endpoint, cipc_flags flags) {
  struct sockaddr_un addr;
  if (!fmt_sockaddr(&addr, endpoint)) {
    return NULL;
  }

  int fd = socket(PF_LOCAL, SOCK_STREAM, 0);
  if (fd == -1)
    return NULL;

  if (!bind_force_stale(fd, &addr)) {
    close(fd);
    return NULL;
  }

  int ret = listen(fd, 16);
  if (ret == -1) {
    perror("listen");
    close(fd);
    return NULL;
  }

  if (flags & cipc_flag_nonblocking) {
    fcntl(fd, F_SETFL, O_NONBLOCK);
  }

  cipc_listener *listener = malloc(sizeof(cipc_listener));
  if (!listener) {
    close(fd);
    return NULL;
  }

  listener->fd = fd;
  memset(listener->file, 0, sizeof(listener->file));
  memcpy(listener->file, addr.sun_path, sizeof(addr.sun_path));

  return listener;
}

void cipc_destroy_listener(cipc_listener *listener) {
  close(listener->fd);
  unlink(listener->file);

  free(listener);
}

struct cipc_stream {
  int fd;
};

cipc_stream *cipc_accept(cipc_listener *listener, cipc_flags flags) {
  struct sockaddr addr;
  socklen_t addrlen;
  int fd = accept(listener->fd, &addr, &addrlen);
  if (fd == -1) {
    perror("accept");
    return NULL;
  }

  if (flags & cipc_flag_nonblocking) {
    fcntl(fd, F_SETFL, O_NONBLOCK);
  }

  cipc_stream *stream = malloc(sizeof(cipc_stream));
  stream->fd = fd;

  return stream;
}

cipc_stream *cipc_connect(cipc_endpoint endpoint, cipc_flags flags) {
  struct sockaddr_un addr;
  if (!fmt_sockaddr(&addr, endpoint)) {
    return NULL;
  }
  int fd = socket(PF_LOCAL, SOCK_STREAM, 0);
  if (fd == -1)
    return NULL;
  int ret = connect(fd, (struct sockaddr *)&addr, sizeof(addr));

  if (ret == -1) {
    perror("connect");
    close(fd);
    return NULL;
  }

  if (flags & cipc_flag_nonblocking) {
    fcntl(fd, F_SETFL, O_NONBLOCK);
  }

  cipc_stream *stream = malloc(sizeof(cipc_stream));
  stream->fd = fd;

  return stream;
}

long cipc_read(cipc_stream *stream, uint8_t *buf, size_t len) {
  return read(stream->fd, buf, len);
}

long cipc_write(cipc_stream *stream, const uint8_t *buf, size_t len) {
  return write(stream->fd, buf, len);
}

void cipc_close(cipc_stream *stream) {
  close(stream->fd);
  free(stream);
}

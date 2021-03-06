#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/epoll.h>

#include "buffer.h"
#include "vector.h"
#include "core.h"

static __thread core core_default = {0};

static core_status core_default_callback(core_event *);
static core_handler core_handler_default = {.callback = core_default_callback};

static core_status core_default_callback(core_event *event __attribute__((unused)))
{
  return CORE_OK;
}

static core *core_get(core *core)
{
  return core ? core : &core_default;
}

void core_construct(core *core)
{
  if (core)
    *core = (struct core) {0};

  core = core_get(core);
  if (!core->ref)
    {
      vector_construct(&core->handlers, sizeof(core_handler));
      vector_construct(&core->next, sizeof(core_handler));
      core->fd = epoll_create1(EPOLL_CLOEXEC);
      if (core->fd == -1)
        core->errors ++;
    }
  core->ref ++;
}

void core_destruct(core *core)
{
  core = core_get(core);
  core->ref --;
  if (!core->ref)
    {
      if (core->fd >= 0)
        (void) close(core->fd);
      vector_destruct(&core->handlers, NULL);
      vector_destruct(&core->next, NULL);
      *core = (struct core) {0};
    }
}

core_status core_dispatch(core_handler *handler, int type, uintptr_t data)
{
  return handler->callback((core_event[]){{.state = handler->state, .type = type, .data = data}});
}

void core_loop(core *core)
{
  struct epoll_event events[CORE_MAX_EVENTS];
  int n, i;

  core = core_get(core);
  while (core->errors == 0 && (core->handlers_active || vector_size(&core->next)))
    {
      for (i = 0; (size_t) i < vector_size(&core->next); i ++)
        (void) core_dispatch(vector_at(&core->next, i), 0, 0);
      vector_clear(&core->next, NULL);

      n = core->handlers_active ? epoll_wait(core->fd, events, CORE_MAX_EVENTS, -1) : 0;
      if (n == -1)
        core->errors ++;

      for (i = 0; i < n; i ++)
        (void) core_dispatch(vector_at(&core->handlers, events[i].data.fd), 0, events[i].events);
    }
}

void core_add(core *core, core_callback *callback, void *state, int fd, int events)
{
  core_handler *handlers;
  int e;

  core = core_get(core);
  while (vector_size(&core->handlers) <= (size_t) fd)
    vector_push_back(&core->handlers, &core_handler_default);

  handlers = vector_data(&core->handlers);
  handlers[fd] = (core_handler) {.callback = callback, .state = state};
  e = epoll_ctl(core->fd, EPOLL_CTL_ADD, fd, (struct epoll_event[]){{.events = events, .data.fd = fd}});
  if (e == -1)
    {
      handlers[fd] = core_handler_default;
      core->errors ++;
    }
  else
    core->handlers_active ++;
}

void core_modify(core *core, int fd, int events)
{
  core_handler *handlers;
  int e;

  core = core_get(core);
  e = epoll_ctl(core->fd, EPOLL_CTL_MOD, fd, (struct epoll_event[]){{.events = events, .data.fd = fd}});
  if (e == -1)
    {
      handlers = vector_data(&core->handlers);
      handlers[fd] = core_handler_default;
      core->errors ++;
    }
}

void core_delete(core *core, int fd)
{
  core_handler *handlers;
  int e;

  core = core_get(core);
  e = epoll_ctl(core->fd, EPOLL_CTL_DEL, fd, NULL);
  if (e == -1)
    core->errors ++;
  handlers = vector_data(&core->handlers);
  handlers[fd] = core_handler_default;
  core->handlers_active --;
}

int core_next(core *core, core_callback *callback, void *state)
{
  core_handler handler = {.callback = callback, .state = state};

  core = core_get(core);
  vector_push_back(&core->next, &handler);
  return vector_size(&core->next);
}

void core_cancel(core *core, int id)
{
  core_handler *handlers;

  core = core_get(core);
  if (id == 0)
    return;

  handlers = vector_data(&core->next);
  handlers[id - 1] = core_handler_default;
}

int core_errors(core *core)
{
  core = core_get(core);
  return core->errors;
}

#ifndef CORE_H_INCLUDED
#define CORE_H_INCLUDED

#define CORE_MAX_EVENTS 16

enum core_status
{
 CORE_OK    = 0,
 CORE_ABORT = -1
};

typedef enum core_status    core_status;
typedef struct core_event   core_event;
typedef core_status         core_callback(core_event *);
typedef struct core_handler core_handler;
typedef struct core         core;

struct core_event
{
  void          *state;
  int            type;
  uintptr_t      data;
};

struct core_handler
{
  core_callback *callback;
  void          *state;
};

struct core
{
  size_t         ref;
  int            fd;
  int            errors;
  vector         handlers;
  size_t         handlers_active;
  vector         next;
};

void        core_construct(core *);
void        core_destruct(core *);
core_status core_dispatch(core_handler *, int, uintptr_t);
void        core_loop(core *);
void        core_add(core *, core_callback *, void *, int, int);
void        core_modify(core *, int, int);
void        core_delete(core *, int);
int         core_next(core *, core_callback *, void *);
void        core_cancel(core *, int);
int         core_errors(core *);

#endif /* CORE_H_INCLUDED */

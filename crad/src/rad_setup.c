#include "rad_setup.h"
#include "rad_conf.h"
#include "rad_types.h"

#include "grid_t.h"
#include "pmap_t.h"

#include "cross_comp.h"

#include "assert.h"
#include "math.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#if RAD_NUM_THREADS > 0
#include "threads.h"
#endif

#if __unix__
#include "dirent.h"
#endif

#define LM_LEVEL 6
#include "logger.h"

#define __min__(X, Y) (((X) < (Y)) ? (X) : (Y))

struct range_st {
  /** @brief Offset */
  int o;
  /** @brief Ending index */
  int e;
  /** @brief Range size */
  int s;
};
typedef struct range_st range_t;

struct worker_st {
  /** @brief Logical range */
  range_t l;
  /** @brief Wealth grid range */
  range_t x;
  /** @brief Radius grid range */
  range_t r;

#if RAD_NUM_THREADS > 0
  /** @brief System thread*/
  thrd_t thread;
#endif /* RAD_NUM_THREADS */
};
typedef struct worker_st worker_t;

struct concurrency_st {
  /** @brief Workers */
  worker_t w[RAD_NUM_THREADS + 1];

  /** @brief Global upper bound for quantity grid */
  double qM;

  /** @brief Global maximum quantity policy buffer */
  double qMbuf;
  /** @brief Global maximum effort policy buffer */
  double sMbuf;
  /** @brief Global maximum value function buffer */
  double vMbuf;
  /** @brief Global accuracy buffer */
  double accbuf;

#if RAD_NUM_THREADS > 0
  /** @brief Mutex */
  mtx_t mtx;
  /** @brief Iteration done condition */
  cnd_t it_done;
  /** @brief Next iteration ready condition */
  cnd_t next_ready;

  /** @brief Iteration done count */
  short it_done_count;
  /** @brief Next iteration ready flag */
  bool is_next_ready;
#endif /* RAD_NUM_THREADS */
};
typedef struct concurrency_st concurrency_t;

struct thread_init_st {
  /** @brief Worker id  */
  int wid;
  /** @brief Global setup data
   * @details Should be immutable */
  const setup_t *u;

  /** @brief Pointer to current value function array
   * @details Mutable. Workers copy their respective parts here */
  double **pv0;

  /** @brief Local quantity grid */
  grid_t qg;
  /** @brief Local objective variables */
  objvar_t ovar;

  /** @brief Local current value function array buffer */
  double *v0buf;
  /** @brief Local average quantity policy */
  double *qpolbuf;
  /** @brief Local effort policy */
  double *spolbuf;

  /** @brief Worker's wealth state index */
  int xi;
  /** @brief Worker's radius state index */
  int ri;
  /** @brief Worker's logical state index */
  int li;

  /** @brief Local maximum quantity policy */
  double qM;
  /** @brief Local maximum effort policy */
  double sM;
  /** @brief Local maximum value function */
  double vM;
  /** @brief Local accuracy */
  double acc;
};
typedef struct thread_init_st thread_init_t;

double linterpV12d(const sol_t *s, short x1, short r1, double xp, double rp,
                   const setup_t *u) {
  short x2 = x1 + 1;
  short r2 = r1 + 1;

  double R1 = s->rg->d[r1];
  double R2 = s->rg->d[r2];
  double Rd = R2 - R1;

  double X1 = s->xg->d[x1];
  double X2 = s->xg->d[x2];
  double Xd = X2 - X1;

  double Y11 = s->v1[x1][r1];
  double Y12 = s->v1[x1][r2];
  double Y21 = s->v1[x2][r1];
  double Y22 = s->v1[x2][r2];
  double Y1d = Y12 - Y11;
  double Y2d = Y22 - Y21;

  double slope1 = Y1d / Rd;
  double Y1 = slope1 * (rp - R1) + Y11;

  double slope2 = Y2d / Rd;
  double Y2 = slope2 * (rp - R1) + Y21;

  double Yd = Y2 - Y1;
  double slope = Yd / Xd;
  double I = slope * (xp - X1) + Y1;

  return I;
}

void calc_indices(thread_init_t *td) {
  td->xi = (td->u->c->w[td->wid].l.o + td->li) / td->u->s->rg->n;
  td->ri = (td->u->c->w[td->wid].l.o + td->li) % td->u->s->rg->n;
}

void init_sovle(thread_init_t *td) {
  td->ovar.s = 0;
  for (td->li = 0; td->li < td->u->c->w[td->wid].l.s; ++td->li) {
    calc_indices(td);

    td->ovar.x = td->u->s->xg->d[td->xi];
    td->ovar.r = td->u->s->rg->d[td->ri];
    td->ovar.q = td->ovar.x / td->ovar.r;
    td->v0buf[td->li] =
        td->u->m->util.fnc(&td->ovar) - td->u->m->cost.fnc(&td->ovar);
#ifdef RAD_DEBUG
    if (td->vM < td->v0buf[td->li])
      td->vM = td->v0buf[td->li];
#endif
  }
}

void step_sovle(thread_init_t *td) {
  double rp = 0, xp = 0, vp = 0, v = 0, diff = 0, u = 0, c = 0;
  short rpli = 0, xpli = 0;

  td->acc = 0;
  td->qM = 0;
  td->sM = 0;
  td->vM = 0;
  for (td->li = 0; td->li < td->u->c->w[td->wid].l.s; ++td->li) {
    calc_indices(td);

    td->ovar.x = td->u->s->xg->d[td->xi];
    td->ovar.r = td->u->s->rg->d[td->ri];
    for (int si = 0; si < td->u->s->sg->n; ++si) {
      td->ovar.s = td->u->s->sg->d[si];
      rp = td->u->m->radt.fnc(&td->ovar);
      rpli = grid_liei(td->u->s->rg, rp);
      td->qg.M = __min__(td->ovar.x / rp, td->u->c->qM);
      grid_calc(&td->qg);
      for (int qi = 0; qi < td->qg.n; ++qi) {
        td->ovar.q = td->qg.d[qi];
        xp = td->u->m->wltt.fnc(&td->ovar);
        xpli = grid_liei(td->u->s->xg, xp);
        vp = linterpV12d(td->u->s, xpli, rpli, xp, rp, td->u);
        u = td->u->m->util.fnc(&td->ovar);
        c = td->u->m->cost.fnc(&td->ovar);
        v = u - c + td->u->m->beta * vp;
        // Find maximum
        if ((qi == 0 && si == 0) || td->v0buf[td->li] < v) {
          td->v0buf[td->li] = v;
          td->qpolbuf[td->li] = td->qg.d[qi];
          td->spolbuf[td->li] = td->u->s->sg->d[si];
        }
      }
    }
    diff = fabs(td->v0buf[td->li] - td->u->s->v1[td->xi][td->ri]);
    if (td->acc < diff)
      td->acc = diff;
    if (td->qM < td->qpolbuf[td->li])
      td->qM = td->qpolbuf[td->li];
    if (td->sM < td->spolbuf[td->li])
      td->sM = td->spolbuf[td->li];
    if (td->vM < td->v0buf[td->li])
      td->vM = td->v0buf[td->li];
  }
}

void copybufs(thread_init_t *td) {
  for (td->li = 0; td->li < td->u->c->w[td->wid].l.s; ++td->li) {
    calc_indices(td);
    td->u->s->v0[td->xi][td->ri] = td->v0buf[td->li];
    td->u->s->spol[td->xi][td->ri] = td->spolbuf[td->li];
    td->u->s->qpol[td->xi][td->ri] = td->qpolbuf[td->li];
  }

  // if local maximum policy and accuracy values are greater than the
  // values of the global buffers, copy them.
  if (td->u->c->accbuf < td->acc)
    td->u->c->accbuf = td->acc;
  if (td->u->c->sMbuf < td->sM)
    td->u->c->sMbuf = td->sM;
  if (td->u->c->qMbuf < td->qM)
    td->u->c->qMbuf = td->qM;
  if (td->u->c->vMbuf < td->vM)
    td->u->c->vMbuf = td->vM;
}

void lock_mutex(thread_init_t *td) {
  mtx_lock(&td->u->c->mtx);
}

void wait_next_ready(thread_init_t *td) {
  cnd_wait(&td->u->c->next_ready, &td->u->c->mtx);
}

void wait_it_done(thread_init_t *td) {
  cnd_wait(&td->u->c->it_done, &td->u->c->mtx);
}

void signal_done(thread_init_t *td) {
  cnd_signal(&td->u->c->it_done);
}

void broadcast_ready(thread_init_t *td) {
  cnd_broadcast(&td->u->c->next_ready);
}

void unlock_mutex(thread_init_t *td) {
  mtx_unlock(&td->u->c->mtx);
}

void worker_sync(thread_init_t *td) {
#if RAD_NUM_THREADS > 0
  lock_mutex(td);

  // copy to global memory
  copybufs(td);

  // signal done
  ++td->u->c->it_done_count;
  signal_done(td);

  // wait until ready for next iteration
  if (!td->u->c->is_next_ready) {
    wait_next_ready(td);
  }

  unlock_mutex(td);
#endif /* RAD_NUM_THREADS */
}

void alloc_thread_init(thread_init_t *td) {
  td->ovar.m = td->u->m;
  td->v0buf = (double *)calloc(td->u->c->w[td->wid].l.s, sizeof(double));
  td->qpolbuf = (double *)calloc(td->u->c->w[td->wid].l.s, sizeof(double));
  td->spolbuf = (double *)calloc(td->u->c->w[td->wid].l.s, sizeof(double));
  grid_copy(&td->qg, td->u->s->qg);
}

void free_thread_init(thread_init_t *td) {
  grid_free(&td->qg);
  free(td->spolbuf);
  free(td->qpolbuf);
  free(td->v0buf);
}

int thread_start(void *vtd) {
#if RAD_NUM_THREADS > 0
  thread_init_t *td = (thread_init_t *)vtd;

  LOGT("Worker %d starting", td->wid);
  alloc_thread_init(td);

  init_sovle(td);
  worker_sync(td);

  while (td->u->s->acc >= td->u->s->tol) {
    step_sovle(td);
    worker_sync(td);
  }

  LOGT("Worker %d exiting", td->wid);

  free_thread_init(td);
  // The worker's data are dynamically allocated. Main's data is not,
  // thus this cannot be part of free_thread_init()
  free(td);
#endif /* RAD_NUM_THREADS */

  thrd_exit(EXIT_SUCCESS);
}

int thread_resume(void *vtd) {
#if RAD_NUM_THREADS > 0
  thread_init_t *td = (thread_init_t *)vtd;

  LOGT("Worker %d resuming", td->wid);
  alloc_thread_init(td);

  while (td->u->s->acc >= td->u->s->tol) {
    LOGT("Thread %d starts iteration", td->wid);
    step_sovle(td);
    worker_sync(td);
    LOGT("Thread %d ends iteration", td->wid);
  }

  LOGT("Worker %d exiting", td->wid);

  free_thread_init(td);
  // The worrke's data are dynamically allocated. Main's data is not,
  // thus this cannot be part of free_thread_init()
  free(td);
#endif /* RAD_NUM_THREADS */

  thrd_exit(EXIT_SUCCESS);
}

/** Initialize pipeline
 * @details Expects that the worker array is already allocated
 * with RAD_NUM_THREADS (workers) + 1 (main thread) elements.
 * The worker threads should be lunched after this function call.
 * By convention, the first RAD_NUM_THREADS worker
 * objects correspond to the slave system threads and the last
 * object to the main thread.
 * @param u Execution setup */
void init_pipeline(setup_t *u) {
  // Total logical size
  u->c->w[RAD_NUM_THREADS].l.e = u->s->xg->n * u->s->rg->n;
  // Quotient work size
  u->c->w[RAD_NUM_THREADS].l.s =
      u->c->w[RAD_NUM_THREADS].l.e / (RAD_NUM_THREADS + 1);
  // Remainder work size
  int rem = u->c->w[RAD_NUM_THREADS].l.e % (RAD_NUM_THREADS + 1);

  u->c->w[RAD_NUM_THREADS].x.e = u->s->xg->n;
  u->c->w[RAD_NUM_THREADS].r.e = u->s->rg->n;

  u->c->w[0].x.o = u->c->w[0].r.o = u->c->w[0].l.o = 0;
  for (int i = 1; i <= RAD_NUM_THREADS; ++i) {
    // Previous worker's logical size
    u->c->w[i - 1].l.s = (i < rem) ? u->c->w[RAD_NUM_THREADS].l.s + 1
                                   : u->c->w[RAD_NUM_THREADS].l.s;
    // Current worker's logical offset = previous worker's logical end
    u->c->w[i].l.o = u->c->w[i - 1].l.e =
        u->c->w[i - 1].l.o + u->c->w[i - 1].l.s;
    // Current worker's wealth offset = previous worker's wealth end
    u->c->w[i].x.o = u->c->w[i - 1].x.e = u->c->w[i].l.o / u->s->rg->n;
    // Previous worker's wealth size
    u->c->w[i - 1].x.s = u->c->w[i - 1].x.e - u->c->w[i - 1].x.o;
    // Current worker's radius offset = previous worker's radius end
    u->c->w[i].r.o = u->c->w[i - 1].r.e = u->c->w[i].l.o % u->s->rg->n;
    // Previous worker's radius size
    u->c->w[i - 1].r.s = u->c->w[i - 1].r.e - u->c->w[i - 1].r.o;
  }
  u->c->w[RAD_NUM_THREADS].x.s =
      u->c->w[RAD_NUM_THREADS].x.e - u->c->w[RAD_NUM_THREADS].x.o;
  u->c->w[RAD_NUM_THREADS].r.s =
      u->c->w[RAD_NUM_THREADS].r.e - u->c->w[RAD_NUM_THREADS].r.o;
}

int join_thread(const setup_t *u, int i) {
  return thrd_join(u->c->w[i].thread, NULL);
}

void free_sync_resources(setup_t *u) {
  cnd_destroy(&u->c->next_ready);
  cnd_destroy(&u->c->it_done);
  mtx_destroy(&u->c->mtx);
}

void join_all_threads(const setup_t *u) {
#if RAD_NUM_THREADS > 0
  int ec = 0;
  for (int i = 0; i < RAD_NUM_THREADS; ++i) {
    ec = join_thread(u, i);
    if (ec != 0) {
      LOGE("Failed to join thread with code %d", ec);
    }
  }
#endif
}

/** @brief Setup disallocation
 * @details Frees setup's dynamically allocated memory.
 * @param u Setup object to be destroyed. */
void setup_free(setup_t *u) {
  free_sync_resources(u);
  solution_free(u->s);
  free(u->c);
}

/** @brief Save setup
 * @details Consolidates model and solution saving functionality. The format and
 * naming conventions of the saved binary data are set in the model_save() and
 * solution_save() functions.
 * @param u Setup to be saved
 * @param setup_path Path where binary model and solution data are to be stored
 */
void setup_save(const setup_t *u, const char *setup_path) {
  model_save(u->m, setup_path);
  solution_save(u->s, setup_path);
}

void init_sync_resources(setup_t *u) {
  mtx_init(&u->c->mtx, mtx_plain);
  cnd_init(&u->c->it_done);
  cnd_init(&u->c->next_ready);

  u->c->it_done_count = 0;
  u->c->is_next_ready = false;
}

void create_thread(thread_init_t *td, int i, thrd_start_t thread_main) {
  int status = 0;
  status = thrd_create(&td->u->c->w[i].thread, thread_main, td);

  if (status != 0) {
    LOGE("Failed to create thread %d", status);
  }
}

void init_concurrency(setup_t *u) {
  u->c = (concurrency_t *)calloc(1, sizeof(concurrency_t));

  // set the initial buffer high enough, so that the solver does not
  // terminate in the starting / resuming iteration
  u->c->accbuf = 0;
  u->c->sMbuf = 0;
  u->c->qMbuf = 0;
  u->c->vMbuf = 0;
  u->c->qM = u->s->qg->M;

  init_pipeline(u);

#if RAD_NUM_THREADS > 0
  init_sync_resources(u);
#endif /* RAD_NUM_THREADS */
}

void log_title() {
#if RAD_LOG_CYCLE > 0
  LOGV("%10s|%10s|%10s|%10s|%10s", "iteration", "diff", "vfnc", "qmax", "smax");
#endif
}

void log_cycle(const setup_t *u) {
#if RAD_LOG_CYCLE > 0
  if (u->s->it && u->s->it % RAD_LOG_CYCLE == 0) {
    LOGV("%10d|%10.4e|%10.4e|%10.4e|%10.4e", u->s->it, u->c->accbuf,
         u->c->vMbuf, u->c->qMbuf, u->c->sMbuf);
  }
#endif
}

void adjust_grid_bounds(const setup_t *u) {
  // if the adapted global maximum policy values are less that the solution's
  // maximum grid values, copy them. Then reset the buffers.
  if (u->s->it) {
    double adp = u->c->qMbuf + u->s->qadp / (u->s->it + 1);
    if (adp < u->c->qM) {
      // Set also qg->M for resuming functionality
      u->s->qg->M = u->c->qM = adp;
    }
    adp = u->c->sMbuf + u->s->sadp / (u->s->it + 1);
    if (adp < u->s->sg->M) {
      u->s->sg->M = adp;
      grid_calc(u->s->sg);
    }
  }
}

void resume_concurrency(setup_t *u) {
  u->c = (concurrency_t *)calloc(1, sizeof(concurrency_t));

  log_title();

  for (int xi = 0; xi < u->s->xg->n; ++xi) {
    for (int ri = 0; ri < u->s->rg->n; ++ri) {
      if (u->c->qMbuf < u->s->qpol[xi][ri])
        u->c->qMbuf = u->s->qpol[xi][ri];
      if (u->c->sMbuf < u->s->spol[xi][ri])
        u->c->sMbuf = u->s->spol[xi][ri];
      if (u->c->vMbuf < u->s->v1[xi][ri])
        u->c->vMbuf = u->s->v1[xi][ri];
    }
  }

  log_cycle(u);

  u->c->qM = u->s->qg->M;
  adjust_grid_bounds(u);

  ++u->s->it;
  u->c->accbuf = 0;
  u->c->sMbuf = 0;
  u->c->qMbuf = 0;
  u->c->vMbuf = 0;

  init_pipeline(u);

#if RAD_NUM_THREADS > 0
  init_sync_resources(u);
#endif /* RAD_NUM_THREADS */
}

/** @brief Load setup
 * @details Consolidates model and solution loading functionality. The format
 * and naming conventions of the binary data are set in the model_load() and
 * solution_load() functions. The order of the functions in the model's Inherits
 * the convention used in model loading.
 * @param u Setup to be populated
 * @param setup_path Path with stored binary model and solution data
 * @param obhparts Model's functional specification
 * @see objpart_st */
void setup_load(setup_t *u, const char *setup_path,
                const struct objpart_st *obhparts) {
  model_load(u->m, setup_path, obhparts);
  solution_load(u->s, setup_path);

  resume_concurrency(u);
}

/** @brief Setup initialization
 * @details The function is responsible for setting up a model using
 * initialization values taken from the passed parameter file. The parameter
 * file given should contain values for model parameter as well as
 * parameterization for solution data, such as for instance grid
 * specifications. The functional specification of the attentional costs,
 * temporal utility, radius and wealth dynamics is passed separately using the
 * obhparts variable. Pipeline calculations are performed here. Threads
 * initialization is not performed here.
 * @param u Setup structure to be initialized.
 * @param parameter_filename Input key-value, text file
 * @param obhparts Function pointers to model's functional specifications
 * @return Zero on success, non-zero otherwise
 * @see pmap_t.h, objpart_st. */
int setup_init(setup_t *u, const char *parameter_filename,
               const struct objpart_st *obhparts) {
  int ec = 0;
  pmap_t pmap;
  char pfile_path[RAD_PATH_BUFFER_SZ];

  snprintf(pfile_path, RAD_PATH_BUFFER_SZ, "%s" CCM_FILE_SYSTEM_SEP "%s",
           RAD_DATA_DIR, parameter_filename);
  if ((ec = pmap_init(&pmap, pfile_path)) != 0) {
    LOGE("Setup initialization failed with code %d", ec);
    return -1;
  }

  model_init(u->m, &pmap, obhparts);
  solution_init(u->s, &pmap);

  pmap_free(&pmap);

  init_concurrency(u);

  return 0;
}

void swapv1v0(thread_init_t *td) {
  double *buf = NULL;
  for (int i = 0; i < td->u->s->xg->n; ++i) {
    buf = td->u->s->v1[i];
    td->u->s->v1[i] = td->u->s->v0[i];
    td->u->s->v0[i] = buf;
  }
}

void main_sync(thread_init_t *td) {
#if RAD_NUM_THREADS > 0
  lock_mutex(td);
#endif /* RAD_NUM_THREADS */

  copybufs(td);

#if RAD_NUM_THREADS > 0
  while (td->u->c->it_done_count < RAD_NUM_THREADS) {
    wait_it_done(td);
  }
  // If the main is here, all workers are waiting on next_ready condition
  td->u->c->it_done_count = 0;
  td->u->c->is_next_ready = false;
#endif /* RAD_NUM_THREADS */

  log_cycle(td->u);

  // swap
  swapv1v0(td);

  // set the solution's accuracy equal to the global accuracy buffer and
  // then reset the global buffer.
  td->u->s->acc = td->u->c->accbuf;
  td->u->c->accbuf = 0;

  adjust_grid_bounds(td->u);

  td->u->c->qMbuf = 0;
  td->u->c->sMbuf = 0;
  td->u->c->vMbuf = 0;

#if RAD_SAVE_CYCLE > 0
  if (td->u->s->it && td->u->s->it % RAD_SAVE_CYCLE == 0) {
    char buf[RAD_PATH_BUFFER_SZ];
    snprintf(buf, RAD_PATH_BUFFER_SZ, "save" CCM_FILE_SYSTEM_SEP "it%05d",
             td->u->s->it);
    // calculate the adjusted quantity grid for saving
    grid_calc(td->u->s->qg);
    setup_save(td->u, buf);
  }
#endif

  // increment iteration (should be after possible save)
  ++td->u->s->it;

#if RAD_NUM_THREADS > 0
  broadcast_ready(td);
  unlock_mutex(td);
#endif /* RAD_NUM_THREADS */
}

void main_fixed_point(thread_init_t *td) {
  while (td->u->s->acc >= td->u->s->tol) {
    step_sovle(td);
    main_sync(td);
  }

  // swap if needed
  if (td->u->s->it % 2 != 0) {
    swapv1v0(td);
  }
}

/** @brief Model solver
 * @details This is the top-level main functionality call. The function expects
 * an initialized model setup (see setup_init()). If multi-threading mode is
 * enabled, the function initializes threading based on the pipeline
 * allocations calculated in the setup. The function initializes
 * the iterative solution procedure. It performs the fixed point calculation
 * steps and checks for convergence. The iterations stops if the maximum number
 * of iterations is reached, or if the convergence criterion is met. Then the
 * function disallocates threads and returns.
 * @param u Model setup
 * @return Zero on success, non-zero otherwise */
int setup_solve(setup_t *u) {
#if RAD_NUM_THREADS > 0
  for (long i = 0; i < RAD_NUM_THREADS; ++i) {
    thread_init_t *td = (thread_init_t *)calloc(1, sizeof(thread_init_t));
    td->wid = i;
    td->u = u;
    td->pv0 = u->s->v0;
    create_thread(td, i, thread_start);
  }
#endif /* RAD_NUM_THREADS */

  thread_init_t td = {
      .wid = RAD_NUM_THREADS, .u = u, .pv0 = u->s->v0, .ovar = {.m = u->m}};
  alloc_thread_init(&td);
  u->c->accbuf = u->s->tol + 1;

  log_title();

  init_sovle(&td);
  main_sync(&td);

  main_fixed_point(&td);

  free_thread_init(&td);

  join_all_threads(td.u);

  return 0;
}

/** @brief Resume model solver
 * @details The functionality is similar to setup_solve(). The function is
 * intended to be used for resuming execution from a point stored in the file
 * system. This functionality is helpful when the application is executed in
 * environments with execution-time constraints. It assumes that the passed
 * setup is populated using the setup_load() function. In contrast to
 * setup_solve(), this function does not call setup initialization routines. It
 * rather jumps directly to iteration functionality execution.
 * @param u Model setup
 * @return Zero on success, non-zero otherwise */
int setup_resume(setup_t *u) {
#if RAD_NUM_THREADS > 0
  for (long i = 0; i < RAD_NUM_THREADS; ++i) {
    thread_init_t *td = (thread_init_t *)calloc(1, sizeof(thread_init_t));
    td->wid = i;
    td->u = u;
    td->pv0 = u->s->v0;
    create_thread(td, i, thread_resume);
  }
#endif /* RAD_NUM_THREADS */

  thread_init_t td = {
      .wid = RAD_NUM_THREADS, .u = u, .pv0 = u->s->v0, .ovar = {.m = u->m}};
  alloc_thread_init(&td);

  main_fixed_point(&td);

  free_thread_init(&td);

  join_all_threads(td.u);

  return 0;
}

/** @brief Automatic last save point acquisition
 * @details This is an auxiliary function that located the last setup save point
 * in a particular folder. It is intended to be used in conjunction with the
 * periodic iteration backup functionality of the applications. When
 * RAD_SAVE_CYCLE is positive, the applications create backup binary files of
 * the execution status in the file system every RAD_SAVE_CYCLE iterations. The
 * files are stored in directories of the form ´itN`, where N is replaced by the
 * iteration count. This function searches into the applications' data path for
 * the greatest saved iteration. It stores the located path into the passed
 * character pointer.
 * @param save_point An output string that stores the last save point directory
 * @return Zero on success and non-zero otherwise */
int setup_find_last_saved(char *save_point) {
  char dir_path[RAD_PATH_BUFFER_SZ];
#if __unix__
  DIR *dir;
  struct dirent *ent, *last = NULL;

  snprintf(dir_path, RAD_PATH_BUFFER_SZ, "%s" CCM_FILE_SYSTEM_SEP "save",
           RAD_TEMP_DIR);
  if ((dir = opendir(dir_path)) != NULL) {
    /* print all the files and directories within directory */
    while ((ent = readdir(dir)) != NULL) {
      if (strncmp("it", ent->d_name, 2) == 0) {
        if (!last || strcmp(ent->d_name, last->d_name) > 0) {
          last = ent;
        }
      }
    }
    if (last) {
      snprintf(save_point, RAD_PATH_BUFFER_SZ, "save" CCM_FILE_SYSTEM_SEP "%s",
               last->d_name);
    } else {
      return -1;
    }
    closedir(dir);
  } else {
    LOGE("Failed to open directory");
    return -2;
  }
#else
  WIN32_FIND_DATA ffd;
  HANDLE hFind;
  char last[RAD_PATH_BUFFER_SZ] = {0};

  snprintf(dir_path, RAD_PATH_BUFFER_SZ,
           "%s" CCM_FILE_SYSTEM_SEP "save" CCM_FILE_SYSTEM_SEP "it*",
           RAD_TEMP_DIR);

  hFind = FindFirstFileA(dir_path, &ffd);
  if (hFind == INVALID_HANDLE_VALUE) {
    LOGE("Failed to find file with error (%d)", GetLastError());
    return -2;
  }

  do {
    if (ffd.cFileName == NULL || strcmp(ffd.cFileName, last) > 0) {
      strncpy(last, ffd.cFileName, RAD_PATH_BUFFER_SZ);
    }
  } while (FindNextFileA(hFind, &ffd) != 0);
  snprintf(save_point, RAD_PATH_BUFFER_SZ, "save" CCM_FILE_SYSTEM_SEP "%s",
           last);
  FindClose(hFind);
#endif

  return 0;
}

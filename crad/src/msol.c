#include "rad_conf.h"
#include "rad_setup.h"
#include "rad_specs.h"
#include "rad_types.h"

#include "cross_comp.h"

#include "stdio.h"
#include "stdlib.h"
#include "time.h"

#define LM_LEVEL 3
#include "logger.h"

int main(void) {
  int rc;
  double dur;
  const objpart_t objparts[4] = {{util, CCM_STRINGIFY(_util_)},
                                 {cost, CCM_STRINGIFY(_cost_)},
                                 {radt, CCM_STRINGIFY(_radt_)},
                                 {wltt, CCM_STRINGIFY(_wltt_)}};
  model_t m;
  sol_t s;
  setup_t u = {.m = &m, .s = &s};

  if (setup_init(&u, "main.prm", objparts) != 0) {
    return EXIT_FAILURE;
  }

  LOGI("Initializing numerical solver");
  s.xbeg = clock();
  if ((rc = setup_solve(&u)) != 0) {
    LOGE("Numerical solver failed with code %d", rc);
    setup_free(&u);
    return EXIT_FAILURE;
  }
  s.xend = clock();
  dur = (double)(s.xend - s.xbeg) / CLOCKS_PER_SEC;
  LOGI("Numerical solver completed (%d iter, %f sec)", s.it, dur);

  setup_save(&u, "msol");

  setup_free(&u);
  return EXIT_SUCCESS;
}

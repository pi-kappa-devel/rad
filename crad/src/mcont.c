#include "rad_conf.h"
#include "rad_setup.h"
#include "rad_specs.h"
#include "rad_types.h"

#include "cross_comp.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
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

  char save_point[RAD_PATH_BUFFER_SZ];
  if (setup_find_last_saved(save_point)) {
    LOGE("Failed to retrieve save point");
    return EXIT_FAILURE;
  }
  LOGI("Resuming numerical solver from save point %s", save_point);
  setup_load(&u, save_point, objparts);

  if ((rc = setup_resume(&u)) != 0) {
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

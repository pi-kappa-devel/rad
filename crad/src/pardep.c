#include "rad_setup.h"
#include "rad_types.h"
#include "rad_specs.h"
#include "rad_conf.h"

#include "stdio.h"
#include "stdlib.h"
#include "time.h"

#include "pmap_t.h"
#include "grid_t.h"

#define LM_LEVEL 3
#include "logger.h"
#include "cross_comp.h"

int main(void) {
	int rc;
	double dur;
	const objpart_t objparts[4] = {
		{util, CCM_STRINGIFY(_util_)},
		{cost, CCM_STRINGIFY(_cost_)},
		{radt, CCM_STRINGIFY(_radt_)},
		{wltt, CCM_STRINGIFY(_wltt_)}
	};
	model_t m;
	sol_t s;
	setup_t u = { .m = &m,.s = &s };
	char path_buf[RAD_PATH_BUFFER_SZ];
	pmap_t pmap;
	grid_t deltag, alphag, gammag;

	snprintf(path_buf, RAD_PATH_BUFFER_SZ, "%s" CCM_FILE_SYSTEM_SEP  "pardep.prm", RAD_DATA_DIR);
	pmap_init(&pmap, path_buf);

#define mdepparam(p) \
	/* Dependence on p */ \
	grid_init_str(&p##g, pmap_find(&pmap,#p"g")); \
	for(int it=0; it<p##g.n; ++it) { \
		if(setup_init(&u, "pardep.prm", objparts)) { \
			return EXIT_FAILURE; \
		} \
		m.p = p##g.d[it]; \
		LOGI("Solving model for " #p " = %f (%d/%d)...", m.p, it+1, p##g.n); \
		s.xbeg = clock(); \
		if((rc = setup_solve(&u))!=0) { \
			LOGE("Numerical solver failed with code %d", rc); \
			setup_free(&u); \
			return EXIT_FAILURE; \
		} \
		s.xend = clock(); \
		dur = (double)(s.xend - s.xbeg) / CLOCKS_PER_SEC; \
		LOGI("Numerical solver completed (%d iter, %f sec)", s.it, dur); \
		snprintf(path_buf, RAD_PATH_BUFFER_SZ, #p CCM_FILE_SYSTEM_SEP #p"%02d", it); \
		setup_save(&u, path_buf); \
		setup_free(&u); \
	} \
	grid_free(&p##g)

	mdepparam(delta);
	mdepparam(alpha);
	mdepparam(gamma);

#undef mdepparam

	pmap_free(&pmap);
	return EXIT_SUCCESS;
}

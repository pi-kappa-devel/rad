#include "rad_types.h"
#include "rad_conf.h"

#include "grid_t.h"
#include "pmap_t.h"

#include "cross_comp.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"
#include "limits.h"
#include "errno.h"

#if __unix__
  #include "sys/stat.h"
  #include "unistd.h"
#else
  #include "winsock2.h"
#endif

#define LM_LEVEL 3
#include "logger.h"

void set_model_callbacks(model_t* m, const objpart_t* objparts) {
	m->util = objparts[0];
	m->cost = objparts[1];
	m->radt = objparts[2];
	m->wltt = objparts[3];
}

#define ifvar(st, v, c, fmt) \
	else if(!strcmp(pmap_gkey(pmap,i),#v)) { \
		st->v = c(pmap_gvalue(pmap,i)); \
		LOGD("Setting " #v " = %" #fmt " (from %s)", st->v, vbeg); \
	}

#define ifgrid(st, v) \
	else if(!strcmp(pmap_gkey(pmap,i),#v)) { \
		st->v = (grid_t*)malloc(sizeof(grid_t)); \
		pmap_cvalue(&buf, pmap, i); \
		grid_init_str(st->v, buf); \
		LOGD( \
			"Setting " #v " = %d,%f,%f,%f (from %s)", \
			st->v->n, st->v->m, st->v->M, st->v->w, pmap->p->key \
		); \
		free(buf); \
	}

/** @brief Model initialization
 * @details Set the parameters using the passed parameter file and hooks the
 * given function to the corresponding objective function parts. The expected
 * order of the parts is
 *  - util (utility function),
 *  - cost (mental costs function),
 *  - radt (radius transition function) and
 *  - wltt (wealth transition function).
 * @param m Model
 * @param pmap Parameter map
 * @param objparts Objective function parts
 * @see solution_init() */
void model_init(model_t* m, const struct pmap_st* pmap, const objpart_t* objparts) {
	for(int i=0; i<pmap->n; ++i) {
		if(0) { }
		ifvar(m, beta, atof, f)
		ifvar(m, delta, atof, f)
		ifvar(m, alpha, atof, f)
		ifvar(m, gamma, atof, f)
		ifvar(m, R, atof, f)
	}
	if(m->R < -1) { m->R = 1/m->beta; }
	set_model_callbacks(m, objparts);
}

/** @brief Initialize solution structure
 * @details The function is responsible for assigning the passed values of the parameter map to solution parameters.
 * It constructs the state and control grids. It also allocates memory to hold the value function and policy
 * approximations.
 * @param s An uninitialized solution structure
 * @param pmap A parameter map with initialization values
 * @see model_init(), solution_free() */
void solution_init(sol_t* s, const struct pmap_st* pmap) {
	char* buf;

	for(int i=0; i<pmap->n; ++i) {
		if(0) { }
		ifvar(s, maxit, atoi, f)
		ifvar(s, tol, atof, f)
		ifvar(s, qadp, atoi, f)
		ifvar(s, sadp, atof, f)
		ifgrid(s, xg)
		ifgrid(s, rg)
		ifgrid(s, qg)
		ifgrid(s, sg)
	}

	s->v0 = (double**)malloc(sizeof(double*)*(s->xg->n));
	s->v1 = (double**)malloc(sizeof(double*)*(s->xg->n));
	s->qpol = (double**)malloc(sizeof(double*)*(s->xg->n));
	s->spol = (double**)malloc(sizeof(double*)*(s->xg->n));

	for(int i=0; i<s->xg->n; ++i) {
		s->v0[i] = (double*)calloc(s->rg->n, sizeof(double));
		s->v1[i] = (double*)calloc(s->rg->n, sizeof(double));
		s->qpol[i] = (double*)calloc(s->rg->n, sizeof(double));
		s->spol[i] = (double*)calloc(s->rg->n, sizeof(double));
	}

	s->acc = s->tol + 1;
	s->it = 0;
	s->xbeg = 0;
	s->xend = 0;
}

#undef ifvar
#undef ifgrid

void save_head(const char* filename) {
	FILE* fh = NULL;
	fh = fopen(filename, "w");
#if RAD_SAFE_MODE
	if(!fh) {
		LOGE("Failed to create header file '%s' with errno %d", filename, errno);
		return;
	}
#endif /* RAD_SAFE_MODE */

#ifdef __unix__
	char hostname[HOST_NAME_MAX];
	char username[LOGIN_NAME_MAX];
	gethostname(hostname, HOST_NAME_MAX);
	getlogin_r(username, LOGIN_NAME_MAX);
#else
#define INFO_BUFFER_SIZE 32767
	DWORD bufCharCount = INFO_BUFFER_SIZE;
	TCHAR hostname[INFO_BUFFER_SIZE];
	TCHAR username[INFO_BUFFER_SIZE];

	GetComputerName(hostname, &bufCharCount);
	GetUserName(username, &bufCharCount);
#undef INFO_BUFFER_SIZE
#endif 

	time_t rawtime;
	struct tm timeinfo;
	time(&rawtime);
	localtime_r(&rawtime, &timeinfo);
#define ASCTIME_BUFFER_SIZE 26
	char timestamp[ASCTIME_BUFFER_SIZE];
	asctime_r(&timeinfo, timestamp);
#undef ASCTIME_BUFFER_SIZE

	fprintf(fh, "%-10s:%s", "Created", timestamp);
	fprintf(fh, "%-10s:%s\n", "Host", hostname);
	fprintf(fh, "%-10s:%s\n", "User", username);

	fclose(fh);
}

void mkmodel_dir(const char* model_path) {
	char path_buffer[RAD_PATH_BUFFER_SZ];
	char filename[2*RAD_PATH_BUFFER_SZ];

	snprintf(path_buffer, RAD_PATH_BUFFER_SZ, "%s" CCM_FILE_SYSTEM_SEP  "%s", RAD_TEMP_DIR, model_path);
	int status = mkdirp(path_buffer, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
	if(!status) {
		snprintf(
			filename, sizeof(filename),
			"%s" CCM_FILE_SYSTEM_SEP  "head", path_buffer
		);
		save_head(filename);
	}
	else if(status<0) {
		if(errno!=EEXIST) {
			LOGE("Failed to create directory '%s' with errno %d", path_buffer, errno);
		}
	}
}

/** @brief Load model
 * @details Performs a binary loads of the model's parameters. The parameters are stored to the passed model object.
 * They are read from the passed model path. The function expects that a binary model file name `model´ exists in the
 * passed directory. The model functional specification is load by the the passed objective parts objects and uses
 * the same rules as the model_init() function.
 * @param m Model object were the loaded data are stored
 * @param model_path The path that contains binary saved data
 * @param objparts The model's functional specification
 * @see objpart_t, model_init() */
void model_load(model_t* m, const char* model_path, const objpart_t* objparts) {
	char filename[RAD_PATH_BUFFER_SZ];
	snprintf(
		filename, RAD_PATH_BUFFER_SZ,
		"%s" CCM_FILE_SYSTEM_SEP  "%s" CCM_FILE_SYSTEM_SEP  "model",
		RAD_TEMP_DIR, model_path
	);
	FILE* fh = NULL;
	fh = fopen(filename, "rb");
#if RAD_SAFE_MODE
	if(!fh) {
		LOGE("Failed to open model file '%s' with errno %d", filename, errno);
		return;
	}
#endif /* RAD_SAFE_MODE */
	fread(m, sizeof(*m), 1, fh);
	fclose(fh);

	set_model_callbacks(m, objparts);
}

/** @brief Save model
 * @details Performs a binary save of the model to he file system. The resulting resulting binary files are intended
 * to be used  by model_load() to replicated the saved data. The function creates the model directory if it does not
 * exist. Then it saves
 *  - a binary dump file of the object and
 *  - a text file with the model's functional specification.
 * @param m Model object to be saved
 * @param model_path Save path
 * @see solution_load() */
void model_save(const model_t* m, const char* model_path) {
	mkmodel_dir(model_path);

	char filename[RAD_PATH_BUFFER_SZ];

	snprintf(
		filename, RAD_PATH_BUFFER_SZ,
		"%s" CCM_FILE_SYSTEM_SEP  "%s" CCM_FILE_SYSTEM_SEP  "model",
		RAD_TEMP_DIR, model_path
	);
	FILE* fh = NULL;
	fh = fopen(filename, "wb");
#if RAD_SAFE_MODE
	if(!fh) {
		LOGE("Failed to create model file '%s' with errno %d", filename, errno);
		return;
	}
#endif /* RAD_SAFE_MODE */
	fwrite(m, sizeof(*m), 1, fh);
	fclose(fh);

	pmap_t pmap = { .p=NULL, .n=0 };
	pmap_add(&pmap, "util", m->util.str);
	pmap_add(&pmap, "cost", m->cost.str);
	pmap_add(&pmap, "radt", m->radt.str);
	pmap_add(&pmap, "wltt", m->wltt.str);
	snprintf(
		filename, RAD_PATH_BUFFER_SZ,
		"%s" CCM_FILE_SYSTEM_SEP  "%s" CCM_FILE_SYSTEM_SEP  "fncs",
		RAD_TEMP_DIR, model_path
	);
	pmap_save(&pmap, filename);
}

void load_variable2(
	double*** var, const char* filename
) {
	FILE* fh = NULL;
	fh = fopen(filename, "rb");
#if RAD_SAFE_MODE
	if(!fh) {
		LOGE("Failed to open variable file '%s' with errno %d", filename, errno);
		return;
	}
#endif /* RAD_SAFE_MODE */

	short d1, d2;
	fread(&d1, sizeof(short), 1, fh);
	fread(&d2, sizeof(short), 1, fh);
	*var = (double**)malloc(sizeof(double*)*d1);
	for(int i1=0; i1<d1; ++i1) {
		(*var)[i1] = (double*)malloc(sizeof(double)*d2);
		fread((*var)[i1], sizeof(double), d2, fh);
	}

	fclose(fh);

#ifdef RAD_DEBUG
	double varm=(*var)[0][0], varM=(*var)[0][0];
	for(int xi=0; xi<d1; ++xi) {
		for(int ri=0; ri<d2; ++ri) {
			if(varm>(*var)[xi][ri]) varm = (*var)[xi][ri];
			if(varM<(*var)[xi][ri]) varM = (*var)[xi][ri];
		}
	}
	LOGD("Variable range is [%f,%f]", varm, varM);
#endif
}

void save_variable2(
	short d1, short d2, double** const var, const char* filename
) {
	FILE* fh = NULL;
	fh = fopen(filename, "wb");
#if RAD_SAFE_MODE
	if(!fh) {
		LOGE("Failed to create variable file '%s' with errno %d", filename, errno);
		return;
	}
	size_t wc;
#endif /* RAD_SAFE_MODE */

	fwrite(&d1, sizeof(d1), 1, fh);
	fwrite(&d2, sizeof(d2), 1, fh);
	for(int i1=0; i1<d1; ++i1) {
#if RAD_SAFE_MODE
		wc =
#endif /* RAD_SAFE_MODE */
		fwrite(var[i1], sizeof(double), d2, fh);
#if RAD_SAFE_MODE
		if(wc!=d2) {
			LOGE("Failed to write to file '%s' with return value %zd", filename, wc);
			return;
		}
#endif /* RAD_SAFE_MODE */
	}

	fclose(fh);
}

/** @brief Load solution
 * @details Loads binary saved solution data from the file system. The resulting solution structure creates a binary
 * equivalent data object with the one that called the solution_save() function. The
 * function expect to find
 *  - a global solution binary file,
 *  - four grid binary files (wealth, radius, quantity and effort) and
 *  - two optimal control binary files and two (current and next date's) value function binary files
 * in the passed directory.
 * @param s Solution structure to be populated
 * @param model_path Base file system directory containing saved execution data
 * @see solution_free() */
void solution_load(sol_t* s, const char* model_path) {
	char filename[RAD_PATH_BUFFER_SZ];

	snprintf(
		filename, RAD_PATH_BUFFER_SZ,
		"%s" CCM_FILE_SYSTEM_SEP  "%s" CCM_FILE_SYSTEM_SEP  "solution",
		RAD_TEMP_DIR, model_path
	);
	FILE* fh = NULL;
	fh = fopen(filename, "rb");
#if RAD_SAFE_MODE
	if(!fh) {
		LOGE("Failed to open solution file '%s' with errno %d", filename, errno);
		return;
	}
#endif /* RAD_SAFE_MODE */
	fread(s, sizeof(*s), 1, fh);
	fclose(fh);

#define loadg(gname) \
	s->gname = (grid_t*)malloc(sizeof(grid_t)); \
	snprintf( \
		filename, RAD_PATH_BUFFER_SZ, \
		"%s" CCM_FILE_SYSTEM_SEP  "%s" CCM_FILE_SYSTEM_SEP  #gname, \
		RAD_TEMP_DIR, model_path \
	); \
	grid_load(s->gname, filename);

	loadg(xg);
	loadg(rg);
	loadg(qg);
	loadg(sg);

#undef loadg

#define loadv(vname) \
	snprintf( \
		filename, RAD_PATH_BUFFER_SZ, \
		"%s" CCM_FILE_SYSTEM_SEP  "%s" CCM_FILE_SYSTEM_SEP  #vname, \
		RAD_TEMP_DIR, model_path \
	); \
	load_variable2(&s->vname, filename);

	loadv(qpol);
	loadv(spol);
	loadv(v0);
	loadv(v1);

#undef loadv
}

void debug_check_save(
	const sol_t* s, const char* model_path,
	double ** const svar, const char* varname
) {
	char filename[RAD_PATH_BUFFER_SZ];
	double **var;

	snprintf(
		filename, RAD_PATH_BUFFER_SZ,
		"%s" CCM_FILE_SYSTEM_SEP  "%s" CCM_FILE_SYSTEM_SEP  "%s",
		RAD_TEMP_DIR, model_path, varname
	);
	load_variable2(&var, filename);
	for(int xi=0; xi<s->xg->n; ++xi) {
		for(int ri=0; ri<s->rg->n; ++ri) {
			if(svar[xi][ri]!=var[xi][ri]) {
				LOGW("Inaccurate save for variable %s at (%d,%d)", varname, xi, ri);
			}
		}
		free(var[xi]);
	}
	free(var);
}

/** @brief Save solution
 * @details Saves the given solution structure in binary format to the file system. The created binary files are
 * intended to be used by solution_load() to create a binary replica of the saved object.
 * The function creates
 *  - a text file with the model's function specifications (see rad_specs.h)
 *  - a head file with creation information in key-value pairs
 *  - a global solution binary dump file of the solution structure,
 *  - four grid binary dump files (wealth, radius, quantity and effort) and
 *  - two optimal control and two (current and next date's) value function binary dump files
 * in the passed directory.
 * @param s Solution structure to be saved
 * @param model_path Base file system save directory */
void solution_save(const sol_t* s, const char* model_path) {
	char filename[RAD_PATH_BUFFER_SZ];

#define saveg(gname) \
	snprintf( \
		filename, RAD_PATH_BUFFER_SZ, \
		"%s" CCM_FILE_SYSTEM_SEP  "%s" CCM_FILE_SYSTEM_SEP  #gname, \
		RAD_TEMP_DIR, model_path \
	); \
	grid_save(s->gname, filename);

	saveg(xg);
	saveg(rg);
	saveg(qg);
	saveg(sg);

#undef saveg

#define savev(vname) \
	snprintf( \
		filename, RAD_PATH_BUFFER_SZ, \
		"%s" CCM_FILE_SYSTEM_SEP  "%s" CCM_FILE_SYSTEM_SEP  #vname, \
		RAD_TEMP_DIR, model_path \
	); \
	save_variable2(s->xg->n, s->rg->n, s->vname, filename);

	savev(qpol);
	savev(spol);
	savev(v0);
	savev(v1);

#undef savev

#ifdef RAD_DEBUG

#define checkv(vname) debug_check_save(s, model_path, s->vname, #vname);

	checkv(qpol);
	checkv(spol);
	checkv(v0);
	checkv(v1);

#undef checkv

#endif

	snprintf(
		filename, RAD_PATH_BUFFER_SZ,
		"%s" CCM_FILE_SYSTEM_SEP  "%s" CCM_FILE_SYSTEM_SEP  "solution",
		RAD_TEMP_DIR, model_path
	);
	FILE* fh = NULL;
	fh = fopen(filename, "wb");
#if RAD_SAFE_MODE
	if(!fh) {
		LOGE("Failed to create solution file '%s' with errno %d", filename, errno);
		return;
	}
#endif /* RAD_SAFE_MODE */
	fwrite(s, sizeof(*s), 1, fh);
	fclose(fh);
}

/** @brief Free solution
 * @details Disallocates a solution object. Object created using solution_init() and
 * solution_load() are expected to be finalized using this function.
 * @param s Solution structure to be destroyed. */
void solution_free(sol_t* s) {
	for(int i=0; i<s->xg->n; ++i) {
		free(s->v0[i]);
		free(s->v1[i]);
		free(s->qpol[i]);
		free(s->spol[i]);
	}
	free(s->v0);
	free(s->v1);
	free(s->qpol);
	free(s->spol);

	grid_free(s->xg);
	grid_free(s->rg);
	grid_free(s->qg);
	grid_free(s->sg);

	free(s->xg);
	free(s->rg);
	free(s->qg);
	free(s->sg);
}

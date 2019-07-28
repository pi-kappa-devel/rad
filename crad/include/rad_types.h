/** @file rad_types.h
 * @brief Data type definitions and model functionality
 * @details The two basic application data types are the model and solution
 * structures. The first models the radial attention example parameters and
 * functions. The second models the numerical approximation of solutions. The
 * indented initialization method for types is through using parameter files
 * (see pmap_t). */

#ifndef RAD_TYPES_H_
#define RAD_TYPES_H_

struct grid_st;
struct model_st;
struct pmap_st;

/** @brief Objective function input structure
 * @details Contains parameters, state variables and controls used to evaluate
 * the model's objective function. The evaluation involves evaluating the
 * temporal utility, the attentional costs, the new radius of attention and
 * the new wealth state. */
struct objvar_st {
	/** @brief Model parameter data */
	const struct model_st* m;
	/** @brief Current wealth state */
	double x;
	/** @brief Current radius of attention */
	double r;
	/** @brief Average product quantity */
	double q;
	/** @brief Effort */
	double s;
};
/** @brief Objective function input type */
typedef struct objvar_st objvar_t;

/** @brief Objective function part structure
 * @details Contains a callback function that is used in the evaluation of the
 * model's objective. The expected parts are the temporal utility, the
 * attentional costs, the radius dynamics and the wealth dynamics. In
 * addition it offers a string placeholder for storing the content of the
 * definition of the function. The intended use is to write the content of
 * the callback as a macro and the use CCM_STRINGIFY to get a string it and
 * store it in the placeholder. See also rad_specs.h */
struct objpart_st {
	/** @brief Objective function part callback
	 * @details A function used in the evaluation of the model's objective.
	 * @param v Input parameters, state variables and controls
	 * @return Evaluated part */
	double(*fnc)(const objvar_t* v);
	/** @brief Objective function part definition
	 * @details A string of the content of the part' definition */
	const char* str;
};
/** @brief Objective function part type */
typedef struct objpart_st objpart_t;

/** @brief Model structure
 * @details Contains model's parameters and objective function parts. Hooking
 * the objective function pars is the responsibility if the user. The structure
 * is used in binary file IO operations. The convention for the order of model
 * parameters is that they appear in lexicographic order as fields. Function
 * callbacks always appear in the end and the order of appearance does not
 * matter. */
struct model_st {
	/** @brief Attentional costs factor */
	double alpha;
	/** @brief Discount factor */
	double beta;
	/** @brief Memory persistence */
	double delta;
	/** @brief Complementarity factor */
	double gamma;
	/** @brief Return */
	double R;

	/** @brief Utility function */
	objpart_t util;
	/** @brief Cost function */
	objpart_t cost;
	/** @brief Radius transition */
	objpart_t radt;
	/** @brief Wealth transition */
	objpart_t wltt;
};
/** @brief Model Type */
typedef struct model_st model_t;

void model_init(model_t* m, const struct pmap_st* pmap, const objpart_t* objparts);
void model_load(model_t* m, const char* model_path, const objpart_t* objparts);
void model_save(const model_t* m, const char* model_path);

/** @brief Solution structure
 * @details Contains solution information. This involves discretized domain data
 * in grid_st fields, approximations of the value function and the optimal
 * controls, numerical method parameters, output accuracy and timing.  Grids
 * and approximation variables are allocated dynamically. Disallocation is the
 * responsibility of the user. */
struct sol_st {
	/** @brief Wealth grid */
	struct grid_st* xg;
	/** @brief Radius grid */
	struct grid_st* rg;
	/** @brief Quantity grid */
	struct grid_st* qg;
	/** @brief Effort grid */
	struct grid_st* sg;

	/** @brief Quantity grid adaptation scale */
	double qadp;
	/** @brief Effort grid adaptation scale */
	double sadp;

	/** @brief Initial value function */
	double** v0;
	/** @brief Final value function */
	double** v1;
	/** @brief Quantity policy */
	double** qpol;
	/** @brief Effort policy */
	double** spol;

	/** @brief Maximum number of iterations */
	int maxit;
	/** @brief Numerical error tolerance */
	double tol;

	/** @brief Achieved numerical accuracy */
	double acc;
	/** @brief Iteration count */
	int it;
	/** @brief Execution start */
	double xbeg;
	/** @brief Execution end */
	double xend;
};
/** @brief Solution type */
typedef struct sol_st sol_t;

void solution_init(sol_t* s, const struct pmap_st* pmap);
void solution_load(sol_t* s, const char* model_path);
void solution_save(const sol_t* s, const char* model_path);
void solution_free(sol_t* s);

#endif /* RAD_TYPES_H_ */

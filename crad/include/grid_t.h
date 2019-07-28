/** @file grid_t.h
 * @brief Grid type and grid functionality.
 * @details A grid is a structure that contains discretization data of a
 * model's variables. It can be initialized by an `initialization
 * string´, i.e. a string that describes how to construct the grid. It can also
 * be initialized giving giving the boundary points, the weighting exponent
 * and the number of grid points. Data allocation is dynamic and releasing
 * memory is the responsibility of the caller. */

#ifndef GRID_T_H_
#define GRID_T_H_

/** @brief Grid structure
 * @details Contains data for grid allocation, creation and storage. The
 * weighting of the grid controls the distribution of grid points over the
 * grid's domain. The weighting is performed using a power function. The
 * weighting parameter is expected to be positive. */
struct grid_st {
	/** @brief Number of grid points */
	short n;
	/** @brief Minimum grid point */
	double m;
	/** @brief Maximum grid point */
	double M;
	/** @brief Weighting exponent */
	double w;
	/** @brief Data */
	double* d;
};
/** @brief Grid type */
typedef struct grid_st grid_t;

void grid_init(grid_t* g, short n, double m, double M, double w);
void grid_init_str(grid_t* g, const char* init_str);

void grid_copy(grid_t* dest, const grid_t* source);

void grid_calc(grid_t* g);

int grid_save(const grid_t* g, const char* filename);
int grid_load(grid_t* g, const char* filename);

void grid_free(grid_t* g);

short grid_liei(const grid_t* g, double X);

#endif /* GRID_T_H_ */

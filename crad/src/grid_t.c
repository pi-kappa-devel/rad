#include "grid_t.h"

#include "cross_comp.h"
#include "logger.h"

#include "assert.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "errno.h"

#ifndef GRID_T_INIT_STR_MASK
#define GRID_T_INIT_STR_MASK "%d, %lf, %lf, %lf"
#endif

void alloc_grid(grid_t *g) {
  g->d = (double *)malloc(sizeof(double) * (g->n));
#ifdef GRID_T_SAFE_MODE
  if (!g->d) {
    LOGE("Failed to allocated memory for grid data");
    exit(EXIT_FAILURE);
  }
#endif
}

/** @brief Grid initialization
 * @details Sets up the output onject's fields, allocates an array of n points
 * to hold the data and calls calc_grid().
 * @param g Output grid object
 * @param n Number of grid points
 * @param m Minimum grid point
 * @param M Maximum grid point
 * @param w Weighting exponent */
void grid_init(grid_t *g, short n, double m, double M, double w) {
  g->n = n;
  g->m = m;
  g->M = M;
  g->w = w;
  alloc_grid(g);
  grid_calc(g);
}

/** @brief Grid initialization from string
 * @details Parses the string using GRID_T_INIT_STR_MASK and calls
 * init_grid() using the parsed data.
 * @param g Output grid object
 * @param init_str Initialization string */
void grid_init_str(grid_t *g, const char *init_str) {
  size_t len = strlen(init_str);
  char *buf = (char *)calloc(len, sizeof(char));
  memcpy(buf, init_str, len);
  char *pbuf;

  pbuf = strtok(buf, ",");
  if (pbuf)
    g->n = (short)atoi(pbuf);
  pbuf = strtok(NULL, ",");
  if (pbuf)
    g->m = atof(pbuf);
  pbuf = strtok(NULL, ",");
  if (pbuf)
    g->M = atof(pbuf);
  pbuf = strtok(NULL, ",");
  if (pbuf)
    g->w = atof(pbuf);
  else
    g->w = 1;

  free(buf);

  alloc_grid(g);
  grid_calc(g);
}

/** @brief Grid copy
 * @details Performs a deep copy of one grid to another.
 * @param dest Destination grid
 * @param source Source grid */
void grid_copy(grid_t *dest, const grid_t *source) {
  memcpy(dest, source, sizeof(*source));
  dest->d = (double *)calloc(dest->n, sizeof(double));
  memcpy(dest->d, source->d, dest->n * sizeof(double));
}

/** Grid calculation
 * @brief Calculates the grip points.
 * @details The function expects that the data array is allocated. It also
 * expects that \f$ m<M \f$ and \f$ w>0 \f$. If GRID_T_SAFE_MODE is true and
 * any of the last two conditions is not satisfied, it prints a warning without
 * breaking execution. The resulting behavior in such a case is undefined.
 *
 * The distribution of grid points is calculated using a power function with
 * exponent g.w. The weighting function is applied to an equidistant
 * distribution on \f$ [0,1] \f$ and is then mapped to the grid's domain.
 * @param g Output grid object */
void grid_calc(grid_t *g) {
#ifdef GRID_T_SAFE_MODE
  if (!(g->M > g->m)) {
    LOGE("Invalid grid domain definition");
  }
  if (!(g->w > 0)) {
    LOGE("Invalid grid weighting exponent");
  }
#endif
  double h = (g->M - g->m) / pow(g->n - 1, g->w);
  for (int i = 0; i < g->n; ++i) {
    g->d[i] = g->m + pow(i, g->w) * h;
  }
}

/** @brief Binary save
 * @details Creates a grid binary file using the passed filename and
 * stores the grid's data in it. The format of grid binary files is
 * the following:
 *  - The first sizeof(g->n) bytes represent the number of elements
 *    of the array as an integer.
 *  - The next sizeof(g->w) bytes represent the weighting exponent
 *    of the array as a floating point number.
 *  - The rest of the bytes represent the grid points as floating point
 *    numbers.
 * @param g Grid object
 * @param filename Output file name
 * @return Zero on success. If GRID_T_SAFE_MODE, it returns -1 if it fails to
 * open the output file. */
int grid_save(const grid_t *g, const char *filename) {
  FILE *fh = NULL;
  fh = fopen(filename, "wb");
#ifdef GRID_T_SAFE_MODE
  if (!fh) {
    LOGE("Failed to create '%f' with errno %d", filename, errno);
    return errno;
  }
#endif
  fwrite(&g->n, sizeof(g->n), 1, fh);
  fwrite(&g->w, sizeof(g->w), 1, fh);
  fwrite(g->d, sizeof(g->m), g->n, fh);
  fclose(fh);

  return 0;
}

/** @brief Binary load
 * @details Populates the given grid structure from the grid binary file with
 * the passed filename. The expected format of grid binary can be found in
 * save_grid().
 * @param g Grid object
 * @param filename Output file name
 * @return Zero on success. If GRID_T_SAFE_MODE, it returns -1 if it fails to
 * open the input file. */
int grid_load(grid_t *g, const char *filename) {
  FILE *fh = NULL;
  fh = fopen(filename, "rb");
#ifdef GRID_T_SAFE_MODE
  if (!fh) {
    LOGE("Failed to open '%f' with errno %d", filename, errno);
    return errno;
  }
#endif
  fread(&g->n, sizeof(g->n), 1, fh);
  fread(&g->w, sizeof(g->w), 1, fh);
  if (g->n > 0) {
    alloc_grid(g);
    fread(g->d, sizeof(g->m), g->n, fh);
    g->m = g->d[0];
    g->M = g->d[g->n - 1];
  }
  fclose(fh);

  return 0;
}

/** @brief Grid disallocation
 * @details Frees grid's data array.
 * @param g Output grid object*/
void grid_free(grid_t *g) { free(g->d); }

/** @brief Lower interpolation-extrapolation index
 * @details Searches the grid array for the greatest domain value that is
 * lower than the passed X value and returns the corresponding index. If
 * the passed value  is lower than the minimum grid value, then the
 * function returns zero. If the passed value is greater than the
 * greatest value, then it returns the previous to last index. The function
 * assumes that the data array of the grid object has at least two values.
 * @param g Grid object
 * @param X Domain value. */
short grid_liei(const grid_t *g, double X) {
  short li = 0, ui = g->n, mi = (g->n % 2) ? (g->n + 1) / 2 : g->n / 2;

  if (X <= g->m)
    mi = 0;
  else if (X > g->M)
    mi = g->n - 2;
  else {
    while (ui - li > 1) {
      if (X < g->d[mi])
        ui = mi;
      else
        li = mi;
      mi = (ui + li) / 2;
    }
    assert(X >= g->d[mi] && X < g->d[mi + 1]);
  }

  return mi;
}

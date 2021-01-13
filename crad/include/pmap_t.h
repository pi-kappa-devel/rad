/** @file pmap_t.h
 * @brief Parameter map type and functionality.
 * @details Models parameter maps. Parameter maps are arrays of key-value
 * parameter values. They are constructed by parsing parameter files.
 * Parameter files are plain text files, the
 * lines of which are key-value pairs. These pairs are parsed based on the
 * mask defined by PMAP_PARAM_MASK. */

#ifndef PMAP_T_H_
#define PMAP_T_H_

struct pmap_pair_st;

/** @brief Parameter file structure
 * @details Contains data for grid allocation, creation and storage. The
 * weighting of the grid controls the distribution of grid points over the
 * grid's domain. The weighting is performed using a power function. The
 * weighting parameter is expected to be positive. */
struct pmap_st {
  /** @brief Number of stored parameters */
  int n;
  /** @brief Key-value pair array */
  struct pmap_pair_st *p;
};
/** @brief Parameter file type */
typedef struct pmap_st pmap_t;

int pmap_init(pmap_t *pmap, const char *pfilename);

void pmap_add(pmap_t *pmap, const char *key, const char *val);
void pmap_add_int(pmap_t *pmap, const char *key, int val);
void pmap_add_double(pmap_t *pmap, const char *key, double val);
void pmap_save(const pmap_t *pmap, const char *pfilename);

void pmap_free(pmap_t *pmap);

const char *pmap_find(const pmap_t *pmap, const char *key);
const char *pmap_gkey(const pmap_t *pmap, int i);
const char *pmap_gvalue(const pmap_t *pmap, int i);

void pmap_cvalue(char **valbuf, const pmap_t *pmap, int i);

#endif /* PMAP_T_H_ */

/** @file rad_setup.h
 * @brief Radial attention model solver. */

#ifndef RAD_SETUP_H_
#define RAD_SETUP_H_

struct objpart_st;
struct model_st;
struct sol_st;

struct concurrency_st;

/** Setup structure
 * @brief Execution consolidating structure
 * @details Contains pointers both model and solution data.  */
struct setup_st {
	/** @brief Model data */
	struct model_st* m;
	/** @brief Solution approximation data */
	struct sol_st* s;

	/** @brief Concurrency data */
	struct concurrency_st* c;
};
/** @brief Setup type */
typedef struct setup_st setup_t;

int setup_init(setup_t* u, const char* parameter_filename, const struct objpart_st* obhparts);
int setup_solve(setup_t* u);
int setup_resume(setup_t* u);
void setup_load(setup_t* u, const char* setup_path, const struct objpart_st* obhparts);
void setup_save(const setup_t* u, const char* setup_path);
void setup_free(setup_t* u);

int setup_find_last_saved(char* save_point);

#endif /* RAD_SETUP_H_ */

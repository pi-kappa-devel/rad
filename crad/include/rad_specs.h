/** @file rad_specs.h
 * @brief Radial attention model functional specification.
 * @details This file contains the declarations of the radial attention
 * example of the article. It uses exponential specification for effort
 * costs. If you want to change the specification you have to redefine the
 * corresponding macros of this file and re-compile. */

#ifndef RAD_SPECS_H_
#define RAD_SPECS_H_

struct objvar_st;

#define _radt_ (1.0 - (1.0 - v->m->delta*v->r)*exp(-v->s))
double radt(const struct objvar_st* v);

#define _util_ (_radt_*(1.0 - exp(-v->q)))
double util(const struct objvar_st* v);

#define _cost_ ((exp(v->m->alpha*v->s)-1.0)*(1.0-v->m->gamma*_radt_))
double cost(const struct objvar_st* v);

#define _wltt_ v->m->R*(v->x-_radt_*v->q)
double wltt(const struct objvar_st* v);

#endif /* RAD_SPECS_H_ */

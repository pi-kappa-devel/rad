#include "rad_specs.h"
#include "rad_types.h"

#include "math.h"

/** @brief Radius transition
 * @details Calculates the next date's radius by
 * \f[
 * r'(s,r) =  1 - (1 - \delta r) \mathrm{e}^{-s}.
 * \f]
 * @param v Objective function data
 * @return Calculated next date's radius. */
double radt(const struct objvar_st* v){
	return _radt_;
}

/** @brief Temporal utility
 * @details Calculates the temporal utility by
 * \f[
 * u(q,r) = r'(s,r) (1 - \mathrm{e}^{-q}).
 * \f]
 * @param v Objective function data
 * @return Calculated temporal utility.
 * @see radt(const objvar_t*) */
double util(const struct objvar_st* v) {
	return _util_;
}

/** @brief Attentional costs
 * @details Calculates the attentional costs by
 * \f[
 * c(s,r') = (\mathrm{e}^{\alpha s} - 1)(1 - \gamma r'(s,r)).
 * \f]
 * @param v Objective function data
 * @return Calculated costs.
 * @see radt(const objvar_t*) */
double cost(const struct objvar_st* v) {
	return _cost_;
}

/** @brief Wealth transition
 * @details Calculates the next date's wealth
 * \f[
 * d(x,r') = R (x - r'(s,r) q).
 * \f]
 * @param v Objective function data
 * @return Calculated costs.
 * @see radt(const objvar_t*) */
double wltt(const struct objvar_st* v) {
	return _wltt_;
}

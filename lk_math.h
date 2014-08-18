#ifndef __lk_math_h
#define __lk_math_h

#include "lk_env.h"

namespace lk {
	double besj0(double x);
	double besj1(double x);
	double besy0(double x);
	double besy1(double x);
	double besi0(double x);
	double besk0(double x);
	double besi1(double x);
	double besk1(double x);

	double gammln(double xx);
	double betacf(double a, double b, double x) throw(lk::error_t);
	double betai(double a, double b, double x) throw( lk::error_t );

	double pearson(double *x, double *y, size_t len );

	void gser(double *gamser, double a, double x, double *gln);
	void gcf(double *gammcf, double a, double x, double *gln);
	double gammp(double a, double x);
	double gammq(double a, double x);
	double erf(double x);
	double erfc(double x);
};

#endif

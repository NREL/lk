
#include "lk_str.h"

#ifdef __WX__


lk_char lk::lower_char( lk_char c )
{
	return wxTolower(c);
}

lk_char lk::upper_char( lk_char c )
{
	return wxToupper(c);
}

bool lk::convert_integer(const lk_string &str, int *x)
{
	long lval;
	bool ok = str.ToLong(&lval);
	if (ok)
	{
		*x = (int) lval;
		return true;
	}
	else return false;
}

bool lk::convert_double(const lk_string &str, double *x)
{
	return str.ToDouble(x);
}

#else

lk_char lk::lower_char( lk_char c )
{
	return ::tolower( c );
}

lk_char lk::upper_char( lk_char c )
{
	return ::toupper( c );
}

bool lk::convert_integer(const lk_string &str, int *x)
{
	const lk_char *startp = str.c_str();
	lk_char *endp = NULL;
	*x = ::strtol( startp, &endp, 10 );
	return !*endp && (endp!=startp);
}

bool lk::convert_double(const lk_string &str, double *x)
{
	const lk_char *startp = str.c_str();
	lk_char *endp = NULL;
	*x = ::strtod( startp, &endp );
	return !*endp && (endp!=startp);
}

#endif

#ifndef __lk_str_h
#define __lk_str_h


#if defined(__WXMSW__) || defined(__WXMAC__)
#include <wx/string.h>

typedef wxChar lk_char;
typedef wxString lk_string;

#else
#include <string>

typedef std::string::value_type lk_char;
typedef std::string lk_string;
#endif

namespace lk
{
	lk_char lower_char( lk_char c );
	lk_char upper_char( lk_char c );
	bool convert_integer( const lk_string &str, int *x );
	bool convert_double( const lk_string &str, double *x );
};


#endif

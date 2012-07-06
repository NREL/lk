#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../lk_invoke.h"

// DLL functions look like
LK_FUNCTION( externtest )
{
	LK_DOCUMENT("externtest", "Tests basic external calling of dynamically loaded dll functions.", "(string, string):number" );
	
	if (lk_arg_count() < 2)
	{
		lk_error( "too few arguments passed to externtest. two strings required!" );
		return;
	}
	
	const char *s1 = lk_as_string( lk_arg( 0 ) );
	if (s1 == 0)
	{
		lk_error( "s1 null" );
		return;
	}
	int len1 = strlen(s1);
	
	const char *s2 = lk_as_string( lk_arg(1) );
	if (s2 == 0)
	{
		lk_error( "s2 null" );
		return;
	}
	int len2 = strlen( s2 );
		
	lk_return_number( len1 + len2 );
}


void steam_psat( struct __lk_invoke_t *lk )
{
	LK_DOCUMENT("steam_psat", "Returns the saturation pressure (kPa) of steam at a given temperature in deg C", "(number:Tc):number");
	
	if ( lk_arg_count() < 1 )
	{
		lk_error( "insufficient arguments provided");
		return;
	}
	
	double T = lk_as_number( lk_arg(0) ) + 273.15;
	
	double Tc = 647.096;
	double Pc = 22.064;
	
	double a1 = -7.85954783;
	double a2 = 1.84408259;
	double a3 = -11.7866497;
	double a4 = 22.6807411;
	double a5 = -15.9618719;
	double a6 = 1.80122502;
	double tau = 1 - T/Tc;
	double tauh = sqrt(tau);
	
	double P = Pc * exp( Tc/T*(a1*tau 
		+ a2*tau*tauh 
		+ a3*tau*tau*tau 
		+ a4*tau*tau*tau*tauh 
		+ a5*tau*tau*tau*tau 
		+ a6*tau*tau*tau*tau*tau*tau*tau*tauh) );
		
	lk_return_number( P*1000 );
}


LK_BEGIN_EXTENSION()
	externtest,
	steam_psat
LK_END_EXTENSION()
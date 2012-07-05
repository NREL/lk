#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../lk_invoke.h"

// DLL functions look like
void externtest(  struct __lk_invoke_t *lk )
{
	LK_DOCUMENT("externtest", "Tests basic external calling of dynamically loaded dll functions.", "(string, string):number" );
	
	int args = lk->arg_count( lk );
	if (args < 2)
	{
		lk->error( lk, "too few arguments passed to externtest. two strings required!" );
		return;
	}
	
	lk_var_t a0 = lk->arg(lk, 0);
	
	if (a0 == 0)
	{
		lk->error(lk, "a0 null");
		return;
	}
	
	lk_var_t a1 = lk->arg(lk, 1);
	if (a1 == 0)
	{
		lk->error(lk, "a1 null");
		return;
	}
	
	char *s1 = (char*)lk->as_string(lk, a0);
	if (s1 == 0)
	{
		lk->error(lk, "s1 null");
		return;
	}
	int len1 = strlen(s1);
	
	char *s2 = (char*)lk->as_string(lk, a1);
	if (s2 == 0)
	{
		lk->error(lk, "s2 null");
		return;
	}
	int len2 = strlen( s2 );
	
	
	lk_var_t ret = lk->result(lk);
	if (ret == 0)
	{
		lk->error(lk, "ret null");
		return;
	}
	
	lk->set_number( lk, ret, len1 + len2 );
}


void steam_psat( struct __lk_invoke_t *lk )
{
	LK_DOCUMENT("steam_psat", "Returns the saturation pressure (kPa) of steam at a given temperature in deg C", "(number:Tc):number");
	
	if ( lk->arg_count(lk) < 1 )
	{
		lk->error(lk, "insufficient arguments provided");
		return;
	}
	
	double T = lk->as_number(lk, lk->arg(lk, 0)) + 273.15;
	
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
		
	lk->set_number( lk, lk->result( lk ), P*1000 );
}


LK_BEGIN_EXTENSION()
	externtest,
	steam_psat
LK_END_EXTENSION()
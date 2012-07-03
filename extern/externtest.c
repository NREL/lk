#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lk_invoke.h"

// DLL functions look like
void externtest( lk_invoke_t *lk )
{
	LK_DOC( "externtest", "Tests basic external calling of dynamically loaded dll functions.", "(string, string):number" );

	int args = lk->arg_count( lk );
	if (args < 2)
	{
		lk->error( "too few arguments passed to externtest. two strings required!" );
		return;
	}
	
	int len1 = strlen( lk->as_string( lk, lk->arg( lk, 0 ) ) );
	int len2 = strlen( lk->as_string( lk, lk->arg( lk, 1 ) ) );
	
	lk->set_number( lk, lk->result( lk ), len1 + len2 );
}

static lk_invokable my_tab[] = {
	externtest,
	0
};

lk_invokable *lk_function_table() {	return my_tab; }
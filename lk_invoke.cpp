#include "lk_env.h"
#include "lk_invoke.h"


static int doc_mode( struct __lk_invoke_t )
{
}

static void document( struct __lk_invoke_t, const char *, const char *, const char * )
{
}

static void error( struct __lk_invoke_t, const char * )
{
}

static int arg_count( struct __lk_invoke_t )
{
}

static lk_var_t arg( struct __lk_invoke_t, int )
{
}

int type( struct __lk_invoke_t, lk_var_t )
{
}

const char *as_string( struct __lk_invoke_t, lk_var_t )
{
}

double as_number( struct __lk_invoke_t, lk_var_t )
{
}

int as_integer( struct __lk_invoke_t, lk_var_t )
{
}

int as_boolean( struct __lk_invoke_t, lk_var_t )
{
}

int vec_count( struct __lk_invoke_t, lk_var_t )
{
}

lk_var_t vec_index( struct __lk_invoke_t, lk_var_t, int )
{
}

	
int tab_count( struct __lk_invoke_t, lk_var_t )
{
}

const char * tab_first_key( struct __lk_invoke_t, lk_var_t )
{
}

const char * tab_next_key( struct __lk_invoke_t, lk_var_t )
{
}

lk_var_t tab_value( struct __lk_invoke_t, lk_var_t, const char * )
{
}

	
lk_var_t result( struct __lk_invoke_t )
{
}


// variable modifications
void set_null( struct __lk_invoke_t, lk_var_t )
{
}

void set_string( struct __lk_invoke_t, lk_var_t, const char * )
{
}

void set_number( struct __lk_invoke_t, lk_var_t, double )
{
}


void set_number_vec( struct __lk_invoke_t, lk_var_t, double *, int )
{
}

void make_vec( struct __lk_invoke_t, lk_var_t )
{
}

void reserve( struct __lk_invoke_t, lk_var_t, int len )
{
}

lk_var_t append_number( struct __lk_invoke_t, lk_var_t, double )
{
}

lk_var_t append_string( struct __lk_invoke_t, lk_var_t, const char * )
{
}

lk_var_t append_null( struct __lk_invoke_t, lk_var_t )
{
}


void make_tab( struct __lk_invoke_t, lk_var_t )
{
}

lk_var_t tab_set_number( struct __lk_invoke_t, lk_var_t, const char *, double )
{
}

lk_var_t tab_set_string( struct __lk_invoke_t, lk_var_t, const char *, const char * )
{
}

lk_var_t tab_set_null( struct __lk_invoke_t, lk_var_t, const char * )
{
}


namespace lk {

	void external_call( lk_invokable p, lk::invoke_t &cxt )
	{
		lk::varhash_t::iterator hash_iter;
		lk_invoke_t ext;
		memset( &ext, 0, sizeof(lk_invoke_t) );

		ext.__pinvoke = &cxt;
		ext.__hiter = &hash_iter;

		ext.doc_mode = doc_mode;
		ext.document = document;
		ext.error = error;
		ext.arg_count = arg_count;
		ext.arg = arg;
		ext.type = type;
		ext.as_string = as_string;
		ext.as_integer = as_integer;
		ext.as_boolean = as_boolean;
		ext.vec_count = vec_count;
		ext.vec_index = vec_index;
		ext.tab_count = tab_count;
		ext.tab_first_key = tab_first_key;
		ext.tab_next_key = tab_next_key;
		ext.tab_value = tab_value;
		ext.result = result;
		ext.set_null = set_null;
		ext.set_string = set_string;
		ext.set_number = set_number;
		ext.set_number_vec = set_number_vec;
		ext.make_vec = make_vec;
		ext.reserve = reserve;
		ext.append_number = append_number;
		ext.append_string = append_string;
		ext.append_null = append_null;
		ext.make_tab = make_tab;
		ext.tab_set_number = tab_set_number;
		ext.tab_set_string = tab_set_string;
		ext.tab_set_null = tab_set_null;



	}

};
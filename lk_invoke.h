#ifndef __lk_invoke_h
#define __lk_invoke_h

#ifdef __cplusplus
extern "C" {
#endif

typedef void* lk_var_t;

#define LK_NULLVAL 1
#define LK_NUMBER  3
#define LK_STRING  4
#define LK_VECTOR  5
#define LK_HASH    6

struct __lk_invoke_t
{
	void *__pinvoke; // internal calling context reference
	void *__hiter; // internal key interator context
	void *__sbuf; // internal string buffer

	int (*doc_mode)( struct __lk_invoke_t );
	void (*document)( struct __lk_invoke_t, const char *, const char *, const char * );
	void (*error)( struct __lk_invoke_t, const char * );
	int (*arg_count)( struct __lk_invoke_t );
	lk_var_t (*arg)( struct __lk_invoke_t, int );

	int (*type)( struct __lk_invoke_t, lk_var_t );
	const char *(*as_string)( struct __lk_invoke_t, lk_var_t );
	double (*as_number)( struct __lk_invoke_t, lk_var_t );
	int (*as_integer)( struct __lk_invoke_t, lk_var_t );
	int (*as_boolean)( struct __lk_invoke_t, lk_var_t );

	int (*vec_count)( struct __lk_invoke_t, lk_var_t );
	lk_var_t (*vec_index)( struct __lk_invoke_t, lk_var_t, int );
	
	int (*tab_count) ( struct __lk_invoke_t, lk_var_t );
	const char * (*tab_first_key)( struct __lk_invoke_t, lk_var_t );
	const char * (*tab_next_key)( struct __lk_invoke_t, lk_var_t );
	lk_var_t (*tab_value)( struct __lk_invoke_t, lk_var_t, const char * );
	
	lk_var_t (*result)( struct __lk_invoke_t );

	// variable modifications
	void (*set_null)  ( struct __lk_invoke_t, lk_var_t );
	void (*set_string)( struct __lk_invoke_t, lk_var_t, const char * );
	void (*set_number)( struct __lk_invoke_t, lk_var_t, double );

	void (*set_number_vec)( struct __lk_invoke_t, lk_var_t, double *, int );
	void (*make_vec)( struct __lk_invoke_t, lk_var_t );
	void (*reserve)( struct __lk_invoke_t, lk_var_t, int len );
	lk_var_t (*append_number)( struct __lk_invoke_t, lk_var_t, double );
	lk_var_t (*append_string)( struct __lk_invoke_t, lk_var_t, const char * );
	lk_var_t (*append_null)( struct __lk_invoke_t, lk_var_t );

	void (*make_tab)( struct __lk_invoke_t, lk_var_t );
	lk_var_t (*tab_set_number)( struct __lk_invoke_t, lk_var_t, const char *, double );
	lk_var_t (*tab_set_string)( struct __lk_invoke_t, lk_var_t, const char *, const char * );
	lk_var_t (*tab_set_null)( struct __lk_invoke_t, lk_var_t, const char * );
};

typedef struct __lk_invoke_t lk_invoke_t;

// function table must look like
typedef void (*lk_invokable)( lk_invoke_t * );

#define LK_DOCUMENT(  fn, desc, sig ) if (lk->doc_mode()) { lk->document( fn, desc, sig ); return; }

#ifdef __cplusplus
}
#endif

#endif

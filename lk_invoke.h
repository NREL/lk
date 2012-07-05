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
	char __errbuf[256]; // error message buffer
	void *__sbuf; // internal string buffer
	void *__callargvec; // internal call argument vector 
	void *__callresult; // internal call result

	int (*doc_mode)( struct __lk_invoke_t* );
	void (*document)( struct __lk_invoke_t*, const char *, const char *, const char * );
	void (*error)( struct __lk_invoke_t*, const char * );
	int (*arg_count)( struct __lk_invoke_t* );
	lk_var_t (*arg)( struct __lk_invoke_t*, int );

	int (*type)( struct __lk_invoke_t*, lk_var_t );
	const char *(*as_string)( struct __lk_invoke_t*, lk_var_t );
	double (*as_number)( struct __lk_invoke_t*, lk_var_t );
	int (*as_integer)( struct __lk_invoke_t*, lk_var_t );
	int (*as_boolean)( struct __lk_invoke_t*, lk_var_t );

	int (*vec_count)( struct __lk_invoke_t*, lk_var_t );
	lk_var_t (*vec_index)( struct __lk_invoke_t*, lk_var_t, int );
	
	int (*tab_count) ( struct __lk_invoke_t*, lk_var_t );
	const char * (*tab_first_key)( struct __lk_invoke_t*, lk_var_t );
	const char * (*tab_next_key)( struct __lk_invoke_t*, lk_var_t );
	lk_var_t (*tab_value)( struct __lk_invoke_t*, lk_var_t, const char * );
	
	lk_var_t (*result)( struct __lk_invoke_t* );

	// variable modifications
	void (*set_null)  ( struct __lk_invoke_t*, lk_var_t );
	void (*set_string)( struct __lk_invoke_t*, lk_var_t, const char * );
	void (*set_number)( struct __lk_invoke_t*, lk_var_t, double );

	void (*set_number_vec)( struct __lk_invoke_t*, lk_var_t, double *, int );
	void (*make_vec)( struct __lk_invoke_t*, lk_var_t );
	void (*reserve)( struct __lk_invoke_t*, lk_var_t, int len );
	lk_var_t (*append_number)( struct __lk_invoke_t*, lk_var_t, double );
	lk_var_t (*append_string)( struct __lk_invoke_t*, lk_var_t, const char * );
	lk_var_t (*append_null)( struct __lk_invoke_t*, lk_var_t );

	void (*make_tab)( struct __lk_invoke_t*, lk_var_t );
	lk_var_t (*tab_set_number)( struct __lk_invoke_t*, lk_var_t, const char *, double );
	lk_var_t (*tab_set_string)( struct __lk_invoke_t*, lk_var_t, const char *, const char * );
	lk_var_t (*tab_set_null)( struct __lk_invoke_t*, lk_var_t, const char * );

	// creating, querying, destroying externally defined object types
	int (*insert_object)( struct __lk_invoke_t*, const char *type, void*, void (*)(void *, void *), void * );
	void *(*query_object)( struct __lk_invoke_t*, int );
	void (*destroy_object)( struct __lk_invoke_t*, int );

	// invoking other functions in LK environment (i.e. for callbacks)
	void (*clear_call_args)( struct __lk_invoke_t* );
	lk_var_t (*append_call_arg)( struct __lk_invoke_t* );
	lk_var_t (*call_result)( struct __lk_invoke_t* );
	const char *(*call)( struct __lk_invoke_t*, const char *name ); // returns 0 on success, error message otherwise.
};


// function table must look like
typedef void (*lk_invokable)( struct __lk_invoke_t * );

#define LK_DOCUMENT(  fn, desc, sig ) if (lk->doc_mode(lk)) { lk->document( lk, fn, desc, sig ); return; }


// DLL must export 2 functions:
// int lk_extension_api_version()
// lk_invokable *lk_function_list()

#define LK_EXTENSION_API_VERSION 1001


#if defined(__WINDOWS__)||defined(WIN32)||defined(_WIN32)||defined(__MINGW___)||defined(_MSC_VER)
#define LKAPIEXPORT __declspec(dllexport)
#else
#define LKAPIEXPORT
#endif


#define LK_BEGIN_EXTENSION() \
	LKAPIEXPORT int lk_extension_api_version() \
	{ return LK_EXTENSION_API_VERSION; } \
	LKAPIEXPORT lk_invokable *lk_function_list() { \
	static lk_invokable _ll[] = {

#define LK_END_EXTENSION() ,0 }; return _ll; }


/* examples:

void mean_func( struct __lk_invoke_t *lk )
{
	LK_DOCUMENT( "mean", "Returns the average of an array of numbers.", "(array):number" );

	lk->set_number( lk->result(lk), 1.3 );
}

LK_BEGIN_EXTENSION()
	mean_func, sigma_func, sum_func, 
	xmult_func, average_func
LK_END_EXTENSION()

*/



#ifdef __cplusplus
}
#endif

#endif

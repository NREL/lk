#ifndef __lk_stdlib_h
#define __lk_stdlib_h

#include "lk_env.h"

namespace lk {
	
	/* these stdlib_xxxx() functions
		return an array of fcall_t references.
		the end of the list is denoted by a null fcall_t */
#ifdef __WX__
	LKEXPORT fcall_t* stdlib_wxui();
#endif

	LKEXPORT fcall_t* stdlib_basic();
	LKEXPORT fcall_t* stdlib_string();
	LKEXPORT fcall_t* stdlib_math();

	LKEXPORT bool tex_doc( const lk_string &file,
				  const lk_string &title,
				  std::vector<fcall_t> lib );


	LKEXPORT std::vector< lk_string > dir_list( const lk_string &dir, const lk_string &extlist, bool ret_dirs=false );

	LKEXPORT std::vector< lk_string > split( const lk_string &str, const lk_string &delim, bool ret_empty=false, bool ret_delim=false );
	LKEXPORT lk_string join( const std::vector< lk_string > &list, const lk_string &delim );

	LKEXPORT bool to_integer(const lk_string &str, int *x);
	LKEXPORT bool to_float(const lk_string &str, float *x);
	LKEXPORT bool to_double(const lk_string &str, double *x);

	LKEXPORT lk_string lower_case( const lk_string &in );
	LKEXPORT lk_string upper_case( const lk_string &in );

	LKEXPORT size_t replace( lk_string &s, const lk_string &old_text, const lk_string &new_text);

	LKEXPORT lk_string read_file( const lk_string &file );
	LKEXPORT bool read_line( FILE *fp, lk_string &text, int prealloc = 256 );

	LKEXPORT bool file_exists( const char *file );
	LKEXPORT bool dir_exists( const char *path );
	LKEXPORT bool remove_file( const char *path );
	LKEXPORT bool mkdir( const char *path, bool make_full = false);
	LKEXPORT lk_string path_only( const lk_string &path );
	LKEXPORT lk_string name_only( const lk_string &path );
	LKEXPORT lk_string ext_only( const lk_string &path );
	LKEXPORT char path_separator();
	LKEXPORT lk_string get_cwd();
	LKEXPORT bool set_cwd( const lk_string &path );

	class LKEXPORT sync_piped_process
	{
	public:
		sync_piped_process() {  }
		virtual ~sync_piped_process() {  }

		int spawn(const lk_string &command, const lk_string &workdir="");
		virtual void on_stdout(const lk_string &line_text) = 0;
	};


	LKEXPORT lk_string trim_to_columns(const lk_string &str, int numcols);
	LKEXPORT lk_string format_vl( const lk_string &fmt, const std::vector< vardata_t* > &args );
	LKEXPORT lk_string format(const char *fmt, ...);
	LKEXPORT size_t format_vn(char *buffer, int maxlen, const char *fmt, va_list arglist);

};


#endif

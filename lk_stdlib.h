#ifndef __lk_stdlib_h
#define __lk_stdlib_h

#include "lk_env.h"

namespace lk {

	LKEXPORT std::vector<fcall_t> stdlib_basic();
	LKEXPORT std::vector<fcall_t> stdlib_string();
	LKEXPORT std::vector<fcall_t> stdlib_math();

	LKEXPORT bool tex_doc( const std::string &file,
				  const std::string &title,
				  std::vector<fcall_t> lib );


	LKEXPORT std::vector< std::string > dir_list( const std::string &dir, const std::string &extlist, bool ret_dirs=false );

	LKEXPORT std::vector< std::string > split( const std::string &str, const std::string &delim, bool ret_empty=false, bool ret_delim=false );
	LKEXPORT std::string join( const std::vector< std::string > &list, const std::string &delim );

	LKEXPORT bool to_integer(const std::string &str, int *x);
	LKEXPORT bool to_float(const std::string &str, float *x);
	LKEXPORT bool to_double(const std::string &str, double *x);

	LKEXPORT std::string to_string( int x, const char *fmt="%d" );
	LKEXPORT std::string to_string( double x, const char *fmt="%lg" );

	LKEXPORT std::string lower_case( const std::string &in );
	LKEXPORT std::string upper_case( const std::string &in );

	LKEXPORT size_t replace( std::string &s, const std::string &old_text, const std::string &new_text);

	LKEXPORT std::string read_file( const std::string &file );
	LKEXPORT bool read_line( FILE *fp, std::string &text, int prealloc = 256 );

	LKEXPORT bool file_exists( const char *file );
	LKEXPORT bool dir_exists( const char *path );
	LKEXPORT bool remove_file( const char *path );
	LKEXPORT bool mkdir( const char *path, bool make_full = false);
	LKEXPORT std::string path_only( const std::string &path );
	LKEXPORT std::string name_only( const std::string &path );
	LKEXPORT std::string ext_only( const std::string &path );
	LKEXPORT char path_separator();
	LKEXPORT std::string get_cwd();
	LKEXPORT bool set_cwd( const std::string &path );

	class LKEXPORT sync_piped_process
	{
	public:
		sync_piped_process() {  }
		virtual ~sync_piped_process() {  }

		int spawn(const std::string &command, const std::string &workdir="");
		virtual void on_stdout(const std::string &line_text) = 0;
	};


	LKEXPORT std::string trim_to_columns(const std::string &str, int numcols);
	LKEXPORT std::string format_vl( const std::string &fmt, const std::vector< vardata_t* > &args );
	LKEXPORT std::string format(const char *fmt, ...);
	LKEXPORT size_t format_vn(char *buffer, int maxlen, const char *fmt, va_list arglist);

};


#endif
